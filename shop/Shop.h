#ifndef SHOP_H
#define SHOP_H

#include <QString>
#include <QVector>

#include "unit/Unit.h"  // Trait 枚举定义

#include <array>
#include <vector>

class Unit;
class Player;
class Bench;

// ============================================================================
// Trait — 完整羁绊标签（声明在 unit/Unit.h 中，此处复用）
// ============================================================================

/**
 * @brief Trait 枚举在 unit/Unit.h 中定义，包含：
 *   None, Warrior, Ranger, Guardian, Mage, Assassin
 *
 * 每个可购买英雄拥有 1–2 个羁绊标签，供阶段三羁绊系统使用。
 */

// ============================================================================
// HeroTemplate — 可购买英雄模板
// ============================================================================

/**
 * @brief HeroTemplate — 商店英雄池中的一条模板记录
 *
 * 定义一种可购买英雄的基础属性、价格与羁绊标签。
 * 所有模板在 Shop 中以静态数据形式存在（只读），
 * 每次 refreshShop() 从池中随机选取 5 个填充商店。
 */
struct HeroTemplate {
    QString     name;       ///< 英雄名称（如"剑士"、"法师"）
    int         cost;       ///< 购买价格（金币），同时也是品质等级
    int         baseHp;     ///< 基础生命值
    int         baseAtk;    ///< 基础攻击力
    int         range;      ///< 攻击距离（格，1=近战，3=远程）
    int         maxMana;    ///< 最大法力值
    QVector<Trait> traits;  ///< 羁绊标签（1–2 个）
};

// ============================================================================
// Shop — 商店系统（逻辑层）
// ============================================================================

/**
 * @brief Shop — 商店系统（逻辑层）
 *
 * 维护 5 个商店槽位，按轮次概率刷新可购买英雄。
 * buyUnit() 扣金 → 创建 Unit → 放入备战区。
 * 纯逻辑层，通过 Player 操作金币，通过 Bench 放置单位。
 */
class Shop {
public:
    // ========== 常量 ==========

    /// 商店槽位数
    static constexpr int SLOT_COUNT = 5;

    /// 刷新商店所需金币
    static constexpr int REFRESH_COST = 2;

    // ========== 构造 ==========

    Shop();
    ~Shop() = default;

    // ========== 商店操作 ==========

    /**
     * @brief 刷新商店（按轮次重新生成 5 个可购买单位）
     *
     * 轮次越高，高品质（高费用）英雄的出现概率越大。
     * 轮次概率分布：
     *   第 1–2 轮： 费用 1 (100%)
     *   第 3–4 轮： 费用 1 (70%) / 费用 2 (30%)
     *   第 5–6 轮： 费用 1 (40%) / 费用 2 (35%) / 费用 3 (25%)
     *   第 7–8 轮： 费用 1 (20%) / 费用 2 (30%) / 费用 3 (30%) / 费用 4 (20%)
     *   第 9+ 轮：  费用 1 (10%) / 费用 2 (20%) / 费用 3 (35%) / 费用 4 (35%)
     *
     * 每次刷新覆盖所有 5 个槽位，旧数据丢失。
     *
     * @param roundNumber 当前轮次（从 1 开始）
     */
    void refreshShop(int roundNumber);

    /**
     * @brief 从指定槽位购买单位
     *
     * 购买流程：
     *   1. 校验槽位合法性（索引范围、槽位非空）
     *   2. 校验玩家金币 >= 费用
     *   3. 校验备战区有空位
     *   4. 扣除玩家金币
     *   5. 创建 Unit 实例（Owner::PlayerCtrl，位置 (-1,-1)）
     *   6. 将 Unit 放入备战区第一个空槽位
     *   7. 清空该商店槽位
     *   8. 返回 Unit*（调用方负责创建 UnitItem 并注册）
     *
     * @param slotIndex 槽位索引 [0, SLOT_COUNT)
     * @param player    玩家实体（用于扣金校验）
     * @param bench     备战区（用于放置新购买的单位）
     * @return 购买成功 → 新创建的 Unit*；失败 → nullptr
     */
    Unit *buyUnit(int slotIndex, Player &player, Bench &bench);

    // ========== 查询 ==========

    /**
     * @brief 返回商店槽位的英雄模板指针
     *
     * @param index 槽位索引 [0, SLOT_COUNT)
     * @return 该槽位的 HeroTemplate*；若槽位为空返回 nullptr
     */
    const HeroTemplate *slot(int index) const;

    /// 指定槽位是否为空（无英雄可购买）
    bool isEmpty(int index) const;

    /// 返回当前商店中所有非空槽位的模板
    std::vector<const HeroTemplate *> availableSlots() const;

    // ========== 存档恢复 ==========

    /**
     * @brief 按英雄名称恢复指定槽位（用于读档重建商店）
     *
     * @param index    槽位索引 [0, SLOT_COUNT)
     * @param heroName 英雄名称；传入空字符串则设该槽为空
     * @return true 设置成功（含设为空）；false 英雄名称不匹配池中任何模板
     */
    bool restoreSlot(int index, const QString &heroName);

    // ========== 静态工具 ==========

    /// 根据英雄名称查询购买费用（出售时使用）
    static int heroCost(const QString &name);

    /// 费用 → 初始星级：cost=1 → 1★, cost=3 → 2★
    static int costToStarLevel(int cost);

private:
    // ========== 数据 ==========

    /// 5 个商店槽位，每个指向一个 HeroTemplate（nullptr = 空）
    std::array<const HeroTemplate *, SLOT_COUNT> m_slots;

    // ========== 英雄池（静态） ==========

    /// 所有可购买英雄的模板池（只读，所有 Shop 实例共享）
    static const std::vector<HeroTemplate> s_heroPool;

    // ========== 内部辅助 ==========

    /**
     * @brief 按轮次随机选取一个费用等级
     *
     * 使用给定的概率分布决定本次抽取的英雄品质。
     *
     * @param roundNumber 当前轮次
     * @return 费用值 [1, 4]（对应 Tier 1–4）
     */
    static int pickCostByRound(int roundNumber);

    /**
     * @brief 从英雄池中随机选取一个指定费用的英雄
     *
     * @param cost 目标费用等级
     * @return 匹配的英雄模板指针；若池中没有该费用的英雄则返回 nullptr
     */
    static const HeroTemplate *pickRandomHero(int cost);

    /// 按名称从静态英雄池中查找模板（读档用）
    static const HeroTemplate *findHeroInPool(const QString &name);

    /**
     * @brief 检查索引是否在有效范围 [0, SLOT_COUNT) 内
     */
    bool isValidIndex(int index) const;
};

#endif // SHOP_H
