#ifndef BATTLESYSTEM_H
#define BATTLESYSTEM_H

#include <QVector>

#include "unit/Unit.h"           // Unit 类声明、Owner 枚举
#include "battle/TargetingSystem.h"     // 索敌系统
#include "battle/Pathfinder.h"          // 寻路系统

// ============================================================================
// BattleSystem — 战斗系统（逻辑层）
// ============================================================================

/**
 * @brief BattleSystem — 管理所有单位的战斗逻辑
 *
 * 纯逻辑层，不依赖 Qt GUI。Unit 所有权归 Game 层，BattleSystem 仅持有观察指针。
 * 每帧驱动索敌、攻击、寻路、死亡判定，并判断战斗是否结束。
 */
class BattleSystem {
public:
    // ========== 构造/析构 ==========

    BattleSystem();
    ~BattleSystem() = default;

    // ========== 单位管理 ==========

    /**
     * @brief 将一个单位加入战斗
     *
     * 只有部署在棋盘上的单位才应加入 BattleSystem。
     * 备战区中的单位不参与战斗。
     *
     * @param unit 要加入的单位指针（不能为 nullptr）
     */
    void addUnit(Unit *unit);

    /**
     * @brief 将一个单位从战斗中移除
     *
     * 调用时机：单位死亡被清理、或从棋盘回收到备战区时。
     * 不会 delete 该单位。
     *
     * @param unit 要移除的单位指针
     */
    void removeUnit(Unit *unit);

    /// 检查单位是否在战斗系统中
    bool contains(Unit *unit) const;

    /// 移除所有参战单位（用于新一轮重新填充）
    void clear();

    // ========== 查询 ==========

    /// 所有参战单位（含死亡，只读）
    const QVector<Unit *> &allUnits() const { return m_combatUnits; }

    /// 所有存活参战单位
    QVector<Unit *> aliveUnits() const;

    /// 指定 owner 的所有存活参战单位
    QVector<Unit *> aliveUnitsByOwner(Owner owner) const;

    /// 参战单位总数（含死亡）
    int unitCount() const { return m_combatUnits.size(); }

    // ========== 战斗生命周期 ==========

    /**
     * @brief 初始化/重置战斗状态
     *
     * 将所有参战单位的状态重置为 Idle，
     * 为新一轮战斗做好准备。
     * 由 Game::startBattle() 在进入 Combat 阶段时调用。
     */
    void startBattle();

    /**
     * @brief 每帧战斗逻辑更新（由 Game 的 QTimer 每 16ms 调用）
     *
     * 内部流程：检查战斗是否结束 → 更新存活单位 → 索敌 → 攻击或寻路移动
     */
    void updateLoop();

    /**
     * @brief 判断战斗是否结束
     *
     * 结束条件：任一方存活单位数为 0。
     */
    bool isCombatOver() const;

    /**
     * @brief 查询玩家是否获胜
     *
     * 判断依据：至少有一个己方单位存活。
     * 仅在 isCombatOver() == true 时调用才有意义。
     */
    bool playerWon() const;

private:
    // ========== 数据 ==========

    /// 当前参战单位列表（不拥有所有权，仅观察指针）
    QVector<Unit *> m_combatUnits;

    /// 索敌系统（纯算法，STL vector 接口）
    TargetingSystem m_targetingSystem;

    /// 寻路系统（BFS 最短路径 + 单步移动）
    Pathfinder m_pathfinder;

    // ========== 内部辅助 ==========

    /**
     * @brief 为指定单位寻找最近的敌对目标
     *
     * 四级优先级：距离最近 → 血量最高 → 从左向右 → 从下到上
     */
    Unit *findNearestEnemy(Unit *unit) const;

    /**
     * @brief 执行一次普攻
     *
     * 调用 unit->attack(target)，造成伤害、回复法力、重置冷却。
     */
    void processAttack(Unit *attacker, Unit *target);
};

#endif // BATTLESYSTEM_H
