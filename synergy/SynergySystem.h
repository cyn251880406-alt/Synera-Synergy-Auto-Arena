#ifndef SYNERGYSYSTEM_H
#define SYNERGYSYSTEM_H

#include <map>
#include <vector>

#include "unit/Unit.h"  // Trait 枚举、Unit 类

// ============================================================================
// TraitBonusTier — 羁绊激活阈值与加成定义
// ============================================================================

/**
 * @brief TraitBonusTier — 羁绊激活阈值与加成定义
 *
 * 每个 Trait 对应一组阈值，上场同标签单位数达标时触发对应加成。
 * 同一羁绊多阈值同时满足时仅最高者生效。
 */
struct TraitBonusTier {
    int threshold;    ///< 触发该层所需的最小单位数
    int atkPercent;   ///< 攻击力加成百分比（0 表示无加成）
    int maxHpPercent; ///< 最大生命值加成百分比（0 表示无加成）
};

// ============================================================================
// SynergySystem — 羁绊系统（逻辑层）
// ============================================================================

/**
 * @brief SynergySystem — 管理上阵单位的羁绊计数与属性加成
 *
 * 统计上阵单位中各羁绊标签的数量，根据阈值计算属性加成。
 * 战斗前 applyBuffs() 修改属性，战斗后 clearBuffs() 恢复。
 * 纯逻辑层，通过 Unit::setRawAtk()/setRawMaxHp() 直接修改属性。
 */
class SynergySystem {
public:
    // ========== 构造 ==========

    SynergySystem() = default;
    ~SynergySystem() = default;

    // ========== 羁绊计数 ==========

    /**
     * @brief 统计所有上阵单位中各羁绊标签的出现次数
     *
     * 遍历单位列表，对每个单位的 traits() 中的所有标签计数。
     * 一个单位有多个标签时，每个标签各自计数+1。
     *
     * @param units 上阵单位列表（通常为 BattleSystem::aliveUnits()）
     * @return 标签 → 出现次数的映射（仅含 count > 0 的条目）
     */
    std::map<Trait, int> calculateTraitCounts(
        const std::vector<Unit *> &units) const;

    // ========== 战斗加成 ==========

    /**
     * @brief 战斗前应用羁绊加成
     *
     * 流程：
     *   1. 清空上次残留的 savedStats（防御性保护）
     *   2. 统计当前上阵单位的标签分布
     *   3. 对每个单位，计算其所有激活标签的加成总和
     *   4. 保存原属性到 m_savedStats
     *   5. 修改单位的 m_atk / m_maxHp
     *
     * 注意：多个标签可叠加（如骑士同时有 Warrior+Guardian，
     *       可能同时获得 ATK 和 MaxHP 加成）。
     *       同一标签的多层阈值只取最高层。
     *
     * @param units 上阵单位列表
     */
    void applyBuffs(const std::vector<Unit *> &units);

    /**
     * @brief 战斗后清除羁绊加成（恢复原始属性）
     *
     * 遍历 m_savedStats，将每个单位的 ATK 和 MaxHP 恢复为保存值。
     * 恢复后清空 m_savedStats。
     */
    void clearBuffs();

    // ========== 查询 ==========

    /**
     * @brief 查询指定标签在指定计数下激活的加成层
     *
     * 静态方法，不依赖实例状态。用于 UI 显示当前羁绊激活情况。
     *
     * @param trait 查询的羁绊标签
     * @param count 当前该标签的单位数
     * @return 最高激活层指针；无激活层时返回 nullptr
     */
    static const TraitBonusTier *activeTier(Trait trait, int count);

private:
    // ========== 数据 ==========

    /// 保存的单位原始属性（用于 clearBuffs 恢复）
    /// key = Unit*，value = (原始 ATK, 原始 MaxHP)
    std::map<Unit *, std::pair<int, int>> m_savedStats;

    // ========== 羁绊定义表 ==========

    /// 所有羁绊的阈值与加成定义（只读，所有实例共享）
    static const std::map<Trait, std::vector<TraitBonusTier>> s_bonusTable;

    // ========== 内部辅助 ==========

    /**
     * @brief 保存一个单位当前的 ATK 和 MaxHP
     *
     * 仅当该单位尚未被保存时执行（防止重复保存覆盖正确值）。
     *
     * @param unit 要保存的单位指针
     */
    void saveUnitStats(Unit *unit);

    /// 清除所有保存记录
    void clearSavedStats();
};

#endif // SYNERGYSYSTEM_H
