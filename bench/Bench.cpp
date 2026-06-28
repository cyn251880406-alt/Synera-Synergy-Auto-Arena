#include "bench/Bench.h"
#include <algorithm> // std::find

// ============================================================================
// 构造
// ============================================================================

Bench::Bench()
{
    // 所有槽位初始为空
    m_slots.fill(nullptr);
}

// ============================================================================
// 容量查询
// ============================================================================

int Bench::count() const
{
    // 统计非空槽位数
    int n = 0;
    for (const Unit *u : m_slots) {
        if (u != nullptr)
            ++n;
    }
    return n;
}

bool Bench::isFull() const
{
    // 所有槽位都非空 → 已满
    for (const Unit *u : m_slots) {
        if (u == nullptr)
            return false;   // 找到一个空位就未满
    }
    return true;
}

bool Bench::hasEmptySlot() const
{
    return !isFull();
}

int Bench::firstEmptySlot() const
{
    // 从头遍历，返回第一个空槽位的索引
    for (int i = 0; i < CAPACITY; ++i) {
        if (m_slots[i] == nullptr)
            return i;
    }
    return -1;  // 已满
}

// ============================================================================
// 单位存取
// ============================================================================

int Bench::addUnit(Unit *unit)
{
    // 空指针拒绝
    if (unit == nullptr)
        return -1;

    // 防止重复添加（同一单位不应占用两个槽位）
    if (indexOf(unit) != -1)
        return -1;

    // 查找第一个空位
    int slot = firstEmptySlot();
    if (slot == -1)
        return -1;          // 已满

    m_slots[slot] = unit;
    return slot;
}

bool Bench::setUnit(int slot, Unit *unit)
{
    // 索引越界或空指针拒绝
    if (!isValidIndex(slot) || unit == nullptr)
        return false;

    // 防止重复添加（同一单位不应占用两个槽位）
    // 但允许覆盖自己所在的槽位
    int existingSlot = indexOf(unit);
    if (existingSlot >= 0 && existingSlot != slot)
        return false;

    m_slots[slot] = unit;
    return true;
}

Unit *Bench::removeUnit(int index)
{
    // 索引越界检查
    if (!isValidIndex(index))
        return nullptr;

    Unit *removed = m_slots[index];
    m_slots[index] = nullptr;       // 清空槽位，不 delete
    return removed;
}

Unit *Bench::getUnit(int index) const
{
    if (!isValidIndex(index))
        return nullptr;

    return m_slots[index];
}

// ============================================================================
// 操作
// ============================================================================

void Bench::swapUnits(int a, int b)
{
    if (!isValidIndex(a) || !isValidIndex(b))
        return;

    // 使用 std::swap 交换两个槽位的指针
    std::swap(m_slots[a], m_slots[b]);
}

int Bench::indexOf(const Unit *unit) const
{
    if (unit == nullptr)
        return -1;

    for (int i = 0; i < CAPACITY; ++i) {
        if (m_slots[i] == unit)
            return i;
    }
    return -1;
}

// ============================================================================
// 内部辅助
// ============================================================================

bool Bench::isValidIndex(int index) const
{
    return index >= 0 && index < CAPACITY;
}
