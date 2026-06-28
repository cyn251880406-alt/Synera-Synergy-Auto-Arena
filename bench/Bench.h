#ifndef BENCH_H
#define BENCH_H

#include <array>

class Unit;

// ============================================================================
// Bench — 备战区（逻辑层）
// ============================================================================

/**
 * @brief Bench — 备战区，管理玩家已购买但尚未上阵的单位
 *
 * 维护 8 个槽位（std::array<Unit*, 8>），提供存入/移除/交换操作。
 * 纯逻辑层，不管理 UnitItem，不拥有 Unit 所有权。
 */
class Bench {
public:
    // ========== 常量 ==========

    /// 备战区槽位数
    static constexpr int CAPACITY = 8;

    // ========== 构造/析构 ==========

    Bench();
    ~Bench() = default;

    // ========== 容量查询 ==========

    /// 返回槽位总数（固定为 8）
    int size() const { return CAPACITY; }

    /// 当前已占用的槽位数
    int count() const;

    /// 备战区是否已满（所有 8 个槽位都被占用）
    bool isFull() const;

    /// 是否存在至少一个空槽位
    bool hasEmptySlot() const;

    /// 返回第一个空槽位的索引 [0, 7]；若已满则返回 -1
    int firstEmptySlot() const;

    // ========== 单位存取 ==========

    /**
     * @brief 将单位放入备战区的第一个空槽位
     *
     * 内部调用 firstEmptySlot() 找到空槽位并放入。
     * 若备战区已满，操作失败。
     *
     * @param unit 要放入的单位指针（不能为 nullptr）
     * @return 单位被放入的槽位索引 [0, 7]；若已满或 unit 为空则返回 -1
     */
    int addUnit(Unit *unit);

    /**
     * @brief 将单位放置到指定槽位（覆盖该槽位原有单位）
     *
     * 与 addUnit() 不同，此方法直接写入目标槽位而非寻找第一个空位。
     * 用于跨控件拖拽时精确控制槽位位置。
     *
     * @param slot 目标槽位索引 [0, CAPACITY)
     * @param unit 要放入的单位指针（不能为 nullptr）
     * @return true 放置成功；false 索引越界或 unit 为空
     */
    bool setUnit(int slot, Unit *unit);

    /**
     * @brief 从指定槽位移除单位
     *
     * 将该槽位置为 nullptr。不会 delete 该单位——
     * 调用方（通常是 Game）负责单位的生命周期。
     *
     * @param index 槽位索引 [0, CAPACITY)
     * @return 被移除的单位指针；若该槽位本就为空则返回 nullptr
     */
    Unit *removeUnit(int index);

    /**
     * @brief 获取指定槽位上的单位（不移除）
     * @param index 槽位索引 [0, CAPACITY)
     * @return 单位指针；若槽位为空则返回 nullptr
     */
    Unit *getUnit(int index) const;

    // ========== 操作 ==========

    /**
     * @brief 交换两个槽位上的单位
     *
     * 若两个索引相同，操作无效果。
     * 支持其中一槽或两槽为空的交换（空槽的指针为 nullptr）。
     *
     * @param a 第一个槽位索引
     * @param b 第二个槽位索引
     */
    void swapUnits(int a, int b);

    /**
     * @brief 查找指定单位所在的槽位
     * @param unit 要查找的单位指针
     * @return 所在槽位索引 [0, 7]；未找到则返回 -1
     */
    int indexOf(const Unit *unit) const;

private:
    // ---- 数据 ----

    /// 8 个槽位的数组，每个槽位存放一个 Unit* 或 nullptr
    std::array<Unit *, CAPACITY> m_slots;

    // ---- 内部辅助 ----

    /// 检查索引是否在有效范围 [0, CAPACITY) 内
    bool isValidIndex(int index) const;
};

#endif // BENCH_H
