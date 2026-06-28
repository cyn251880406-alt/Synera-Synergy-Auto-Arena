#ifndef GAME_H
#define GAME_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPoint>

#include "unit/Unit.h"     // Owner 枚举、Unit 类声明
#include "core/Player.h"         // Player 实体类
#include "battle/EnemySpawner.h"   // 敌方轮次生成器
#include "battle/BattleSystem.h"   // 战斗系统
#include "shop/Shop.h"           // 商店系统
#include "synergy/SynergySystem.h"  // 羁绊系统
#include "synergy/StarSystem.h"     // 升星系统
#include "save/SaveLoadManager.h"// 存档系统
#include "battle/BattleEffectOverlay.h"// 战斗特效

class BoardWidget;
class UnitItem;
class Bench;
class BenchWidget;
class SellZone;

// ============================================================================
// Game — 游戏主控制器
// ============================================================================

/**
 * @brief Game — 自走棋游戏的核心管理类
 *
 * 顶层控制器：QTimer 主循环（16ms），三阶段流转（准备/战斗/结算），
 * 统一管理单位注册、棋盘-备战区转移、商店购买、羁绊激活、升星合并、存档读档。
 */
class Game : public QObject
{
    Q_OBJECT

public:
    // ========== 游戏阶段枚举 ==========

    /**
     * @brief Phase — 游戏流程的三个阶段
     *
     * Preparation → Combat → Resolution → (下一轮 Preparation)
     */
    enum class Phase {
        Preparation, ///< 准备阶段——玩家可拖拽布阵、购买单位
        Combat,       ///< 战斗阶段——AI 接管，单位自动寻路攻击
        Resolution    ///< 结算阶段——判定胜负，发放金币/装备
    };

    // ========== 构造/析构 ==========

    /**
     * @brief 构造 Game 实例
     * @param board  游戏棋盘（Game 不拥有所有权）
     * @param parent Qt 父对象（用于 QObject 生命周期管理）
     */
    explicit Game(BoardWidget *board, QObject *parent = nullptr);
    ~Game() override;

    // ========== 游戏主循环 ==========

    /**
     * @brief 启动/重启游戏主循环
     *
     * 启动一个 16ms 间隔的 QTimer，每次触发时调用 update()。
     * 通常在程序启动时调用一次即可。
     */
    void startLoop();

    /// 暂停游戏循环（例如弹出菜单时）
    void pauseLoop();

    /// 恢复游戏循环
    void resumeLoop();

    // ========== 阶段管理 ==========

    /**
     * @brief 进入战斗阶段
     *
     * 从 Preparation 切换到 Combat：
     *   - 锁定棋盘（禁止拖拽）
     *   - 所有单位由 AI 接管
     *   - 开始自动战斗
     */
    void startBattle();

    /**
     * @brief 为当前轮次自动生成敌方单位并部署到敌方半场
     *
     * 调用 EnemySpawner::generateWave() → 在敌方半场随机空位创建 Unit + UnitItem → 注册。
     */
    void spawnEnemyWave();

    /// 返回当前阶段
    Phase currentPhase() const { return m_phase; }

    /// 返回当前轮次（从第 1 轮开始）
    int roundNumber() const { return m_round; }

    // ========== 每帧更新 ==========

    /**
     * @brief 每个逻辑帧的更新入口（由 QTimer 自动调用）
     *
     * 根据当前阶段执行不同逻辑：
     *   - Preparation：不执行战斗逻辑（玩家手动操作）
     *   - Combat：更新所有单位、索敌、移动、攻击
     *   - Resolution：等待结算后进入下一轮
     *
     * 可重写以扩展阶段逻辑。
     */
    virtual void update();

    // ========== 单位管理 ==========

    /**
     * @brief 注册一个单位及其视图图元
     * @param unit 逻辑单位指针（Game 不拥有所有权）
     * @param item 视图图元指针（Game 不拥有所有权）
     *
     * 注册后，每帧 update() 会调用 unit->update()。
     * 所有权仍归 main.cpp 调用方（或由 Qt parent 管理）。
     */
    void addUnit(Unit *unit, UnitItem *item);

    /// 获取所有注册的单位
    const QVector<Unit *> &units() const { return m_units; }

    /// 获取所有注册的单位图元
    const QVector<UnitItem *> &unitItems() const { return m_items; }

    /// 获取所有存活的单位（快捷方法）
    QVector<Unit *> aliveUnits() const;

    /// 获取指定 owner 的所有存活单位
    QVector<Unit *> aliveUnitsByOwner(Owner owner) const;

    // ========== 备战区管理 ==========

    /**
     * @brief 绑定备战区（逻辑层 + 视图层）
     *
     * 调用后 Game 可通过 deployFromBench() / recallToBench()
     * 在棋盘与备战区之间转移单位。Game 不拥有 Bench 的所有权。
     *
     * @param bench       备战区逻辑层指针
     * @param benchWidget 备战区视图层指针（用于 UnitItem 场景转移）
     */
    void setBench(Bench *bench, BenchWidget *benchWidget);

    /// 返回绑定的备战区逻辑层指针
    Bench *bench() const { return m_bench; }

    /**
     * @brief 将备战区中的一个单位部署到棋盘上
     *
     * 校验准备阶段 + 玩家半场 + 未被占用，转移 UnitItem 场景并更新占用表。
     *
     * @param benchSlot 备战区槽位索引 [0, 7]
     * @param targetRow 目标棋盘行号 [4, 7]（玩家半场）
     * @param targetCol 目标棋盘列号 [0, 7]
     * @return true 部署成功
     */
    bool deployFromBench(int benchSlot, int targetRow, int targetCol);

    /**
     * @brief 将棋盘上的一个己方单位回收到备战区
     *
     * 校验准备阶段 + 己方单位 + 备战区空位，释放棋盘占用并转移 UnitItem 场景。
     *
     * @param boardRow 棋盘行号
     * @param boardCol 棋盘列号
     * @return 回收后所在的备战区槽位索引；失败返回 -1
     */
    int recallToBench(int boardRow, int boardCol);

    /**
     * @brief 将指定单位回收到备战区（重载）
     *
     * 自动查找单位当前所在的棋盘位置并回收。
     *
     * @param unit 要回收的单位指针
     * @return 回收后所在的备战区槽位索引；失败返回 -1
     */
    int recallToBench(Unit *unit);

    /**
     * @brief 判断一个单位是否部署在棋盘上（而非备战区）
     *
     * 部署在棋盘上的单位参与战斗；备战区中的单位不参与。
     * 判断依据：单位位置是否有效（非 (-1, -1) 且索引在棋盘范围内）。
     */
    bool isUnitDeployed(const Unit *unit) const;

    /// 返回所有部署在棋盘上的存活己方单位
    QVector<Unit *> deployedPlayerUnits() const;

    // ========== 玩家资源 ==========

    /// 返回 Player 实体引用（供外部直接查询完整状态）
    Player &player() { return m_player; }
    const Player &player() const { return m_player; }

    /// 当前血量（委托 Player）
    int playerHp()  const { return m_player.hp(); }
    /// 金币（委托 Player）
    int gold()      const { return m_player.gold(); }
    /// 人口上限（委托 Player）
    int maxPop()    const { return m_player.populationCap(); }

    /// 扣除玩家血量（委托 Player，扣除后 emit 信号）
    void damagePlayer(int amount);

    // ========== 战斗系统访问 ==========

    /// 返回战斗系统引用（供查询战斗状态）
    BattleSystem &battleSystem() { return m_battleSystem; }
    const BattleSystem &battleSystem() const { return m_battleSystem; }

    // ========== 商店系统访问 ==========

    /// 返回商店引用（供 UI 层读取招募位信息）
    Shop &shop() { return m_shop; }
    const Shop &shop() const { return m_shop; }

    // ========== 羁绊系统访问 ==========

    /// 返回羁绊系统引用
    SynergySystem &synergySystem() { return m_synergySystem; }
    const SynergySystem &synergySystem() const { return m_synergySystem; }

    // ========== 升星系统访问 ==========

    /// 返回升星系统引用
    StarSystem &starSystem() { return m_starSystem; }
    const StarSystem &starSystem() const { return m_starSystem; }

    // ========== 装备池访问 ==========

    /// 返回装备池引用（装备栏 UI 使用）
    const QVector<Equipment *> &equipmentPool() const { return m_equipmentPool; }

    /// 将装备从池中取出（装备到单位时调用）
    void removeEquipmentFromPool(Equipment *eq) { m_equipmentPool.removeOne(eq); }
    /// 将装备放回池中（卸下装备时调用）
    void addEquipmentToPool(Equipment *eq) { m_equipmentPool.append(eq); }

    /// 当前选中的装备（EquipWidget 点击设置，UnitItem 点击装备）
    Equipment *selectedEquipment() const { return m_selectedEquipment; }
    void setSelectedEquipment(Equipment *eq) { m_selectedEquipment = eq; }

    /// 拖拽合并：由 UnitItem 在 drop-on-occupied-cell 时调用
    /// 检测候补的第三单元并执行 StarSystem 合并
    bool triggerDragMerge(Unit *a, Unit *b);

    // ========== 商店 ==========

    /// 从商店购买单位（自动处理扣金 + Unit 创建 + UnitItem 注册 + 备战区同步）
    /// @param slotIndex 商店槽位索引 [0, 4]
    /// @return true 购买成功；false 金币不足/备战区满/槽位空
    bool buyFromShop(int slotIndex);

    /// 手动刷新商店（消耗 REFRESH_COST 金币，重新生成 5 个招募位）
    bool refreshShop();

    /// 出售备战区指定槽位的单位（返还购买时花费的金币）
    bool sellFromBench(int slot);

    /// 出售棋盘上已部署的单位（拖拽到出售区时调用）
    bool sellDeployedUnit(Unit *unit);

    /// 设置出售区域控件引用
    void setSellZone(SellZone *sz) { m_sellZone = sz; }

    // ========== 存档/读档 ==========

    /**
     * @brief 将当前游戏状态保存到 JSON 文件
     *
     * 保存内容：Player 状态、所有我方单位（棋盘+备战区）、商店。
     * 敌方单位由 EnemySpawner 重新生成，不保存。
     *
     * @param filename 保存路径（如 "save.json"）
     * @return true 保存成功
     */
    bool saveGame(const QString &filename);

    /**
     * @brief 从 JSON 文件恢复游戏状态
     *
     * 加载流程：
     *   1. 清除当前所有我方单位
     *   2. 恢复 Player 状态
     *   3. 从存档数据重建我方单位（含装备、羁绊、星级）
     *   4. 恢复商店状态
     *   5. 刷新备战区与棋盘显示
     *
     * @param filename 存档路径（如 "save.json"）
     * @return true 加载成功
     */
    bool loadGame(const QString &filename);

    // ========== 常量 ==========

    /// 游戏帧间隔（毫秒），16ms ≈ 62.5fps
    static constexpr int FRAME_INTERVAL_MS = 16;

    /// 敌方半场：行 0~3
    static constexpr int ENEMY_HALF_START = 0;
    static constexpr int ENEMY_HALF_END   = 3;
    /// 玩家半场：行 4~7
    static constexpr int PLAYER_HALF_START = 4;
    static constexpr int PLAYER_HALF_END   = 7;

    /// 游戏总回合数
    static constexpr int MAX_ROUNDS = 10;

signals:
    /// 阶段切换时发出，供 UI 更新状态文字
    void phaseChanged(Game::Phase newPhase);

    /// 战斗结束时发出（一方全灭或玩家 HP 归零）
    void battleEnded(bool playerWin);

    /// 游戏结束时发出（玩家 HP 归零，已无继续游戏的可能）
    void gameOver();

    /// 通关时发出（完成全部 10 回合）
    void gameWon();

    /// 玩家 HP 变化时发出
    void playerHpChanged(int currentHp, int maxHp);

    /// 金币变化时发出
    void goldChanged(int currentGold);

    /// 装备掉落时发出（供 UI 层显示掉落提示）
    void equipmentDropped(Equipment *equipment);

private slots:
    /// QTimer 超时槽函数，内部调用 update()
    void onTimerTick();

private:
    // ---- Qt 主循环 ----
    QTimer *m_timer = nullptr;

    // ---- 游戏状态 ----
    Phase m_phase = Phase::Preparation;  ///< 当前阶段
    int   m_round = 1;                   ///< 当前轮次

    // ---- 玩家资源 ----
    Player m_player;  ///< 玩家实体（HP/金币/等级/人口上限/连胜）

    // ---- 棋盘引用 ----
    BoardWidget *m_board = nullptr;

    // ---- 备战区引用 ----
    Bench       *m_bench       = nullptr;  ///< 备战区逻辑层（不拥有所有权）
    BenchWidget *m_benchWidget = nullptr;  ///< 备战区视图层（用于场景转移）
    SellZone    *m_sellZone    = nullptr;  ///< 出售区域控件

    // ---- 敌方生成 ----
    EnemySpawner m_enemySpawner;  ///< 敌方轮次生成器（按轮次自动生成 Er）

    // ---- 商店系统 ----
    Shop m_shop;  ///< 商店逻辑层（管理 5 个招募位）

    // ---- 羁绊系统 ----
    SynergySystem m_synergySystem;  ///< 羁绊系统（战斗时属性加成）

    // ---- 升星系统 ----
    StarSystem m_starSystem;  ///< 升星系统（3 合 1 自动合成）

    // ---- 装备池（已掉落但未穿戴的装备） ----
    QVector<Equipment *> m_equipmentPool;

    // ---- 战斗特效 ----
    BattleEffectOverlay *m_effectOverlay = nullptr;  ///< 战斗特效叠加层（加到 board scene）
    QMap<Unit*, int> m_prevHp;  ///< 上一帧各单位 HP（用于检测治疗）
    QSet<Unit*> m_deathEffectSpawned;  ///< 已生成死亡特效的单位（避免重复）
    Equipment *m_selectedEquipment = nullptr;  ///< EquipWidget 当前选中的装备

    // ---- 战斗系统 ----
    BattleSystem m_battleSystem;  ///< 战斗系统（管理索敌/攻击/死亡判定）

    // ---- 单位注册表 ----
    QVector<Unit *>     m_units;   ///< 逻辑层单位（不拥有所有权）
    QVector<UnitItem *> m_items;   ///< 视图层图元（不拥有所有权）

    // ---- 内部辅助 ----

    /// 结算阶段逻辑
    void resolveBattle();

    /// 检测并执行自动升星（3 合 1 合成）
    /// 调用时机：购买单位后、每轮准备阶段开始时
    void processMerges();

    /// 随机生成一件装备（40% 概率掉落）
    /// 胜利时由 resolveBattle 调用
    void rollEquipmentDrop();

    /// 收集当前游戏状态为 SaveData（供存档使用）
    SaveData collectSaveData() const;

    /// 从 SaveData 恢复游戏状态（供读档使用）
    void applySaveData(const SaveData &data);

    /// 根据英雄名称匹配技能（用于购买/合并后自动分配）
    Skill *skillForUnitName(const QString &name);

    /// 星级 → 出售价格倍率（1★=×1, 2★=×3, 3★=×9）
    static int sellPriceMultiplier(int starLevel);

    /// 启动新一轮
    void startNewRound();

    /// 根据 Unit* 查找对应的 UnitItem*（平行数组查找）
    UnitItem *findUnitItem(const Unit *unit) const;

    /// 在敌方半场随机寻找一个未被占用的空格
    /// @return 棋盘坐标 QPoint(col, row)；若无空位返回 (-1, -1)
    QPoint findFreeEnemyCell() const;
};

#endif // GAME_H
