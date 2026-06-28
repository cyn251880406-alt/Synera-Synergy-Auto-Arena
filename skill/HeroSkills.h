#ifndef HEROSKILLS_H
#define HEROSKILLS_H

#include "Skill.h"

// ============================================================================
// 英雄技能 — 派生类实现（3 职业各 1 技能）
// ============================================================================

// 注意：所有 Skill 派生类均为 stateless（无可变成员数据），
//       cast() 为 const 方法，可跨单位安全共享。

// --------------------------------------------------------------------------
// 1. PowerStrike — 重击（战士：单体物理伤害）
// --------------------------------------------------------------------------

/**
 * @brief PowerStrike — 对目标造成 ATK × 2.5 的单体伤害
 *
 * 目标类型： SingleEnemy（单体敌方）
 * 适用职业： 战士（剑术师 / 狂战士）
 */
class PowerStrike : public Skill {
public:
    QString name() const override;
    SkillTargetType targetType() const override;
    void cast(Unit *caster, const std::vector<Unit *> &targets) const override;

    static constexpr double DAMAGE_MULTIPLIER = 2.5;
};

// --------------------------------------------------------------------------
// 2. IronWall — 铁壁（坦克：自愈 + 护盾）
// --------------------------------------------------------------------------

/**
 * @brief IronWall — 恢复自身 ATK × 2.0 生命值
 *
 * 目标类型： HealFriendly（治疗自身）
 * 适用职业： 坦克（重装兵 / 守护者）
 */
class IronWall : public Skill {
public:
    QString name() const override;
    SkillTargetType targetType() const override;
    void cast(Unit *caster, const std::vector<Unit *> &targets) const override;

    static constexpr double HEAL_MULTIPLIER = 2.0;
};

// --------------------------------------------------------------------------
// 3. ArrowRain — 箭雨（射手：范围 AOE）
// --------------------------------------------------------------------------

/**
 * @brief ArrowRain — 对主目标周围 2 格内所有敌方造成 ATK × 1.5 的 AOE 伤害
 *
 * 目标类型： AOEEnemy（范围敌方）
 * 适用职业： 射手（神射手 / 鹰眼）
 * 设计说明： 射手独有的攻击特效——箭雨覆盖，克制密集站位。
 */
class ArrowRain : public Skill {
public:
    QString name() const override;
    SkillTargetType targetType() const override;
    int aoeRadius() const override { return 2; }
    void cast(Unit *caster, const std::vector<Unit *> &targets) const override;

    static constexpr double DAMAGE_MULTIPLIER = 1.5;
};

#endif // HEROSKILLS_H
