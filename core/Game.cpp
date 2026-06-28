#include "core/Game.h"
#include "board/BoardWidget.h"
#include "bench/Bench.h"
#include "bench/BenchWidget.h"
#include "unit/Unit.h"
#include "unit/UnitItem.h"
#include "skill/HeroSkills.h"       // 技能派生类（提供敌方技能分配）
#include "equipment/Equipment.h"    // 装备（读档时恢复用）
#include "ui/ResourceManager.h"

#include <QtMath>            // qSqrt
#include <QRandomGenerator>
#include <algorithm>         // std::min, std::sort
#include <array>             // std::array (processMerges 中存放旧 3 单位)
#include <exception>
#include <QMap>
#include <QSet>

// ============================================================================
// 文件级技能实例（stateless，供敌方单位共享）
// ============================================================================
namespace {
    PowerStrike   s_enemyPowerStrike;    // 重击（单体高伤）→ 战士型敌人
    ArrowRain     s_enemyArrowRain;      // 箭雨（范围 AOE）→ 射手型敌人
    IronWall      s_enemyIronWall;       // 铁壁（自愈）→ 坦克型敌人

    // 装备实例（stateless，读档时恢复装备用）
    Sword   s_sword;
    Armor   s_armor;
    Glove   s_glove;
    Crystal s_crystal;
}

// ============================================================================
// 构造
// ============================================================================

Game::Game(BoardWidget *board, QObject *parent)
    : QObject(parent)
    , m_board(board)
{
    // 创建定时器，每 16ms 触发一次（≈ 62.5fps）
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);   // 精确计时，减少帧间隔抖动

    // 连接定时器的 timeout 信号到 onTimerTick 槽
    connect(m_timer, &QTimer::timeout, this, &Game::onTimerTick);

    // 初始状态：第 1 轮准备阶段
    m_phase = Phase::Preparation;
    m_round = 1;

    // 创建战斗特效叠加层并附加到棋盘场景
    m_effectOverlay = new BattleEffectOverlay();
    if (m_board && m_board->scene())
        m_board->scene()->addItem(m_effectOverlay);
}

Game::~Game()
{
    // 清理装备池（heap 分配的 Equipment 实例）
    qDeleteAll(m_equipmentPool);
    m_equipmentPool.clear();

    // 清理战斗特效叠加层（若未被 scene 接管则手动删除）
    if (m_effectOverlay && m_effectOverlay->scene() == nullptr)
        delete m_effectOverlay;
    // 若已加入 scene，则由 scene 的析构负责删除（通过 QGraphicsScene::addItem 的所有权）
}

// ============================================================================
// 游戏主循环
// ============================================================================

void Game::startLoop()
{
    m_timer->start(FRAME_INTERVAL_MS);
}

void Game::pauseLoop()
{
    m_timer->stop();
}

void Game::resumeLoop()
{
    // 仅在非结算阶段恢复循环
    if (m_phase != Phase::Resolution)
        m_timer->start(FRAME_INTERVAL_MS);
}

// ============================================================================
// 阶段管理
// ============================================================================

void Game::startBattle()
{
    // 只能在准备阶段开始战斗
    if (m_phase != Phase::Preparation)
        return;

    // —— 填充战斗系统：将所有部署在棋盘上的单位加入 ——
    m_battleSystem.clear();
    for (Unit *u : m_units) {
        if (isUnitDeployed(u))
            m_battleSystem.addUnit(u);
    }

    // —— 初始化战斗状态 ——
    m_battleSystem.startBattle();

    // 重置特效跟踪
    m_deathEffectSpawned.clear();
    m_prevHp.clear();

    // 切换阶段
    m_phase = Phase::Combat;
    emit phaseChanged(m_phase);

    // 锁定棋盘：战斗阶段禁止拖拽单位
    UnitItem::setDragLock(true);

    // —— 应用羁绊加成（仅对我方单位） ——
    auto alivePlayerVec = m_battleSystem.aliveUnitsByOwner(Owner::PlayerCtrl);
    std::vector<Unit *> playerUnits(alivePlayerVec.begin(), alivePlayerVec.end());
    m_synergySystem.applyBuffs(playerUnits);
}

// ============================================================================
// 每帧更新
// ============================================================================

void Game::update()
{
    switch (m_phase) {
    case Phase::Preparation:
        // 准备阶段不执行战斗逻辑
        // 玩家手动操作（拖拽布阵、商店购买等）
        break;

    case Phase::Combat:
        // 委托 BattleSystem 推进一帧战斗
        m_battleSystem.updateLoop();

        // 同步所有参战单位的视觉位置 + 棋盘占用（寻路移动后位置发生变化）
        for (Unit *u : m_battleSystem.allUnits()) {
            UnitItem *item = findUnitItem(u);
            if (!item)
                continue;

            QPoint newPos = u->position();
            int oldRow = item->boardRow();
            int oldCol = item->boardCol();
            int newRow = newPos.y();
            int newCol = newPos.x();

            // 位置发生变化 → 更新棋盘占用表 + 视觉坐标
            if ((oldRow != newRow || oldCol != newCol)
                && newRow >= 0 && newCol >= 0)
            {
                // 防御：目标格已被其他单位占据 → 碰撞！回退单位位置
                UnitItem *existing = m_board->unitItemAt(newRow, newCol);
                if (existing && existing != item) {
                    // 回退到旧位置（BattleSystem 下帧会重新索敌）
                    u->setPosition(QPoint(oldCol, oldRow));
                    item->setBoardPos(oldRow, oldCol);
                } else {
                    // 释放旧格子占用
                    if (oldRow >= 0 && oldCol >= 0)
                        m_board->freeCell(oldRow, oldCol);
                    // 注册新格子占用
                    m_board->occupyCell(newRow, newCol, item);
                }
            }

            // 同步视觉坐标（从 Unit 位置 → UnitItem 像素位置）
            item->syncVisualPosition();

            // —— 攻击闪光检测 ——
            if (u->justAttacked()) {
                item->triggerAttackFlash();
                u->clearJustAttacked();

                // 攻击弹道特效
                if (u->currentTarget() && u->currentTarget()->isAlive()) {
                    QPointF fromPx = UnitItem::boardToPixel(u->position().y(),
                                                             u->position().x());
                    QPointF toPx   = UnitItem::boardToPixel(
                        u->currentTarget()->position().y(),
                        u->currentTarget()->position().x());
                    bool isRanged = (u->range() > 1);
                    m_effectOverlay->spawnAttackProjectile(fromPx, toPx, isRanged);
                }
            }

            // —— 技能施放检测 ——
            if (u->justCasted()) {
                u->clearJustCasted();
                QPointF casterPx = UnitItem::boardToPixel(
                    u->position().y(), u->position().x());

                if (u->skill()) {
                    SkillTargetType st = u->skill()->targetType();
                    const auto &targets = u->lastSkillTargets();

                    if (st == SkillTargetType::AOEEnemy) {
                        // 箭雨：加载射手特效贴图（箭矢飞行）
                        QPixmap effectImg = ResourceManager::instance().skillEffectImage(u->name());

                        // 向每个目标发射弹道 + 特效飞弹
                        for (Unit *t : targets) {
                            if (!t->isAlive()) continue;
                            QPointF targetPx = UnitItem::boardToPixel(
                                t->position().y(), t->position().x());
                            // 粒子弹道
                            m_effectOverlay->spawnSkillProjectile(
                                casterPx, targetPx, QColor(100, 220, 80));
                            // 特效贴图飞弹（箭矢图像）
                            if (!effectImg.isNull()) {
                                m_effectOverlay->spawnSkillEffectProjectile(
                                    casterPx, targetPx, effectImg);
                            }
                        }
                        // AOE 扩散环（以主目标为中心）
                        if (!targets.empty() && targets[0]->isAlive()) {
                            QPointF centerPx = UnitItem::boardToPixel(
                                targets[0]->position().y(), targets[0]->position().x());
                            m_effectOverlay->spawnSkillAOE(
                                centerPx, u->skill()->aoeRadius(), QColor(100, 220, 80));
                        }
                    } else if (st == SkillTargetType::SingleEnemy) {
                        // 重击：单体弹道射向目标
                        for (Unit *t : targets) {
                            if (!t->isAlive()) continue;
                            QPointF targetPx = UnitItem::boardToPixel(
                                t->position().y(), t->position().x());
                            m_effectOverlay->spawnSkillProjectile(
                                casterPx, targetPx, QColor(255, 200, 50));
                        }
                        // 命中闪光
                        m_effectOverlay->spawnSkillAOE(
                            casterPx, 1, QColor(255, 200, 50));
                    } else if (st == SkillTargetType::HealFriendly) {
                        // 铁壁：自愈光效（治疗检测也会触发 heal 特效，这里是施法瞬间光效）
                        m_effectOverlay->spawnSkillAOE(
                            casterPx, 1, QColor(80, 180, 220));
                    } else {
                        // 回退：通用 AOE
                        m_effectOverlay->spawnSkillAOE(
                            casterPx, 2, QColor(255, 150, 50));
                    }
                } else {
                    // 无技能绑定：回退 AOE
                    m_effectOverlay->spawnSkillAOE(
                        casterPx, 2, QColor(255, 150, 50));
                }
            }

            // —— 治疗检测（HP 增加） ——
            int prevHp = m_prevHp.value(u, u->hp());
            if (u->hp() > prevHp) {
                QPointF healPx = UnitItem::boardToPixel(
                    u->position().y(), u->position().x());
                m_effectOverlay->spawnHealEffect(healPx);
            }
            m_prevHp[u] = u->hp();

            // —— 死亡检测 ——
            if (!u->isAlive() && !m_deathEffectSpawned.contains(u)) {
                QPointF deathPx = UnitItem::boardToPixel(
                    u->position().y(), u->position().x());
                m_effectOverlay->spawnDeathEffect(deathPx);
                m_deathEffectSpawned.insert(u);
            }

            // 刷新外观
            item->refreshAppearance();
        }

        // 更新特效叠加层
        m_effectOverlay->updateEffects();

        // 战斗结束 → 进入结算
        if (m_battleSystem.isCombatOver())
            resolveBattle();
        break;

    case Phase::Resolution:
        // 结算阶段等待 resolveBattle() 完成后自动进入下一轮
        // 此处不执行任何逻辑，避免重复结算
        break;
    }
}

// ============================================================================
// 结算阶段
// ============================================================================

void Game::resolveBattle()
{
    m_phase = Phase::Resolution;
    emit phaseChanged(m_phase);

    // 判断胜负：由 BattleSystem 查询己方是否还有存活单位
    bool playerWin = m_battleSystem.playerWon();

    emit battleEnded(playerWin);

    // —— 清除羁绊加成（恢复所有单位的原始属性） ——
    m_synergySystem.clearBuffs();

    if (!playerWin) {
        // 我方全灭：扣除玩家血量
        damagePlayer(10);
        // 回合结算：记录连败 + 计算利息（利息根据当前金币计算）
        m_player.onRoundEnd(false);
        emit goldChanged(m_player.gold());
    } else {
        // 我方胜利：获得金币
        m_player.addGold(8);
        // 回合结算：记录连胜 + 计算利息（基于包含胜利奖励的金币）
        m_player.onRoundEnd(true);
        emit goldChanged(m_player.gold());

        // 胜利后有概率掉落装备
        rollEquipmentDrop();
    }

    // 检查玩家是否死亡
    if (!m_player.isAlive()) {
        emit gameOver();
        m_timer->stop();
        return;
    }

    // 检查是否已完成全部回合（通关）
    if (m_round >= MAX_ROUNDS && playerWin) {
        emit gameWon();
        m_timer->stop();
        return;
    }

    // 结算完成 → 进入下一轮准备阶段
    startNewRound();
}

// ============================================================================
// 新一轮
// ============================================================================

void Game::startNewRound()
{
    ++m_round;

    // ========== 1. 清除所有敌方单位 ==========
    for (int i = m_units.size() - 1; i >= 0; --i) {
        if (m_units[i]->owner() == Owner::EnemyCtrl) {
            QPoint pos = m_units[i]->position();
            m_board->freeCell(pos.y(), pos.x());

            if (m_board->scene())
                m_board->scene()->removeItem(m_items[i]);

            delete m_items[i];
            delete m_units[i];

            m_units.removeAt(i);
            m_items.removeAt(i);
        }
    }

    // ========== 2. 我方单位：复活 + 回满血 + 清除战斗状态 ==========
    for (int i = 0; i < m_units.size(); ++i) {
        Unit *u = m_units[i];
        if (u->owner() != Owner::PlayerCtrl)
            continue;

        // 复活阵亡单位
        if (!u->isAlive())
            u->reviveAfterBattle();

        // 回满血
        int missingHp = u->maxHp() - u->hp();
        if (missingHp > 0)
            u->heal(missingHp);

        // 清除战斗状态（路径、目标、计时器、FSM）
        u->clearPath();
        u->setTarget(nullptr);
        u->setMoveTimer(0);
        if (u->state() != UnitState::Idle)
            u->changeState(UnitState::Idle);
    }

    // ========== 3. 我方单位强制归位到玩家半场 ==========
    // 先收集玩家半场所有空位
    QVector<QPoint> freePlayerCells;
    for (int row = PLAYER_HALF_START; row <= PLAYER_HALF_END; ++row)
        for (int col = 0; col < BOARD_COLS; ++col)
            if (!m_board->isCellOccupied(row, col))
                freePlayerCells.append(QPoint(col, row));

    int freeIdx = 0;
    for (int i = 0; i < m_units.size(); ++i) {
        Unit *u = m_units[i];
        if (u->owner() != Owner::PlayerCtrl)
            continue;

        // 只处理棋盘上已部署的单位，备战区里的不动
        if (!isUnitDeployed(u))
            continue;

        QPoint pos = u->position();
        int oldRow = pos.y();
        int oldCol = pos.x();

        // 已在玩家半场 → 不动（但要确保占用表正确）
        if (oldRow >= PLAYER_HALF_START && oldRow <= PLAYER_HALF_END
            && oldCol >= 0 && oldCol < BOARD_COLS)
        {
            // 修复可能的占用表不同步：确权该格子
            if (!m_board->isCellOccupied(oldRow, oldCol)) {
                m_board->occupyCell(oldRow, oldCol, m_items[i]);
            }
            continue;
        }

        // 先释放旧位置（如果不在棋盘范围内则跳过）
        if (oldRow >= 0 && oldRow < BOARD_ROWS && oldCol >= 0 && oldCol < BOARD_COLS)
            m_board->freeCell(oldRow, oldCol);

        // 从预先收集的空位表中取一个
        if (freeIdx < freePlayerCells.size()) {
            QPoint dst = freePlayerCells[freeIdx++];
            u->setPosition(dst);
            m_board->occupyCell(dst.y(), dst.x(), m_items[i]);
            m_items[i]->setBoardPos(dst.y(), dst.x());
        } else {
            // 极端情况：玩家半场全满，强行放到最后一行
            int fallbackCol = qBound(0, oldCol, BOARD_COLS - 1);
            u->setPosition(QPoint(fallbackCol, PLAYER_HALF_END));
            // 如果该格被占用，先腾出来（不会丢失单位，因为半场全满时所有格都被我方占满）
            if (m_board->isCellOccupied(PLAYER_HALF_END, fallbackCol))
                m_board->freeCell(PLAYER_HALF_END, fallbackCol);
            m_board->occupyCell(PLAYER_HALF_END, fallbackCol, m_items[i]);
            m_items[i]->setBoardPos(PLAYER_HALF_END, fallbackCol);
        }
    }

    // ========== 4. 切换到准备阶段 ==========
    m_phase = Phase::Preparation;
    emit phaseChanged(m_phase);
    UnitItem::setDragLock(false);

    // ========== 5. 刷新商店、处理升星 ==========
    m_shop.refreshShop(m_round);
    processMerges();

    // ========== 6. 最后生成新一轮敌方（此时占用表完全正确） ==========
    spawnEnemyWave();
}

// ============================================================================
// 敌方轮次生成
// ============================================================================

void Game::spawnEnemyWave()
{
    // —— 1. 获取本轮的敌方生成配置 ——
    QVector<EnemySpawner::SpawnEntry> entries = m_enemySpawner.generateWave(m_round);

    // —— 2. 为每个配置创建 Unit + UnitItem 并部署 ——
    for (const auto &entry : entries) {
        // 在敌方半场寻找一个随机空位
        QPoint cell = findFreeEnemyCell();
        if (cell.x() < 0)
            break;          // 敌方半场已满，停止生成

        int col = cell.x();
        int row = cell.y();

        // 创建逻辑单位
        Unit *unit = new Unit(
            entry.name,
            Owner::EnemyCtrl,
            QPoint(col, row),
            entry.hp,
            entry.atk,
            entry.range,
            entry.maxMana
        );

        // — 根据敌方模板名分配技能 —
        Skill *enemySkill = nullptr;
        if (entry.name == QStringLiteral("敌射手"))
            enemySkill = &s_enemyArrowRain;
        else if (entry.name == QStringLiteral("敌坦克"))
            enemySkill = &s_enemyIronWall;
        else if (entry.name == QStringLiteral("敌战士"))
            enemySkill = &s_enemyPowerStrike;
        if (enemySkill)
            unit->setSkill(enemySkill);

        // 创建视图图元
        UnitItem *item = new UnitItem(unit);
        item->setGame(this);

        // 放置到棋盘
        m_board->placeUnitItem(item, row, col);

        // 设置备战区引用（拖拽判定用；敌方单位不会被拖拽但保持统一）
        if (m_benchWidget)
            item->setBenchWidget(m_benchWidget);

        // 注册到 Game
        addUnit(unit, item);
    }
}

// ============================================================================
// 商店购买
// ============================================================================

bool Game::buyFromShop(int slotIndex)
{
    // —— 1. 校验：仅准备阶段可购买 ——
    if (m_phase != Phase::Preparation)
        return false;

    // —— 2. 校验备战区和棋盘已绑定 ——
    if (!m_bench || !m_benchWidget)
        return false;

    // —— 3. 委托 Shop 执行购买（扣金 + 创建 Unit + 放入备战区） ——
    Unit *unit = m_shop.buyUnit(slotIndex, m_player, *m_bench);
    if (!unit)
        return false;

    // —— 3b. 根据英雄名称分配技能 ——
    Skill *skill = skillForUnitName(unit->name());
    if (skill) unit->setSkill(skill);

    // —— 4. 创建视图图元 ——
    auto *item = new UnitItem(unit);
    item->setGame(this);

    // —— 5. 放置到备战区视图层 ——
    int benchSlot = m_bench->indexOf(unit);
    if (benchSlot >= 0) {
        m_benchWidget->setUnitItem(benchSlot, item);
        unit->setPosition(QPoint(-1, -1));  // 标记为备战区（非棋盘）
    }

    // —— 6. 设置棋盘与备战区引用（跨控件拖拽落点检测用） ——
    item->setBoard(m_board);
    item->setBenchWidget(m_benchWidget);
    item->setSellZone(m_sellZone);

    // —— 7. 注册到 Game ——
    addUnit(unit, item);

    // —— 8. 通知金币变化 ——
    emit goldChanged(m_player.gold());

    // —— 9. 购买后自动检测升星合成 ——
    processMerges();

    return true;
}

// ============================================================================
// 出售备战区单位
// ============================================================================

bool Game::sellFromBench(int slot)
{
    if (m_phase != Phase::Preparation)
        return false;
    if (!m_bench || !m_benchWidget)
        return false;
    if (slot < 0 || slot >= Bench::CAPACITY)
        return false;

    Unit *unit = m_bench->getUnit(slot);
    if (!unit)
        return false;

    // 查找出售价格（基础费用 × 星级倍率：2★=×3, 3★=×9）
    int baseCost = Shop::heroCost(unit->name());
    if (baseCost <= 0)
        return false;
    int sellPrice = baseCost * sellPriceMultiplier(unit->starLevel());

    // 清理 UnitItem
    UnitItem *item = findUnitItem(unit);
    if (item) {
        if (item->scene())
            item->scene()->removeItem(item);
        int idx = m_items.indexOf(item);
        if (idx >= 0) {
            m_items.removeAt(idx);
            m_units.removeAt(idx);
        }
        delete item;
    }

    // 卸下装备并归还装备池（必须在 delete unit 之前）
    if (unit->equipment()) {
        Equipment *eq = removeEquipment(unit);
        if (eq)
            addEquipmentToPool(eq);
    }

    // 从备战区移除
    m_bench->removeUnit(slot);
    m_benchWidget->takeUnitItem(slot);

    delete unit;

    // 返还金币
    m_player.addGold(sellPrice);
    emit goldChanged(m_player.gold());

    return true;
}

bool Game::sellDeployedUnit(Unit *unit)
{
    if (m_phase != Phase::Preparation)
        return false;
    if (!unit || unit->owner() != Owner::PlayerCtrl)
        return false;
    if (!isUnitDeployed(unit))
        return false;

    int baseCost = Shop::heroCost(unit->name());
    if (baseCost <= 0)
        return false;
    int sellPrice = baseCost * sellPriceMultiplier(unit->starLevel());

    UnitItem *item = findUnitItem(unit);
    if (!item)
        return false;

    // 释放棋盘占用
    QPoint pos = unit->position();
    if (pos.x() >= 0 && pos.y() >= 0)
        m_board->freeCell(pos.y(), pos.x());

    // 从场景中移除
    if (item->scene())
        item->scene()->removeItem(item);

    // 卸下装备并归还装备池（必须在 delete unit 之前）
    if (unit->equipment()) {
        Equipment *eq = removeEquipment(unit);
        if (eq)
            addEquipmentToPool(eq);
    }

    // 从注册表中移除
    int idx = m_units.indexOf(unit);
    if (idx >= 0) {
        m_items.removeAt(idx);
        m_units.removeAt(idx);
    }

    delete item;
    delete unit;

    m_player.addGold(sellPrice);
    emit goldChanged(m_player.gold());
    return true;
}

// ============================================================================
// 技能分配
// ============================================================================

Skill *Game::skillForUnitName(const QString &name)
{
    // 战士类 — 重击
    if (name == QStringLiteral("战士"))   return &s_enemyPowerStrike;
    if (name == QStringLiteral("剑术师")) return &s_enemyPowerStrike;
    if (name == QStringLiteral("狂战士")) return &s_enemyPowerStrike;
    // 射手类 — 箭雨
    if (name == QStringLiteral("神射手")) return &s_enemyArrowRain;
    if (name == QStringLiteral("鹰眼"))   return &s_enemyArrowRain;
    // 坦克类 — 铁壁
    if (name == QStringLiteral("重装兵")) return &s_enemyIronWall;
    if (name == QStringLiteral("守护者")) return &s_enemyIronWall;
    return nullptr;
}

int Game::sellPriceMultiplier(int starLevel)
{
    switch (starLevel) {
    case 1:  return 1;
    case 2:  return 3;
    case 3:  return 9;
    default: return 1;
    }
}

// ============================================================================
// 自动升星合成
// ============================================================================

void Game::processMerges()
{
    // 循环直到无更多合成（处理级联合成：1★→2★→3★）
    while (true) {
        // —— 1. 收集所有我方存活单位 ——
        std::vector<Unit *> playerUnits;
        for (Unit *u : m_units) {
            if (u->owner() == Owner::PlayerCtrl && u->isAlive())
                playerUnits.push_back(u);
        }

        // —— 2. 检测可合成候选 ——
        std::vector<MergeCandidate> merges = m_starSystem.findMerges(playerUnits);
        if (merges.empty())
            return;

        // —— 3. 逐一执行合成 ——
        for (const auto &mc : merges) {
        // 3a. 创建升级后的新 Unit
        Unit *newUnit = m_starSystem.executeMerge(mc);

        // 3b. 记录旧单位中是否有棋盘上的（用于决定合成后放置位置）
        std::array<Unit *, 3> oldUnits = { mc.unit1, mc.unit2, mc.unit3 };
        QPoint boardPos(-1, -1);
        for (Unit *old : oldUnits) {
            if (!old) continue;
            QPoint p = old->position();
            if (p.x() >= 0 && p.y() >= 0 && m_board && m_board->isCellOccupied(p.y(), p.x())) {
                boardPos = p;
                break;
            }
        }

        // 3c. 删除旧的 3 个单位
        for (Unit *old : oldUnits) {
            if (!old)
                continue;

            UnitItem *item = findUnitItem(old);
            if (!item)
                continue;

            // 通过 bench 查成员关系判断是否在备战区（比 position 更可靠）
            int benchSlot = (m_bench) ? m_bench->indexOf(old) : -1;
            bool isOnBench = (benchSlot >= 0);

            if (isOnBench) {
                m_bench->removeUnit(benchSlot);
                if (m_benchWidget)
                    m_benchWidget->takeUnitItem(benchSlot);
                if (item->scene())
                    item->scene()->removeItem(item);
            } else {
                QPoint pos = old->position();
                if (pos.x() >= 0 && pos.y() >= 0) {
                    m_board->freeCell(pos.y(), pos.x());
                    if (m_board->scene())
                        m_board->scene()->removeItem(item);
                } else {
                    if (item->scene())
                        item->scene()->removeItem(item);
                }
            }

            int idx = m_units.indexOf(old);
            if (idx >= 0) {
                m_items.removeAt(idx);
                m_units.removeAt(idx);
            }

            // 卸下旧单位装备并归还装备池
            if (old->equipment()) {
                Equipment *eq = removeEquipment(old);
                if (eq)
                    addEquipmentToPool(eq);
            }

            delete item;
            delete old;
        }

        // 3d. 创建新单位视图
        auto *newItem = new UnitItem(newUnit);
        newItem->setGame(this);
        newItem->setBoard(m_board);
        newItem->setBenchWidget(m_benchWidget);
        newItem->setSellZone(m_sellZone);

        // 3e. 放置新单位：有棋盘原位→棋盘；否则→备战区最左空位
        bool placed = false;
        if (boardPos.x() >= 0 && boardPos.y() >= 0
            && !m_board->isCellOccupied(boardPos.y(), boardPos.x())) {
            m_board->placeUnitItem(newItem, boardPos.y(), boardPos.x());
            placed = true;
        }

        if (!placed) {
            int slot = m_bench->firstEmptySlot();
            if (slot >= 0 && m_benchWidget) {
                m_bench->setUnit(slot, newUnit);
                m_benchWidget->setUnitItem(slot, newItem);
                newUnit->setPosition(QPoint(-1, -1));
                placed = true;
            }
        }

        if (!placed) {
            // 备战区满 → 放棋盘玩家半场空位
            for (int row = PLAYER_HALF_START; row <= PLAYER_HALF_END && !placed; ++row) {
                for (int col = 0; col < BOARD_COLS && !placed; ++col) {
                    if (!m_board->isCellOccupied(row, col)) {
                        m_board->placeUnitItem(newItem, row, col);
                        placed = true;
                    }
                }
            }
        }

        if (!placed) {
            delete newItem;
            delete newUnit;
            continue;
        }

        // 3f. 注册 + 分配技能
        Skill *sk = skillForUnitName(newUnit->name());
        if (sk) newUnit->setSkill(sk);
        addUnit(newUnit, newItem);
    }
    } // while (true) — 循环直到无更多合成
}

// ============================================================================
// 拖拽合并（Drag-to-Combine）
// ============================================================================

bool Game::triggerDragMerge(Unit *a, Unit *b)
{
    if (!a || !b)
        return false;

    // —— 前置校验：同名同星级 ——
    if (a->name() != b->name())
        return false;
    if (a->starLevel() != b->starLevel())
        return false;

    // —— 寻找第三同名同星单位 ——
    Unit *third = nullptr;
    for (Unit *u : m_units) {
        if (u == a || u == b)
            continue;
        if (!u->isAlive() || u->owner() != Owner::PlayerCtrl)
            continue;
        if (u->name() == a->name() && u->starLevel() == a->starLevel()) {
            third = u;
            break;
        }
    }

    if (!third)
        return false;   // 没有第三单位，无法合并

    // —— 构建合并候选并执行合成 ——
    MergeCandidate mc;
    mc.unit1      = a;
    mc.unit2      = b;
    mc.unit3      = third;
    mc.name       = a->name();
    mc.currentStar = a->starLevel();
    mc.targetStar  = mc.currentStar + 1;

    Unit *newUnit = m_starSystem.executeMerge(mc);

    // 记录放置位置（优先放到第一个单位所在处）
    QPoint firstPos = a->position();

    // 删除旧 3 单位（复用 processMerges 中的清理逻辑）
    std::array<Unit *, 3> oldUnits = { a, b, third };
    for (Unit *old : oldUnits) {
        if (!old) continue;
        UnitItem *item = findUnitItem(old);
        if (!item) continue;

        QPoint pos = old->position();
        if (pos.x() >= 0 && pos.y() >= 0) {
            m_board->freeCell(pos.y(), pos.x());
            if (m_board->scene())
                m_board->scene()->removeItem(item);
        } else {
            int slot = m_bench->indexOf(old);
            if (slot >= 0) {
                m_bench->removeUnit(slot);
                if (m_benchWidget)
                    m_benchWidget->takeUnitItem(slot);
            }
            if (item->scene())
                item->scene()->removeItem(item);
        }

        int idx = m_units.indexOf(old);
        if (idx >= 0) {
            m_items.removeAt(idx);
            m_units.removeAt(idx);
        }

        // 卸下旧单位装备并归还装备池
        if (old->equipment()) {
            Equipment *eq = removeEquipment(old);
            if (eq)
                addEquipmentToPool(eq);
        }

        delete item;
        delete old;
    }

    // 放置新单位
    auto *newItem = new UnitItem(newUnit);
    newItem->setGame(this);
    newItem->setBoard(m_board);
    newItem->setBenchWidget(m_benchWidget);
    newItem->setSellZone(m_sellZone);

    bool placed = false;
    if (firstPos.x() >= 0 && firstPos.y() >= 0) {
        m_board->placeUnitItem(newItem, firstPos.y(), firstPos.x());
        placed = true;
    } else {
        int slot = m_bench->firstEmptySlot();
        if (slot >= 0 && m_benchWidget) {
            m_bench->setUnit(slot, newUnit);
            m_benchWidget->setUnitItem(slot, newItem);
            placed = true;
        }
    }

    if (!placed) {
        // 兜底：放玩家半场空位
        for (int row = PLAYER_HALF_START; row <= PLAYER_HALF_END && !placed; ++row) {
            for (int col = 0; col < BOARD_COLS && !placed; ++col) {
                if (!m_board->isCellOccupied(row, col)) {
                    m_board->placeUnitItem(newItem, row, col);
                    placed = true;
                }
            }
        }
    }

    if (!placed) {
        delete newItem;
        delete newUnit;
        return false;
    }
    Skill *sk = skillForUnitName(newUnit->name());
    if (sk) newUnit->setSkill(sk);
    addUnit(newUnit, newItem);
    newItem->triggerMergeFlash();     // 合成闪光
    return true;
}

// ============================================================================
// 商店刷新
// ============================================================================

bool Game::refreshShop()
{
    // 校验金币
    if (m_player.gold() < Shop::REFRESH_COST)
        return false;

    m_player.spendGold(Shop::REFRESH_COST);
    m_shop.refreshShop(m_round);
    emit goldChanged(m_player.gold());
    return true;
}

// ============================================================================
// 存档 / 读档
// ============================================================================

SaveData Game::collectSaveData() const
{
    SaveData data;
    data.round = m_round;

    // —— 玩家状态 ——
    data.playerHp        = m_player.hp();
    data.playerMaxHp     = m_player.maxHp();
    data.playerGold      = m_player.gold();
    data.playerLevel     = m_player.level();
    data.playerExp       = m_player.exp();
    data.playerPopCap    = m_player.populationCap();
    data.winStreak       = m_player.winStreak();
    data.lossStreak      = m_player.lossStreak();

    // —— 我方单位 ——
    for (const Unit *u : m_units) {
        if (u->owner() != Owner::PlayerCtrl)
            continue;

        SaveData::UnitInfo info;
        info.name        = u->name();
        info.owner       = static_cast<int>(u->owner());
        info.hp          = u->hp();
        info.maxHp       = u->maxHp();
        info.atk         = u->atk();
        info.range       = u->range();
        info.maxMana     = u->maxMana();
        info.attackSpeed = u->attackSpeed();
        info.starLevel   = u->starLevel();

        QPoint pos = u->position();
        info.boardCol = pos.x();
        info.boardRow = pos.y();

        // 羁绊标签
        for (Trait t : u->traits()) {
            QString ts = SaveLoadManager::traitToString(t);
            if (!ts.isEmpty())
                info.traits.append(ts);
        }

        // 装备
        if (u->equipment())
            info.equipment = u->equipment()->typeName();

        data.units.append(info);
    }

    // —— 商店 ——
    data.shopSlots.clear();
    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        const HeroTemplate *tmpl = m_shop.slot(i);
        data.shopSlots.append(tmpl ? tmpl->name : QString());
    }

    return data;
}

void Game::applySaveData(const SaveData &data)
{
    // —— 1. 清除当前所有我方单位 ——
    // 从后往前遍历，避免索引错位
    for (int i = m_units.size() - 1; i >= 0; --i) {
        if (m_units[i]->owner() != Owner::PlayerCtrl)
            continue;

        Unit *u   = m_units[i];
        UnitItem *item = m_items[i];

        // 从棋盘释放（如果在棋盘上）
        QPoint pos = u->position();
        if (pos.x() >= 0 && pos.y() >= 0 && m_board)
            m_board->freeCell(pos.y(), pos.x());

        // 从备战区移除（如果在备战区）
        if (m_bench) {
            int slot = m_bench->indexOf(u);
            if (slot >= 0) {
                m_bench->removeUnit(slot);
                if (m_benchWidget)
                    m_benchWidget->takeUnitItem(slot);
            }
        }

        // 从场景移除
        if (item && item->scene())
            item->scene()->removeItem(item);

        delete item;
        delete u;

        m_units.removeAt(i);
        m_items.removeAt(i);
    }

    // —— 2. 恢复玩家状态 ——
    m_player.restoreState(
        data.playerHp, data.playerMaxHp,
        data.playerGold, data.playerLevel, data.playerExp,
        data.playerPopCap, data.winStreak, data.lossStreak
    );
    emit playerHpChanged(m_player.hp(), m_player.maxHp());
    emit goldChanged(m_player.gold());

    // —— 3. 恢复轮次 ——
    m_round = data.round;

    // —— 4. 重建我方单位 ——
    for (const auto &info : data.units) {
        // 4a. 创建逻辑单位
        auto *unit = new Unit(
            info.name,
            static_cast<Owner>(info.owner),
            QPoint(info.boardCol, info.boardRow),
            info.maxHp, info.atk, info.range, info.maxMana,
            info.attackSpeed
        );
        unit->setStarLevel(info.starLevel);
        unit->setRawHp(info.hp);

        // 4b. 恢复羁绊标签
        for (const QString &ts : info.traits) {
            Trait t = SaveLoadManager::stringToTrait(ts);
            if (t != Trait::None)
                unit->addTrait(t);
        }

        // 4c. 创建视图图元
        auto *item = new UnitItem(unit);
        item->setGame(this);
        item->setBoard(m_board);
        item->setBenchWidget(m_benchWidget);
        item->setSellZone(m_sellZone);

        // 4d. 放置到正确位置
        if (info.boardCol >= 0 && info.boardRow >= 0) {
            // 棋盘上
            if (m_board)
                m_board->placeUnitItem(item, info.boardRow, info.boardCol);
        } else {
            // 备战区
            if (m_bench && m_benchWidget) {
                int slot = m_bench->firstEmptySlot();
                if (slot >= 0) {
                    m_bench->setUnit(slot, unit);
                    m_benchWidget->setUnitItem(slot, item);
                } else {
                    // 备战区满 → 放棋盘空位
                    if (m_board)
                        m_board->placeUnitItem(item, PLAYER_HALF_START, 0);
                }
            }
        }

        // 4e. 恢复装备
        if (!info.equipment.isEmpty()) {
            Equipment *eq = nullptr;
            if      (info.equipment == QStringLiteral("Sword"))   eq = &s_sword;
            else if (info.equipment == QStringLiteral("Armor"))   eq = &s_armor;
            else if (info.equipment == QStringLiteral("Glove"))   eq = &s_glove;
            else if (info.equipment == QStringLiteral("Crystal")) eq = &s_crystal;

            if (eq)
                equipItem(unit, eq);
        }

        // 4f. 注册到 Game
        addUnit(unit, item);
    }

    // —— 5. 恢复商店 ——
    for (int i = 0; i < data.shopSlots.size() && i < Shop::SLOT_COUNT; ++i) {
        m_shop.restoreSlot(i, data.shopSlots[i]);
    }
}

bool Game::saveGame(const QString &filename)
{
    try {
        SaveData data = collectSaveData();
        return SaveLoadManager::saveGame(filename, data);
    } catch (const std::exception &) {
        return false;
    }
}

bool Game::loadGame(const QString &filename)
{
    try {
        SaveData data = SaveLoadManager::loadGame(filename);
        if (data.round < 1)
            return false;

        // 清除旧敌方单位
        for (int i = m_units.size() - 1; i >= 0; --i) {
            if (m_units[i]->owner() == Owner::EnemyCtrl) {
                QPoint pos = m_units[i]->position();
                if (m_board)
                    m_board->freeCell(pos.y(), pos.x());
                if (m_items[i] && m_items[i]->scene())
                    m_items[i]->scene()->removeItem(m_items[i]);
                delete m_items[i];
                delete m_units[i];
                m_units.removeAt(i);
                m_items.removeAt(i);
            }
        }

        applySaveData(data);

        m_phase = Phase::Preparation;
        emit phaseChanged(m_phase);
        UnitItem::setDragLock(false);

        if (m_benchWidget)
            m_benchWidget->refreshDisplay();

        spawnEnemyWave();

        return true;
    } catch (const std::exception &) {
        return false;
    }
}

// ============================================================================
// 装备掉落
// ============================================================================

void Game::rollEquipmentDrop()
{
    // 40% 概率掉落一件装备，四种等概率随机
    if (QRandomGenerator::global()->bounded(100) >= 40)
        return;

    Equipment *eq = nullptr;
    int roll = QRandomGenerator::global()->bounded(4);
    switch (roll) {
    case 0: eq = new Sword();   break;
    case 1: eq = new Armor();   break;
    case 2: eq = new Glove();   break;
    case 3: eq = new Crystal(); break;
    }

    if (eq) {
        m_equipmentPool.append(eq);
        emit equipmentDropped(eq);
    }
}

// ============================================================================
// 寻找敌方空位
// ============================================================================

QPoint Game::findFreeEnemyCell() const
{
    // 收集敌方半场（行 0~3）中所有未被占用的格子
    QVector<QPoint> freeCells;
    for (int row = ENEMY_HALF_START; row <= ENEMY_HALF_END; ++row) {
        for (int col = 0; col < BOARD_COLS; ++col) {
            if (!m_board->isCellOccupied(row, col))
                freeCells.append(QPoint(col, row));
        }
    }

    if (freeCells.isEmpty())
        return QPoint(-1, -1);      // 没有空位

    // 随机选取一个空位
    int idx = QRandomGenerator::global()->bounded(freeCells.size());
    return freeCells[idx];
}

// ============================================================================
// 单位管理
// ============================================================================

void Game::addUnit(Unit *unit, UnitItem *item)
{
    if (!unit || !item)
        return;

    // 避免重复注册
    if (m_units.contains(unit))
        return;

    m_units.append(unit);
    m_items.append(item);
}

QVector<Unit *> Game::aliveUnits() const
{
    QVector<Unit *> result;
    for (Unit *u : m_units) {
        if (u->isAlive())
            result.append(u);
    }
    return result;
}

QVector<Unit *> Game::aliveUnitsByOwner(Owner owner) const
{
    QVector<Unit *> result;
    for (Unit *u : m_units) {
        if (u->isAlive() && u->owner() == owner)
            result.append(u);
    }
    return result;
}

// ============================================================================
// 备战区管理
// ============================================================================

void Game::setBench(Bench *bench, BenchWidget *benchWidget)
{
    m_bench       = bench;
    m_benchWidget = benchWidget;
}

bool Game::deployFromBench(int benchSlot, int targetRow, int targetCol)
{
    // —— 1. 校验：仅准备阶段可操作 ——
    if (m_phase != Phase::Preparation)
        return false;

    // —— 2. 校验：备战区已绑定 ——
    if (!m_bench || !m_benchWidget || !m_board)
        return false;

    // —— 3. 从备战区逻辑层取出单位 ——
    Unit *unit = m_bench->getUnit(benchSlot);
    if (!unit)
        return false;               // 槽位为空

    // —— 4. 校验目标格合法性 ——
    // 必须在玩家半场（行 4~7）
    if (targetRow < PLAYER_HALF_START || targetRow > PLAYER_HALF_END)
        return false;
    if (targetCol < 0 || targetCol >= BOARD_COLS)
        return false;

    // 目标格不能被占用
    if (m_board->isCellOccupied(targetRow, targetCol))
        return false;

    // —— 4b. 人口上限校验 ——
    // 检查当前已部署单位数是否已达到人口上限
    int deployedCount = deployedPlayerUnits().size();
    if (!m_player.canDeploy(deployedCount))
        return false;

    // —— 5. 查找对应的视图图元 ——
    UnitItem *item = findUnitItem(unit);
    if (!item)
        return false;

    // —— 6. 从备战区逻辑层移除 ——
    m_bench->removeUnit(benchSlot);

    // —— 7. UnitItem 场景转移（备战区 → 棋盘） ——
    QGraphicsScene *benchScene = m_benchWidget->scene();
    if (benchScene && item->scene() == benchScene) {
        benchScene->removeItem(item);
    }
    if (item->scene() != m_board->scene()) {
        m_board->scene()->addItem(item);
    }

    // —— 8. 棋盘占用 + 定位 ——
    m_board->occupyCell(targetRow, targetCol, item);
    item->setBoardPos(targetRow, targetCol);
    item->setBoard(m_board);
    item->setDragSource(DragSource::Board);

    return true;
}

int Game::recallToBench(int boardRow, int boardCol)
{
    // —— 1. 校验：仅准备阶段可操作 ——
    if (m_phase != Phase::Preparation)
        return -1;

    // —— 2. 校验：备战区已绑定 ——
    if (!m_bench || !m_benchWidget || !m_board)
        return -1;

    // —— 3. 查找目标格上的单位 ——
    UnitItem *item = m_board->unitItemAt(boardRow, boardCol);
    if (!item)
        return -1;                  // 格子上没有单位

    Unit *unit = item->unit();
    if (!unit)
        return -1;

    // 只能回收己方单位
    if (unit->owner() != Owner::PlayerCtrl)
        return -1;

    // —— 4. 校验备战区有空位 ——
    int slot = m_bench->firstEmptySlot();
    if (slot < 0)
        return -1;                  // 备战区已满

    // —— 5. 释放棋盘占用 ——
    m_board->freeCell(boardRow, boardCol);

    // —— 6. UnitItem 场景转移（棋盘 → 备战区） ——
    if (item->scene() == m_board->scene()) {
        m_board->scene()->removeItem(item);
    }
    m_benchWidget->acceptUnitItem(item, slot);

    // —— 7. 标记单位为"未部署" ——
    unit->setPosition(QPoint(-1, -1));

    return slot;  // 返回所在备战区槽位
}

int Game::recallToBench(Unit *unit)
{
    if (!unit)
        return -1;

    // 从单位的 position 获取棋盘坐标
    QPoint pos = unit->position();
    if (pos.x() < 0 || pos.y() < 0)
        return -1;                  // 单位不在棋盘上

    return recallToBench(pos.y(), pos.x());
    // 注：position 中 x=列, y=行；recallToBench(row, col)
}

bool Game::isUnitDeployed(const Unit *unit) const
{
    if (!unit)
        return false;

    // 部署判断：单位坐标在棋盘有效范围内
    QPoint pos = unit->position();
    if (pos.x() < 0 || pos.x() >= BOARD_COLS)
        return false;
    if (pos.y() < 0 || pos.y() >= BOARD_ROWS)
        return false;

    // 进一步确认：单位确实在棋盘占用表中
    // （防止 position 脏数据导致误判）
    if (m_board && !m_board->isCellOccupied(pos.y(), pos.x()))
        return false;

    // 额外检查：不在备战区中
    if (m_bench && m_bench->indexOf(unit) >= 0)
        return false;

    return true;
}

QVector<Unit *> Game::deployedPlayerUnits() const
{
    QVector<Unit *> result;
    for (Unit *u : m_units) {
        if (u->isAlive()
            && u->owner() == Owner::PlayerCtrl
            && isUnitDeployed(u))
        {
            result.append(u);
        }
    }
    return result;
}

// ============================================================================
// 内部辅助
// ============================================================================

UnitItem *Game::findUnitItem(const Unit *unit) const
{
    if (!unit)
        return nullptr;

    // 平行数组查找：m_units[i] 对应 m_items[i]
    for (int i = 0; i < m_units.size(); ++i) {
        if (m_units[i] == unit)
            return m_items[i];
    }
    return nullptr;
}

void Game::damagePlayer(int amount)
{
    if (amount <= 0)
        return;

    m_player.takeDamage(amount);
    emit playerHpChanged(m_player.hp(), m_player.maxHp());
}

// ============================================================================
// 私有槽
// ============================================================================

void Game::onTimerTick()
{
    // 定时器每 16ms 触发一次，调用 update() 推进一帧
    update();
}
