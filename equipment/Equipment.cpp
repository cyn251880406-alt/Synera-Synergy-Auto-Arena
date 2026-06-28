#include "Equipment.h"
#include "unit/Unit.h"

#include <algorithm>  // std::max

// ============================================================================
// Sword — 铁剑（攻击力 +15）
// ============================================================================

QString Sword::name() const
{
    return QStringLiteral("铁剑");
}

QString Sword::typeName() const
{
    return QStringLiteral("Sword");
}

void Sword::apply(Unit &unit) const
{
    // 在原有 ATK 基础上增加固定数值
    unit.setRawAtk(unit.atk() + ATK_BONUS);
}

void Sword::remove(Unit &unit) const
{
    // 反向操作：减去之前加上的数值
    unit.setRawAtk(unit.atk() - ATK_BONUS);
}

// ============================================================================
// Armor — 锁子甲（最大生命值 +200）
// ============================================================================

QString Armor::name() const
{
    return QStringLiteral("锁子甲");
}

QString Armor::typeName() const
{
    return QStringLiteral("Armor");
}

void Armor::apply(Unit &unit) const
{
    unit.setRawMaxHp(unit.maxHp() + HP_BONUS);
}

void Armor::remove(Unit &unit) const
{
    unit.setRawMaxHp(unit.maxHp() - HP_BONUS);
}

// ============================================================================
// Glove — 急速手套（攻击速度提升 20%，攻击间隔 ×0.8）
// ============================================================================

QString Glove::name() const
{
    return QStringLiteral("急速手套");
}

QString Glove::typeName() const
{
    return QStringLiteral("Glove");
}

void Glove::apply(Unit &unit) const
{
    // 攻击间隔帧数 × 4/5（提升 20% 攻速 = 间隔减少 20%）
    //  例如 60 帧 → 48 帧，即每秒攻击从 1 次提升到 1.25 次
    int newSpeed = unit.attackSpeed() * SPEED_NUM / SPEED_DENOM;
    unit.setAttackSpeed(std::max(1, newSpeed));  // 至少 1 帧
}

void Glove::remove(Unit &unit) const
{
    // 反向操作：× 5/4（还原到 × 4/5 之前的值）
    //  例如 48 帧 → 60 帧
    int restored = unit.attackSpeed() * SPEED_DENOM / SPEED_NUM;
    unit.setAttackSpeed(restored);
}

// ============================================================================
// Crystal — 蓝水晶（最大法力值 -30）
// ============================================================================

QString Crystal::name() const
{
    return QStringLiteral("蓝水晶");
}

QString Crystal::typeName() const
{
    return QStringLiteral("Crystal");
}

void Crystal::apply(Unit &unit) const
{
    // 减少最大法力值，让单位更快攒满蓝放技能
    int newMax = unit.maxMana() - MANA_REDUCTION;
    if (newMax < MANA_MINIMUM)
        newMax = MANA_MINIMUM;  // 确保至少有最低法力值
    unit.setRawMaxMana(newMax);
}

void Crystal::remove(Unit &unit) const
{
    // 反向操作：加回之前减去的量
    unit.setRawMaxMana(unit.maxMana() + MANA_REDUCTION);
}

// ============================================================================
// 全局操作接口
// ============================================================================

void equipItem(Unit *unit, Equipment *equipment)
{
    if (!unit || !equipment)
        return;

    // —— 1. 若已有装备，先卸下 ——
    if (unit->equipment())
        removeEquipment(unit);

    // —— 2. 应用装备属性 ——
    equipment->apply(*unit);

    // —— 3. 注册到单位 ——
    unit->setEquipment(equipment);
}

Equipment *removeEquipment(Unit *unit)
{
    if (!unit)
        return nullptr;

    Equipment *eq = unit->equipment();
    if (!eq)
        return nullptr;

    // —— 1. 恢复属性 ——
    eq->remove(*unit);

    // —— 2. 清空装备槽 ——
    unit->setEquipment(nullptr);

    // —— 3. 返回 Equipment*（调用方管理后续生命周期） ——
    return eq;
}
