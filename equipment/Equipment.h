#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QString>

class Unit;

// ============================================================================
// Equipment — 装备基类
// ============================================================================

/**
 * @brief Equipment — 所有装备的抽象基类
 *
 * 每个单位最多穿戴 1 件装备。装备提供属性加成。
 * 纯虚接口: apply()/remove()/name()/typeName()。
 * stateless 设计，可跨单位共享。
 */
class Equipment {
public:
    Equipment() = default;
    virtual ~Equipment() = default;

    /// 返回装备名称（用于 UI 显示）
    virtual QString name() const = 0;

    /**
     * @brief 装备时调用，修改单位属性
     *
     * 直接调用 Unit::setRawAtk / setRawMaxHp / setRawMaxMana / setAttackSpeed
     * 等公开 setter 修改属性值。
     *
     * @param unit 要装备的目标单位
     */
    virtual void apply(Unit &unit) const = 0;

    /**
     * @brief 卸下时调用，恢复单位属性
     *
     * 反向操作 apply 中对属性的修改。
     * 调用方需确保 apply 和 remove 的调用次数配对，防止数值偏移。
     *
     * @param unit 要卸下的目标单位
     */
    virtual void remove(Unit &unit) const = 0;

    /**
     * @brief 返回装备类型标识符（用于存档/读档）
     *
     * 返回稳定的英文标识符，与显示名称 name() 无关。
     *   铁剑 → "Sword"    锁子甲 → "Armor"
     *   急速手套 → "Glove" 蓝水晶 → "Crystal"
     *
     * @return 类型标识符（常量字符串）
     */
    virtual QString typeName() const = 0;
};

// ============================================================================
// 派生装备类
// ============================================================================

// --------------------------------------------------------------------------
// 1. Sword — 铁剑（攻击力 +15）
// --------------------------------------------------------------------------

/**
 * @brief Sword — 铁剑，攻击力 +15
 */
class Sword : public Equipment {
public:
    QString name() const override;
    QString typeName() const override;
    void apply(Unit &unit) const override;
    void remove(Unit &unit) const override;

    /// 攻击力加成值
    static constexpr int ATK_BONUS = 15;
};

// --------------------------------------------------------------------------
// 2. Armor — 锁子甲（生命值 +200）
// --------------------------------------------------------------------------

/**
 * @brief Armor — 锁子甲，最大生命值 +200
 */
class Armor : public Equipment {
public:
    QString name() const override;
    QString typeName() const override;
    void apply(Unit &unit) const override;
    void remove(Unit &unit) const override;

    /// 生命值加成值
    static constexpr int HP_BONUS = 200;
};

// --------------------------------------------------------------------------
// 3. Glove — 急速手套（攻击速度提升 20%）
// --------------------------------------------------------------------------

/**
 * @brief Glove — 急速手套，攻击速度提升 20%（攻击间隔 × 0.8）
 */
class Glove : public Equipment {
public:
    QString name() const override;
    QString typeName() const override;
    void apply(Unit &unit) const override;
    void remove(Unit &unit) const override;

    /// 攻击间隔倍率（整数运算：× 4 / 5 = × 0.8）
    static constexpr int SPEED_NUM   = 4;
    static constexpr int SPEED_DENOM = 5;
};

// --------------------------------------------------------------------------
// 4. Crystal — 蓝水晶（最大法力值 -30）
// --------------------------------------------------------------------------

/**
 * @brief Crystal — 蓝水晶，最大法力值 -30（最低保留 20）
 */
class Crystal : public Equipment {
public:
    QString name() const override;
    QString typeName() const override;
    void apply(Unit &unit) const override;
    void remove(Unit &unit) const override;

    /// 法力值减少量
    static constexpr int MANA_REDUCTION = 30;
    /// 法力值下限（防止减到 0 导致无法放技能）
    static constexpr int MANA_MINIMUM = 20;
};

// ============================================================================
// 全局操作接口
// ============================================================================

/**
 * @brief 为单位装备一件物品
 *
 * 执行步骤：
 *   1. 若单位已有装备，先卸下（自动恢复属性）
 *   2. 调用 equipment->apply(unit) 修改属性
 *   3. 注册 equipment 到 unit->setEquipment()
 *
 * 不管理 Equipment 的生命周期——调用方保证 equipment 指针在装备期间有效。
 *
 * @param unit       目标单位（不能为 nullptr）
 * @param equipment  要装备的物品（不能为 nullptr）
 */
void equipItem(Unit *unit, Equipment *equipment);

/**
 * @brief 从单位卸下当前装备
 *
 * 执行步骤：
 *   1. 调用 equipment->remove(unit) 恢复属性
 *   2. 清空 unit->setEquipment(nullptr)
 *   3. 返回 equipment 指针（调用方可回收或放入装备池）
 *
 * @param unit 目标单位（不能为 nullptr）
 * @return 被卸下的 Equipment*；若单位无装备则返回 nullptr
 */
Equipment *removeEquipment(Unit *unit);

#endif // EQUIPMENT_H
