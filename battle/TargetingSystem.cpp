#include "battle/TargetingSystem.h"
#include "unit/Unit.h"

#include <algorithm>     // std::min_element

// ============================================================================
// 索敌接口
// ============================================================================

Unit *TargetingSystem::findTarget(const Unit *source,
                                  const std::vector<Unit *> &candidates) const
{
    if (!source)
        return nullptr;
    if (candidates.empty())
        return nullptr;

    // —— 获取 source 的棋盘坐标 ——
    int srcX = source->position().x();
    int srcY = source->position().y();

    // —— 使用 std::min_element 选出最优目标 ——
    // 比较函数：距离平方越小越优先；距离相同时按平局规则决定
    auto best = std::min_element(candidates.begin(), candidates.end(),
        [srcX, srcY](const Unit *a, const Unit *b) {
            // ——— 第 1 级：欧氏距离（平方值） ———
            int ax = a->position().x();
            int ay = a->position().y();
            int bx = b->position().x();
            int by = b->position().y();

            double distA = distanceSq(srcX, srcY, ax, ay);
            double distB = distanceSq(srcX, srcY, bx, by);

            if (distA != distB)
                return distA < distB;       // 距离越近越优先

            // ——— 第 2~4 级：距离相同时的平局规则 ———
            return isHigherPriority(a, b);
        });

    return *best;
}

// ============================================================================
// 内部辅助
// ============================================================================

double TargetingSystem::distanceSq(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;
    return static_cast<double>(dx) * dx + static_cast<double>(dy) * dy;
}

bool TargetingSystem::isHigherPriority(const Unit *a, const Unit *b)
{
    // —— 规则 1：血量高优先 ——
    // 优先攻击满血/高血量单位，保留低血量单位给后续收割
    if (a->hp() != b->hp())
        return a->hp() > b->hp();

    // —— 规则 2：从左向右（列号小的优先） ——
    // 列坐标 x 越小 → 越靠左 → 优先级越高
    if (a->position().x() != b->position().x())
        return a->position().x() < b->position().x();

    // —— 规则 3：从下到上（行号大的优先） ——
    // 行坐标 y 越大 → 越靠下 → 优先级越高
    return a->position().y() > b->position().y();
}
