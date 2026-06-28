#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <vector>
#include <QPoint>

class Unit;

// ============================================================================
// Pathfinder — BFS 寻路系统（逻辑层）
// ============================================================================

/**
 * @brief Pathfinder — 8×8 棋盘上的 BFS 最短路径搜索与单步移动
 *
 * findPath() 四方向 BFS 搜索，moveStep() 每帧推进一格（20帧/格）。
 * 被占据的格子不可通行（起点和目标格除外）。纯逻辑层，无状态。
 */
class Pathfinder {
public:
    // ========== 构造/析构 ==========

    Pathfinder() = default;
    ~Pathfinder() = default;

    // ========== 常量 ==========

    /// 移动一格所需的帧数（20 帧 ≈ 0.33 秒 @ 60fps）
    static constexpr int MOVE_SPEED = 20;

    /// 棋盘大小
    static constexpr int BOARD_SIZE = 8;

    // ========== 寻路接口 ==========

    /**
     * @brief BFS 搜索从 start 到 target 攻击范围内某格的最短路径
     *
     * 所有被占据的格子（含 target 所在格）均不可通行。
     * BFS 在首次到达与 target 距离 ≤ range 的格子时停止。
     *
     * @param start    起点棋盘坐标（单位当前位置）
     * @param target   目标单位棋盘坐标
     * @param range    攻击距离（格数）
     * @param occupied  8×8 占用表
     * @return 从 start 到目标范围的最短路径（不含 start）；
     *         若不可达则返回空向量
     */
    std::vector<QPoint> findPath(QPoint start, QPoint target, int range,
                                  const bool occupied[BOARD_SIZE][BOARD_SIZE]) const;

    /**
     * @brief 为单位执行单帧移动逻辑
     *
     * 内部流程（每帧由 BattleSystem 调用）：
     *   1. 单位无路径 → 回到 Idle
     *   2. 移动计时器 > 0 → 递减并等待
     *   3. 计时器归零 → 取出路径下一格
     *   4. 若下一格被占据 → 清空路径回到 Idle（由上层重新索敌）
     *   5. 移动到下一格 → 重置计时器
     *   6. 若已进入攻击范围 → 清空路径回到 Idle（准备攻击）
     *
     * 移动方向：四方向（上下左右），每次移动一格。
     * 路径由 findPath() 预先计算并存入 Unit::setPath()。
     *
     * @param unit     要移动的单位指针
     * @param occupied  当前 8×8 占用表
     * @return true  单位在本帧移动到了新格子
     * @return false 单位未移动（等待计时器 / 无路径 / 被阻塞）
     */
    bool moveStep(Unit *unit, const bool occupied[BOARD_SIZE][BOARD_SIZE]);

private:
    // ========== 内部常量 ==========

    /// 四方向偏移：上、下、左、右
    /// dirs[i] = {dx, dy}，其中 dx=列偏移, dy=行偏移
    static constexpr int DIRS[4][2] = {
        { 0, -1},  // 上（行号减小）
        { 0,  1},  // 下（行号增大）
        {-1,  0},  // 左（列号减小）
        { 1,  0}   // 右（列号增大）
    };
};

#endif // PATHFINDER_H
