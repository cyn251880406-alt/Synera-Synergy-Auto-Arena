#include "battle/Pathfinder.h"
#include "unit/Unit.h"

#include <queue>
#include <utility>        // std::pair, std::make_pair

// ============================================================================
// BFS 寻路
// ============================================================================

std::vector<QPoint> Pathfinder::findPath(
    QPoint start, QPoint target, int range,
    const bool occupied[BOARD_SIZE][BOARD_SIZE]) const
{
    int tx = target.x(), ty = target.y();

    // —— 已在攻击范围内 → 无需移动 ——
    int dx0 = start.x() - tx;
    int dy0 = start.y() - ty;
    if (dx0 * dx0 + dy0 * dy0 <= range * range)
        return {};

    // —— BFS 数据结构 ——
    bool visited[BOARD_SIZE][BOARD_SIZE] = {};
    QPoint prev[BOARD_SIZE][BOARD_SIZE];

    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            prev[r][c] = QPoint(-1, -1);

    std::queue<QPoint> q;

    int sr = start.y(), sc = start.x();
    visited[sr][sc] = true;
    q.push(start);

    // —— BFS 主循环 ——
    QPoint endpoint = start;  // 找到的终点（攻击范围内离目标最近的格子）
    bool found = false;

    while (!q.empty()) {
        QPoint cur = q.front();
        q.pop();

        int cr = cur.y(), cc = cur.x();

        // 检查当前格是否已在攻击范围内（所有格子均视为障碍物，BFS 到达目标邻格即停）
        int dx = cc - tx;
        int dy = cr - ty;
        if (dx * dx + dy * dy <= range * range) {
            endpoint = cur;
            found = true;
            break;
        }

        // 探索四个方向
        for (int d = 0; d < 4; ++d) {
            int nr = cr + DIRS[d][1];
            int nc = cc + DIRS[d][0];

            if (nr < 0 || nr >= BOARD_SIZE || nc < 0 || nc >= BOARD_SIZE)
                continue;

            if (visited[nr][nc])
                continue;

            // 所有被占用的格子均不可通行（含目标格）
            if (occupied[nr][nc])
                continue;

            visited[nr][nc] = true;
            prev[nr][nc] = cur;
            q.push(QPoint(nc, nr));
        }
    }

    if (!found)
        return {};

    // —— 回溯构造路径 ——
    std::vector<QPoint> reversePath;
    QPoint at = endpoint;

    while (at != start) {
        reversePath.push_back(at);
        at = prev[at.y()][at.x()];

        if (at.x() < 0)
            break;
    }

    std::vector<QPoint> path;
    for (int i = static_cast<int>(reversePath.size()) - 1; i >= 0; --i)
        path.push_back(reversePath[static_cast<size_t>(i)]);

    return path;
}

// ============================================================================
// 单步移动
// ============================================================================

bool Pathfinder::moveStep(Unit *unit,
                           const bool occupied[BOARD_SIZE][BOARD_SIZE])
{
    if (!unit)
        return false;

    // —— 1. 无路径 → 回到 Idle ——
    if (!unit->hasPath()) {
        if (unit->state() == UnitState::Moving)
            unit->changeState(UnitState::Idle);
        return false;
    }

    // —— 2. 移动计时器倒计时 ——
    // 计时器 > 0 表示还在前往下一格的路上（模拟移动耗时）
    if (unit->moveTimer() > 0) {
        unit->decrementMoveTimer();
        return false;
    }

    // —— 3. 路径已空（防御性检查） ——
    const auto &path = unit->path();
    if (path.empty()) {
        unit->clearPath();
        unit->changeState(UnitState::Idle);
        return false;
    }

    // —— 4. 取出路径中的下一格 ——
    QPoint next = path.front();   // 路径头即是下一步目标格
    int nr = next.y(), nc = next.x();

    // —— 4b. 下一格是攻击目标本身 → 已进入攻击范围，停下 ——
    // BFS 路径包含目标格（搜索时目标格按可通行处理），
    // 但单位不应走进目标所在格，应在邻格停下准备攻击
    if (unit->currentTarget() && unit->currentTarget()->isAlive()) {
        QPoint tp = unit->currentTarget()->position();
        if (nr == tp.y() && nc == tp.x()) {
            unit->clearPath();
            unit->changeState(UnitState::Idle);
            return false;
        }
    }

    // —— 5. 下一格被阻塞 → 清空路径，重新索敌 ——
    // 发生场景：另一单位先一步移动到了该格
    if (occupied[nr][nc]) {
        unit->clearPath();
        unit->changeState(UnitState::Idle);
        return false;
    }

    // —— 6. 弹出路径头，更新单位位置 ——
    unit->popPathCell();          // 从 path 中移除已到达的格子
    unit->setPosition(next);
    unit->setMoveTimer(MOVE_SPEED);   // 重置计时器，下一格需要再等 MOVE_SPEED 帧

    // —— 7. 到达目标附近 → 清空路径，准备攻击 ——
    if (unit->currentTarget() && unit->currentTarget()->isAlive()) {
        QPoint tp = unit->currentTarget()->position();
        int dx = next.x() - tp.x();
        int dy = next.y() - tp.y();
        double distSq = dx * dx + dy * dy;
        double rangeSq = unit->range() * unit->range();

        if (distSq <= rangeSq) {
            // 已进入攻击范围 → 停下准备战斗
            unit->clearPath();
            unit->changeState(UnitState::Idle);
        }
    }

    // —— 8. 路径耗尽（到达末端） → 回到 Idle ——
    // 注：正常情况在步骤 7 就会因为进入攻击范围而清空路径
    //     此分支仅为防御：目标中途死亡导致路径末端非攻击范围
    if (unit->hasPath() && unit->path().empty()) {
        unit->clearPath();
        unit->changeState(UnitState::Idle);
    }

    return true;  // 本帧确实移动了
}
