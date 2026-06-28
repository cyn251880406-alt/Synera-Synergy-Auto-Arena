#include "HeroSkills.h"
#include "unit/Unit.h"

// ============================================================================
// PowerStrike — 重击（战士：单体物理伤害）
// ============================================================================

QString PowerStrike::name() const
{
    return QStringLiteral("重击");
}

SkillTargetType PowerStrike::targetType() const
{
    return SkillTargetType::SingleEnemy;
}

void PowerStrike::cast(Unit *caster,
                        const std::vector<Unit *> &targets) const
{
    if (!caster || targets.empty())
        return;

    int damage = static_cast<int>(caster->atk() * DAMAGE_MULTIPLIER);

    for (Unit *target : targets) {
        if (target && target->isAlive())
            target->takeDamage(damage);
    }
}

// ============================================================================
// IronWall — 铁壁（坦克：自愈）
// ============================================================================

QString IronWall::name() const
{
    return QStringLiteral("铁壁");
}

SkillTargetType IronWall::targetType() const
{
    // 以自身为目标回血
    return SkillTargetType::HealFriendly;
}

void IronWall::cast(Unit *caster,
                     const std::vector<Unit *> &targets) const
{
    if (!caster || !caster->isAlive())
        return;

    // IronWall heals the caster itself
    int healAmount = static_cast<int>(caster->atk() * HEAL_MULTIPLIER);
    caster->heal(healAmount);
}

// ============================================================================
// ArrowRain — 箭雨（射手：范围 AOE）
// ============================================================================

QString ArrowRain::name() const
{
    return QStringLiteral("箭雨");
}

SkillTargetType ArrowRain::targetType() const
{
    return SkillTargetType::AOEEnemy;
}

void ArrowRain::cast(Unit *caster,
                     const std::vector<Unit *> &targets) const
{
    if (!caster || targets.empty())
        return;

    int damage = static_cast<int>(caster->atk() * DAMAGE_MULTIPLIER);

    for (Unit *target : targets) {
        if (target && target->isAlive())
            target->takeDamage(damage);
    }
}
