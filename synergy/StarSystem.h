#ifndef STARSYSTEM_H
#define STARSYSTEM_H

#include <QString>

#include <vector>

#include "unit/Unit.h"  // Unit 类、Owner 枚举

// ============================================================================
// MergeCandidate — 升星合成候选信息
// ============================================================================

/**
 * @brief MergeCandidate — 描述一次升星合成的候选单位与结果
 *
 * 3 个同名同星级的单位合并为 1 个高星级单位，属性按倍率提升。
 */
struct MergeCandidate {
    Unit   *unit1 = nullptr;    ///< 第 1 个参与合成的单位
    Unit   *unit2 = nullptr;    ///< 第 2 个参与合成的单位
    Unit   *unit3 = nullptr;    ///< 第 3 个参与合成的单位
    QString name;               ///< 合成后的单位名称（与原单位相同）
    int     currentStar = 1;    ///< 当前星级（如 1 → 升为 2）
    int     targetStar  = 2;    ///< 目标星级（如 1 → 2，2 → 3）
};

// ============================================================================
// StarSystem — 升星系统（逻辑层）
// ============================================================================

/**
 * @brief StarSystem — 管理同名同星单位的自动合成与属性提升
 *
 * findMerges() 检测可合成的三元组，executeMerge() 创建高星 Unit。
 * 纯逻辑层，不依赖 Qt GUI。新 Unit 的删除由 Game 层负责。
 */
class StarSystem {
public:
    // ========== 常量 ==========

    /// 合成所需的同名同星单位数量
    static constexpr int UNITS_REQUIRED = 3;

    /// 星级属性倍率（1★ → 2★: ×1.8, 2★ → 3★: ×1.8² = ×3.24）
    static constexpr double STAR_MULTIPLIER = 1.8;

    // ========== 构造 ==========

    StarSystem() = default;
    ~StarSystem() = default;

    // ========== 合成检测 ==========

    /**
     * @brief 检测所有可能的升星合成
     *
     * 按名称+星级分组，对每组 ≥ 3 的单位生成 MergeCandidate。
     * 同一组中每 3 个生成一个候选（例如 5 个同名同星 → 1 个候选，剩余 2 个等待）。
     *
     * @param units 玩家所有单位（含备战区 + 棋盘上已部署的）
     * @return 所有可执行的合成候选列表
     */
    std::vector<MergeCandidate> findMerges(
        const std::vector<Unit *> &units) const;

    /**
     * @brief 执行一次合成，返回升级后的新 Unit*
     *
     * 合成规则：
     *   - 新 Unit 的 HP = 原 HP × STAR_MULTIPLIER
     *   - 新 Unit 的 ATK = 原 ATK × STAR_MULTIPLIER
     *   - 保留原单位的 Range、MaxMana、AttackSpeed
     *   - 复制原单位的羁绊标签
     *   - 新单位位置设为 (-1, -1)，由 Game 层在后续处理中放置
     *
     * @param candidate 由 findMerges 返回的合成候选
     * @return 新创建的高星 Unit*（调用方负责 delete 旧 3 单位并注册新单位）
     */
    Unit *executeMerge(const MergeCandidate &candidate) const;

    // ========== 静态工具 ==========

    /**
     * @brief 计算指定星际对应的属性值
     *
     * 公式：stat × STAR_MULTIPLIER^(targetStar - baseStar)
     * 例如 1★ → 2★: atk × 1.8^1
     *      2★ → 3★: atk × 1.8^1（在 2★ 基础上再 ×1.8）
     *
     * @param baseStat   基础属性值（1★ 时的值）
     * @param baseStar   当前星级
     * @param targetStar 目标星级
     * @return 目标星级下的属性值（int，向下取整）
     */
    static int statForStar(int baseStat, int baseStar, int targetStar);
};

#endif // STARSYSTEM_H
