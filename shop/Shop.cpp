#include "shop/Shop.h"
#include "unit/Unit.h"
#include "core/Player.h"
#include "bench/Bench.h"

#include <QRandomGenerator>

// ============================================================================
// 静态英雄池定义
// ============================================================================

/**
 * 可购买英雄池（6 种，3 职业各 2 名）：
 *   费用 1: 神射手(射手)、重装兵(坦克)、剑术师(战士) — 较弱，出现率高
 *   费用 2: 鹰眼(射手)、守护者(坦克)、狂战士(战士) — 稍强，出现率低
 *
 * 全部以 1★ 出现在商店，3 合 1 升 2★，3×2★ 升 3★。
 */
const std::vector<HeroTemplate> Shop::s_heroPool = {
    // ===== 费用 1（1★ 入门） =====
    {
        QStringLiteral("神射手"),   // name
        1,                          // cost
        220, 32, 3, 60,             // hp, atk, range, maxMana
        { Trait::Archer }           // traits
    },
    {
        QStringLiteral("重装兵"),
        1,
        450, 18, 1, 70,
        { Trait::Tank }
    },
    {
        QStringLiteral("剑术师"),
        1,
        320, 28, 1, 55,
        { Trait::Warrior }
    },

    // ===== 费用 2（1★ 稀有） =====
    {
        QStringLiteral("鹰眼"),
        2,
        200, 42, 3, 50,
        { Trait::Archer }
    },
    {
        QStringLiteral("守护者"),
        2,
        550, 22, 1, 75,
        { Trait::Tank }
    },
    {
        QStringLiteral("狂战士"),
        2,
        350, 38, 1, 50,
        { Trait::Warrior }
    },
};

// ============================================================================
// 构造
// ============================================================================

Shop::Shop()
{
    // 所有槽位初始均为空
    m_slots.fill(nullptr);
}

// ============================================================================
// 刷新商店
// ============================================================================

void Shop::refreshShop(int roundNumber)
{
    // 钳制轮次下限，防止 0 或负值导致的异常概率
    if (roundNumber < 1)
        roundNumber = 1;

    // 为每个槽位独立随机选取英雄
    for (int i = 0; i < SLOT_COUNT; ++i) {
        int cost = pickCostByRound(roundNumber);
        m_slots[i] = pickRandomHero(cost);
    }
}

// ============================================================================
// 购买单位
// ============================================================================

Unit *Shop::buyUnit(int slotIndex, Player &player, Bench &bench)
{
    // —— 1. 校验槽位合法性 ——
    if (!isValidIndex(slotIndex))
        return nullptr;

    const HeroTemplate *tmpl = m_slots[slotIndex];
    if (!tmpl) {
        // 该槽位没有可购买的英雄
        return nullptr;
    }

    // —— 2. 校验金币 ——
    if (player.gold() < tmpl->cost)
        return nullptr;

    // —— 3. 校验备战区空位 ——
    if (!bench.hasEmptySlot())
        return nullptr;

    // —— 4. 扣除金币 ——
    player.spendGold(tmpl->cost);

    // —— 5. 创建 Unit 实例 ——
    // 位置设为 (-1, -1) 表示"未部署"，后续由玩家拖拽上阵
    auto *unit = new Unit(
        tmpl->name,
        Owner::PlayerCtrl,
        QPoint(-1, -1),
        tmpl->baseHp,
        tmpl->baseAtk,
        tmpl->range,
        tmpl->maxMana
    );

    // 复制羁绊标签
    for (Trait t : tmpl->traits)
        unit->addTrait(t);

    // 根据费用设置初始星级：cost=1 → 1★, cost=3 → 2★
    unit->setStarLevel(costToStarLevel(tmpl->cost));

    // —— 6. 放入备战区 ——
    // Bench::addUnit 自动寻找第一个空槽位
    bench.addUnit(unit);

    // —— 7. 清空该商店槽位（已售出） ——
    m_slots[slotIndex] = nullptr;

    return unit;
}

// ============================================================================
// 查询
// ============================================================================

const HeroTemplate *Shop::slot(int index) const
{
    if (!isValidIndex(index))
        return nullptr;
    return m_slots[index];
}

bool Shop::isEmpty(int index) const
{
    if (!isValidIndex(index))
        return true;
    return m_slots[index] == nullptr;
}

std::vector<const HeroTemplate *> Shop::availableSlots() const
{
    std::vector<const HeroTemplate *> result;
    for (const auto *tmpl : m_slots) {
        if (tmpl)
            result.push_back(tmpl);
    }
    return result;
}

// ============================================================================
// 内部辅助
// ============================================================================

int Shop::pickCostByRound(int roundNumber)
{
    // 轮次越高，高费用英雄的出现概率越大
    struct CostProb { int cost; int probability; };  // probability = 万分比

    static const CostProb TABLE[5][3] = {
        // Round 1-2: 只出费用 1
        { {1, 10000}, {2, 0}, {0, 0} },

        // Round 3-4: 费用 1 (70%) / 费用 2 (30%)
        { {1, 7000}, {2, 3000}, {0, 0} },

        // Round 5-6: 费用 1 (50%) / 费用 2 (50%)
        { {1, 5000}, {2, 5000}, {0, 0} },

        // Round 7-8: 费用 1 (35%) / 费用 2 (65%)
        { {1, 3500}, {2, 6500}, {0, 0} },

        // Round 9+: 费用 1 (25%) / 费用 2 (75%)
        { {1, 2500}, {2, 7500}, {0, 0} },
    };

    // 选择对应的概率行
    int tableRow;
    if (roundNumber >= 9)
        tableRow = 4;
    else if (roundNumber >= 7)
        tableRow = 3;
    else if (roundNumber >= 5)
        tableRow = 2;
    else if (roundNumber >= 3)
        tableRow = 1;
    else
        tableRow = 0;

    // 按概率随机抽取
    int roll = QRandomGenerator::global()->bounded(10000);
    int cumulative = 0;

    for (int i = 0; i < 3; ++i) {
        if (TABLE[tableRow][i].probability == 0)
            break;
        cumulative += TABLE[tableRow][i].probability;
        if (roll < cumulative)
            return TABLE[tableRow][i].cost;
    }

    // 兜底：返回费用 1（理论上不会执行到此处，因为概率总和为 10000）
    return 1;
}

const HeroTemplate *Shop::pickRandomHero(int cost)
{
    // 收集所有匹配费用的英雄
    std::vector<const HeroTemplate *> candidates;
    for (const auto &hero : s_heroPool) {
        if (hero.cost == cost)
            candidates.push_back(&hero);
    }

    if (candidates.empty())
        return nullptr;

    // 随机选取一个
    int idx = QRandomGenerator::global()->bounded(
        static_cast<int>(candidates.size()));
    return candidates[static_cast<size_t>(idx)];
}

bool Shop::isValidIndex(int index) const
{
    return index >= 0 && index < SLOT_COUNT;
}

bool Shop::restoreSlot(int index, const QString &heroName)
{
    if (!isValidIndex(index))
        return false;

    if (heroName.isEmpty()) {
        m_slots[index] = nullptr;
        return true;
    }

    const HeroTemplate *tmpl = findHeroInPool(heroName);
    if (!tmpl)
        return false;

    m_slots[index] = tmpl;
    return true;
}

const HeroTemplate *Shop::findHeroInPool(const QString &name)
{
    for (const auto &hero : s_heroPool) {
        if (hero.name == name)
            return &hero;
    }
    return nullptr;
}

int Shop::heroCost(const QString &name)
{
    const HeroTemplate *tmpl = findHeroInPool(name);
    return tmpl ? tmpl->cost : 0;
}

int Shop::costToStarLevel(int)
{
    return 1;  // 全部以 1★ 出现在商店
}
