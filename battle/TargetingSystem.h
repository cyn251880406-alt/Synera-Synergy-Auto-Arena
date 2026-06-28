#ifndef TARGETINGSYSTEM_H
#define TARGETINGSYSTEM_H

#include <vector>

class Unit;          // 前向声明，避免循环依赖
class QPoint;        // 仅用于前向声明，实际 include 在 .cpp 中

// ============================================================================
// TargetingSystem — 索敌系统（逻辑层）
// ============================================================================

/**
 * @brief TargetingSystem — 为战斗单位选择最优攻击目标
 *
 * 四级优先级：欧氏距离最小 → 血量最高 → 从左向右 → 从下到上。
 * 纯算法类，无状态，使用 STL std::vector。
 */
class TargetingSystem {
public:
    // ========== 构造/析构 ==========

    TargetingSystem() = default;
    ~TargetingSystem() = default;

    // ========== 索敌接口 ==========

    /**
     * @brief 为 source 单位从候选池中选出最优攻击目标
     *
     * 排序规则（优先级从高到低）：
     *   1. 欧氏距离最小（平方比较，避免开根号）
     *   2. 距离相同 → 当前血量最高（hp 值大的优先）
     *   3. 血量相同 → 从左向右（棋盘列坐标 x 小的优先）
     *   4. 列相同   → 从下到上（棋盘行坐标 y 大的优先）
     *
     * 调用方需自行保证 candidates 中的单位满足：
     *   - owner ≠ source->owner()（敌方单位）
     *   - isAlive() == true（存活）
     *
     * @param source     搜索方单位（不能为 nullptr）
     * @param candidates 候选敌方单位列表（STL vector）
     * @return 最优目标单位指针；candidates 为空时返回 nullptr
     */
    Unit *findTarget(const Unit *source,
                     const std::vector<Unit *> &candidates) const;

private:
    // ========== 内部辅助 ==========

    /**
     * @brief 计算两点间欧氏距离的平方
     *
     * 使用平方值比较避免开根号，性能更优。
     * 对任意两点 a, b：dist(a,b) < dist(c,d) ⇔ distSq(a,b) < distSq(c,d)
     */
    static double distanceSq(int x1, int y1, int x2, int y2);

    /**
     * @brief 比较两个候选目标的优先级
     *
     * 当前已确认 a 和 b 到 source 的距离相同，
     * 按平局规则（血量 → 列 → 行）决定谁更优先。
     *
     * @return true 表示 a 的优先级高于 b（应排在前面）
     */
    static bool isHigherPriority(const Unit *a, const Unit *b);
};

#endif // TARGETINGSYSTEM_H
