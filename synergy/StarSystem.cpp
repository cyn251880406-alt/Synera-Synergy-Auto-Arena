#include "synergy/StarSystem.h"

#include <cmath>     // std::pow
#include <map>       // std::map (分组计数)
#include <utility>   // std::pair

// ============================================================================
// 合成检测
// ============================================================================

std::vector<MergeCandidate> StarSystem::findMerges(
    const std::vector<Unit *> &units) const
{
    // —— 1. 按 "名称 + 当前星级" 分组 ——
    // key = name + starLevel，如 "剑士1"、"法师2"
    // 这样 1★ 剑士和 2★ 剑士不会混在一起
    std::map<QString, std::vector<Unit *>> groups;

    for (Unit *u : units) {
        // 跳过空指针、死亡单位、敌方单位（只合我方单位）
        if (!u || !u->isAlive())
            continue;
        if (u->owner() != Owner::PlayerCtrl)
            continue;

        // 生成分组 key
        QString key = u->name() + QString::number(u->starLevel());
        groups[key].push_back(u);
    }

    // —— 2. 从每组中提取 3 合 1 候选 ——
    std::vector<MergeCandidate> candidates;

    for (auto &[key, group] : groups) {
        // 每组可能有 ≥ 3 个同名同星单位，每 3 个生成一个候选
        // 例如 5 个同名同星 → 1 个候选（消耗 3 个），剩余 2 个等待下次合成
        while (group.size() >= UNITS_REQUIRED) {
            MergeCandidate mc;
            mc.unit1       = group[0];
            mc.unit2       = group[1];
            mc.unit3       = group[2];
            mc.name        = mc.unit1->name();
            mc.currentStar = mc.unit1->starLevel();
            mc.targetStar  = mc.currentStar + 1;

            candidates.push_back(mc);

            // 消耗这 3 个单位（从本地分组中移除）
            group.erase(group.begin(), group.begin() + UNITS_REQUIRED);
        }
    }

    return candidates;
}

// ============================================================================
// 执行合成
// ============================================================================

Unit *StarSystem::executeMerge(const MergeCandidate &candidate) const
{
    // 以第一个单位为基础模板
    Unit *base = candidate.unit1;
    if (!base)
        return nullptr;

    // —— 1. 计算升星后的属性 ——
    int newMaxHp = statForStar(base->maxHp(),
                               candidate.currentStar,
                               candidate.targetStar);
    int newAtk   = statForStar(base->atk(),
                               candidate.currentStar,
                               candidate.targetStar);

    // Range、MaxMana、AttackSpeed 不随星级变化
    int newRange    = base->range();
    int newMaxMana  = base->maxMana();
    int attackSpeed = base->attackSpeed();

    // —— 2. 创建新的高星 Unit ——
    // 位置设为 (-1, -1)，由 Game 层后续处理放置
    auto *newUnit = new Unit(
        candidate.name,
        Owner::PlayerCtrl,
        QPoint(-1, -1),
        newMaxHp,
        newAtk,
        newRange,
        newMaxMana,
        attackSpeed
    );

    // —— 3. 复制羁绊标签 ——
    for (Trait t : base->traits()) {
        if (t != Trait::None)
            newUnit->addTrait(t);
    }

    // —— 4. 设置星级 ——
    newUnit->setStarLevel(candidate.targetStar);

    return newUnit;
}

// ============================================================================
// 静态工具
// ============================================================================

int StarSystem::statForStar(int baseStat, int baseStar, int targetStar)
{
    // 目标星级不高于当前 → 无变化
    if (targetStar <= baseStar)
        return baseStat;

    // 属性按 STAR_MULTIPLIER 倍率逐级增长
    // 1★ → 2★: ×1.8    2★ → 3★: ×1.8（再次）
    int levels = targetStar - baseStar;
    double result = static_cast<double>(baseStat)
                  * std::pow(STAR_MULTIPLIER, levels);
    return static_cast<int>(result);
}
