#include "synergy/SynergySystem.h"

#include <algorithm>  // std::max
#include <utility>    // std::pair

// ============================================================================
// 静态羁绊定义表
// ============================================================================

/**
 * 羁绊加成表（s_bonusTable）
 *
 * 当前设计（3 种羁绊，对应 3 职业）：
 *
 * Archer  射手  [2] ATK+15%     [3] ATK+30%       ← 属性光环（远程压制）
 * Tank    坦克  [2] MaxHP+15%  [3] MaxHP+30%      ← 属性光环（钢铁防线）
 * Warrior 战士  [2] ATK+12%     [3] ATK+25%       ← 属性光环（近战精通）
 */
const std::map<Trait, std::vector<TraitBonusTier>> SynergySystem::s_bonusTable = {
    { Trait::Archer, {
        { 2, 15, 0 },   // 2 名射手 → ATK+15%
        { 3, 30, 0 }    // 3 名射手 → ATK+30%
    }},
    { Trait::Tank, {
        { 2, 0, 15 },   // 2 名坦克 → MaxHP+15%
        { 3, 0, 30 }    // 3 名坦克 → MaxHP+30%
    }},
    { Trait::Warrior, {
        { 2, 12, 0 },   // 2 名战士 → ATK+12%
        { 3, 25, 0 }    // 3 名战士 → ATK+25%
    }}
};

// ============================================================================
// 羁绊计数
// ============================================================================

std::map<Trait, int> SynergySystem::calculateTraitCounts(
    const std::vector<Unit *> &units) const
{
    std::map<Trait, int> counts;

    for (const Unit *u : units) {
        if (!u->isAlive())
            continue;

        // 遍历该单位的每个羁绊标签，各自 +1
        for (Trait t : u->traits()) {
            if (t != Trait::None)
                ++counts[t];
        }
    }

    return counts;
}

// ============================================================================
// 战斗加成：应用与清除
// ============================================================================

void SynergySystem::applyBuffs(const std::vector<Unit *> &units)
{
    // —— 1. 清空上次残留 ——
    clearSavedStats();

    // —— 2. 计算当前羁绊分布 ——
    std::map<Trait, int> counts = calculateTraitCounts(units);

    // —— 3. 对每个活跃标签，确定当前激活的最高层 ——
    // 将每个标签的加成存入 lookup，供后续按 unit 合并
    std::map<Trait, const TraitBonusTier *> activeBonuses;

    for (const auto &[trait, count] : counts) {
        const TraitBonusTier *tier = activeTier(trait, count);
        if (tier)
            activeBonuses[trait] = tier;
    }

    // —— 4. 对每个上阵单位，计算其所有标签的总加成 ——
    for (Unit *u : units) {
        if (!u->isAlive())
            continue;

        int totalAtkPercent = 0;
        int totalHpPercent  = 0;

        // 遍历该单位的标签，积累来自不同羁绊的加成
        for (Trait t : u->traits()) {
            auto it = activeBonuses.find(t);
            if (it != activeBonuses.end()) {
                totalAtkPercent += it->second->atkPercent;
                totalHpPercent  += it->second->maxHpPercent;
            }
        }

        // 无激活加成 → 跳过
        if (totalAtkPercent == 0 && totalHpPercent == 0)
            continue;

        // —— 5. 保存原始属性 ——
        saveUnitStats(u);

        // —— 6. 应用加成 ——
        // 整数百分比计算：atk = atk * (100 + percent) / 100
        int newAtk   = u->atk();
        int newMaxHp = u->maxHp();

        if (totalAtkPercent > 0) {
            newAtk = u->atk() * (100 + totalAtkPercent) / 100;
            // 确保至少 +1（防止整数截断为零加成）
            if (newAtk == u->atk() && totalAtkPercent > 0)
                newAtk = u->atk() + 1;
        }

        if (totalHpPercent > 0) {
            newMaxHp = u->maxHp() * (100 + totalHpPercent) / 100;
            if (newMaxHp == u->maxHp() && totalHpPercent > 0)
                newMaxHp = u->maxHp() + 1;
        }

        u->setRawAtk(newAtk);
        u->setRawMaxHp(newMaxHp);
    }
}

void SynergySystem::clearBuffs()
{
    // 遍历所有被保存的单位，恢复原始属性
    for (auto &[unit, saved] : m_savedStats) {
        if (unit) {
            unit->setRawAtk(saved.first);
            unit->setRawMaxHp(saved.second);
        }
    }

    clearSavedStats();
}

// ============================================================================
// 查询
// ============================================================================

const TraitBonusTier *SynergySystem::activeTier(Trait trait, int count)
{
    // 查找该羁绊的定义
    auto it = s_bonusTable.find(trait);
    if (it == s_bonusTable.end())
        return nullptr;

    // 从高阈值向低阈值遍历，返回第一个满足条件者
    // 因为阈值列表按升序排列（2→4），逆序遍历找最高层
    const auto &tiers = it->second;
    const TraitBonusTier *best = nullptr;

    for (const auto &tier : tiers) {
        if (count >= tier.threshold)
            best = &tier;  // 后遍历到的阈值更高
    }

    return best;  // 无满足条件时返回 nullptr
}

// ============================================================================
// 内部辅助
// ============================================================================

void SynergySystem::saveUnitStats(Unit *unit)
{
    if (!unit)
        return;

    // 仅当尚未保存时才保存（避免覆盖）
    if (m_savedStats.find(unit) == m_savedStats.end()) {
        m_savedStats[unit] = { unit->atk(), unit->maxHp() };
    }
}

void SynergySystem::clearSavedStats()
{
    m_savedStats.clear();
}
