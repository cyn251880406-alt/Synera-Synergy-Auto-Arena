#include "battle/BattleSystem.h"

#include <QtMath>        // qSqrt
#include <vector>        // std::vector (供 TargetingSystem 接口)

// ============================================================================
// 构造
// ============================================================================

BattleSystem::BattleSystem()
{
    m_combatUnits.clear();
}

// ============================================================================
// 单位管理
// ============================================================================

void BattleSystem::addUnit(Unit *unit)
{
    if (!unit)
        return;

    // 避免重复添加
    if (m_combatUnits.contains(unit))
        return;

    m_combatUnits.append(unit);
}

void BattleSystem::removeUnit(Unit *unit)
{
    if (!unit)
        return;

    m_combatUnits.removeOne(unit);
    // 不 delete——所有权在 Game 层
}

bool BattleSystem::contains(Unit *unit) const
{
    if (!unit)
        return false;
    return m_combatUnits.contains(unit);
}

void BattleSystem::clear()
{
    m_combatUnits.clear();
    // 不 delete 任何单位——所有权在 Game 层
}

// ============================================================================
// 查询
// ============================================================================

QVector<Unit *> BattleSystem::aliveUnits() const
{
    QVector<Unit *> result;
    for (Unit *u : m_combatUnits) {
        if (u->isAlive())
            result.append(u);
    }
    return result;
}

QVector<Unit *> BattleSystem::aliveUnitsByOwner(Owner owner) const
{
    QVector<Unit *> result;
    for (Unit *u : m_combatUnits) {
        if (u->isAlive() && u->owner() == owner)
            result.append(u);
    }
    return result;
}

// ============================================================================
// 战斗生命周期
// ============================================================================

void BattleSystem::startBattle()
{
    // 将所有参战单位重置为 Idle 状态（通过 FSM 入口）
    // 同时清空上轮遗留的移动路径和锁定目标
    for (Unit *u : m_combatUnits) {
        if (!u->isAlive())
            continue;
        u->clearPath();
        u->setTarget(nullptr);
        u->setMoveTimer(0);
        u->changeState(UnitState::Idle);
    }
}

void BattleSystem::updateLoop()
{
    // ——— 1. 战斗已结束则停止更新 ———
    if (isCombatOver())
        return;

    // ——— 1b. 更新所有存活单位的战斗上下文（供技能目标解析） ———
    auto playerUnitsVec = aliveUnitsByOwner(Owner::PlayerCtrl);
    auto enemyUnitsVec  = aliveUnitsByOwner(Owner::EnemyCtrl);
    std::vector<Unit *> playerUnits(playerUnitsVec.begin(), playerUnitsVec.end());
    std::vector<Unit *> enemyUnits(enemyUnitsVec.begin(), enemyUnitsVec.end());
    for (Unit *u : m_combatUnits) {
        if (!u->isAlive() || !u->skill())
            continue;
        if (u->owner() == Owner::PlayerCtrl)
            u->setCombatContext(playerUnits, enemyUnits);
        else
            u->setCombatContext(enemyUnits, playerUnits);
    }

    // ——— 2. 更新所有存活单位的内部计时（冷却递减、移动步进等） ———
    for (Unit *u : m_combatUnits) {
        if (!u->isAlive())
            continue;
        u->update();
    }

    // ——— 3. 构建当前棋盘占用表 ———
    // 被占据的格子不可通行
    bool occupied[Pathfinder::BOARD_SIZE][Pathfinder::BOARD_SIZE] = {};
    for (Unit *u : m_combatUnits) {
        if (!u->isAlive())
            continue;
        QPoint pos = u->position();
        if (pos.x() >= 0 && pos.x() < Pathfinder::BOARD_SIZE
            && pos.y() >= 0 && pos.y() < Pathfinder::BOARD_SIZE)
        {
            occupied[pos.y()][pos.x()] = true;
        }
    }

    // ——— 4. 处理 Moving 状态单位的移动 ———
    // 对正在移动的单位，调用 moveStep 推进一格
    // moveStep 内部会检测路径阻塞、进入攻击范围等
    for (Unit *u : m_combatUnits) {
        if (!u->isAlive())
            continue;

        if (u->state() == UnitState::Moving) {
            // 移动前将此单位自身从占用表中移除（不阻塞自己）
            QPoint myPos = u->position();
            if (myPos.x() >= 0 && myPos.y() >= 0)
                occupied[myPos.y()][myPos.x()] = false;

            m_pathfinder.moveStep(u, occupied);

            // 移动后重新标记占用（新位置已在 moveStep 中更新）
            QPoint newPos = u->position();
            if (newPos.x() >= 0 && newPos.y() >= 0)
                occupied[newPos.y()][newPos.x()] = true;
        }
    }

    // ——— 5. 自动索敌与攻击 ———
    // FSM 驱动：只有 Idle 状态的单位才能发起攻击
    // Attacking 单位正在冷却中；Moving 单位已在步骤 4 处理
    for (Unit *u : m_combatUnits) {
        if (!u->isAlive())
            continue;

        // 非 Idle 状态（Attacking/Moving/Casting）→ 不发起新攻击
        if (u->state() != UnitState::Idle)
            continue;

        // 寻找最近敌人
        Unit *target = findNearestEnemy(u);
        if (!target)
            continue;           // 没有存活敌人

        // 锁定目标（用于冷却期间目标死亡检测）
        u->setTarget(target);

        // 计算欧氏距离
        QPoint src = u->position();
        QPoint dst = target->position();
        int dx = src.x() - dst.x();
        int dy = src.y() - dst.y();
        double dist = qSqrt(dx * dx + dy * dy);

        // 在攻击范围内且冷却完毕 → 普攻
        if (dist <= u->range() && u->canAttack()) {
            processAttack(u, target);
        }
        // 不在范围内 → BFS 寻路，开始向目标移动
        else if (dist > u->range()) {
            // 从占用表中暂时移除此单位自身（不阻塞寻路起点）
            if (src.x() >= 0 && src.y() >= 0)
                occupied[src.y()][src.x()] = false;

            // BFS 搜索最短路径（停在攻击范围内即可，不穿过任何单位）
            std::vector<QPoint> path =
                m_pathfinder.findPath(src, dst, u->range(), occupied);

            // 恢复占用
            if (src.x() >= 0 && src.y() >= 0)
                occupied[src.y()][src.x()] = true;

            if (!path.empty()) {
                // 找到路径 → 设置路径，切换到 Moving 状态
                u->setPath(path);
                u->changeState(UnitState::Moving);
            }
            // 路径为空（target 被包围/不可达）：
            // 保持 Idle 状态，下一帧可能选择不同目标
        }
    }
}

bool BattleSystem::isCombatOver() const
{
    // 任一方存活单位数为 0 → 战斗结束
    bool hasPlayerUnit = false;
    bool hasEnemyUnit  = false;

    for (const Unit *u : m_combatUnits) {
        if (!u->isAlive())
            continue;

        if (u->owner() == Owner::PlayerCtrl)
            hasPlayerUnit = true;
        else
            hasEnemyUnit = true;

        // 双方都还有存活 → 继续战斗
        if (hasPlayerUnit && hasEnemyUnit)
            return false;
    }

    // 有一方全灭
    return true;
}

bool BattleSystem::playerWon() const
{
    // 至少有一个己方单位存活 → 玩家胜利
    return !aliveUnitsByOwner(Owner::PlayerCtrl).isEmpty();
}

// ============================================================================
// 内部辅助
// ============================================================================

Unit *BattleSystem::findNearestEnemy(Unit *unit) const
{
    if (!unit)
        return nullptr;

    // 收集所有存活的敌方单位（STL vector，供 TargetingSystem 使用）
    std::vector<Unit *> enemies;
    Owner myOwner = unit->owner();
    for (Unit *u : m_combatUnits) {
        if (u->isAlive() && u->owner() != myOwner)
            enemies.push_back(u);
    }

    // 委托索敌系统按四级优先级选择最优目标
    return m_targetingSystem.findTarget(unit, enemies);
}

void BattleSystem::processAttack(Unit *attacker, Unit *target)
{
    if (!attacker || !target)
        return;

    // 执行普攻（扣血 + 回蓝 + 重置冷却 + 设置状态）
    attacker->attack(*target);

    // 注：UI 刷新（UnitItem::refreshAppearance）由 Game 层在
    //     update() 中统一处理，BattleSystem 不持有 UnitItem 引用
}
