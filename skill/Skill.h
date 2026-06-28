#ifndef SKILL_H
#define SKILL_H

#include <QString>
#include <vector>

class Unit;

// ============================================================================
// SkillTargetType — 技能目标类型枚举
// ============================================================================

/**
 * @brief SkillTargetType — 描述技能的目标选择方式
 *
 * 辅助技能系统确定在施法时应该选谁作为目标。
 * 每种技能在基类中声明其目标类型，由 Unit::castSkill() 根据此枚举
 * 和当前 CombatContext 自动解析目标列表。
 */
enum class SkillTargetType {
    /**
     * @brief 单体敌方
     *
     * 对当前攻击锁定的目标造成伤害。
     * 目标 = { m_currentTarget }
     * 适用：单体攻击技能（如重击、奥术飞弹）
     */
    SingleEnemy,

    /**
     * @brief 范围敌方（以主目标为中心）
     *
     * 对主目标周围一定半径内的所有敌方单位造成伤害。
     * 目标 = { enemies within AOE_RADIUS of primaryTarget.position() }
     * 适用：以目标位置为中心的范围技能（如火球术）
     */
    AOEEnemy,

    /**
     * @brief 范围敌方（以施法者自身为中心）
     *
     * 对施法者自身周围一定半径内的所有敌方单位造成伤害。
     * 目标 = { enemies within AOE_RADIUS of caster.position() }
     * 适用：以自身为中心的溅射技能（如旋风斩、战争践踏）
     */
    AOEEnemyAroundSelf,

    /**
     * @brief 治疗友方
     *
     * 在己方存活单位中选取血量百分比最低的进行回血。
     * 目标 = { lowestHpAlly }
     * 适用：治疗技能（如圣光术）
     */
    HealFriendly
};

// ============================================================================
// Skill — 技能基类
// ============================================================================

/**
 * @brief Skill — 所有技能的抽象基类
 *
 * 使用多态实现不同技能效果。在 Synera 中，每个英雄单位拥有一个 Skill
 * 对象（组合关系），满蓝时自动触发。
 *
 * 多态设计：
 *   每个派生类重写 name()、targetType() 和 cast() 以实现独特效果。
 *   派生类应当在构造时确认目标类型和技能常量（如伤害倍率、AOE 半径等），
 *   不要通过运行期参数改变这些属性。
 *
 * 线程安全性：所有 Skill 对象应为 stateless（无可变成员状态），
 *   cast() 为 const 方法，使其可以安全地跨单位共享。
 *
 * 存放建议：技能对象应在 main.cpp 中以静态/栈分配方式创建，
 *   生命周期覆盖整个游戏过程，通过 Unit::setSkill() 与单位绑定。
 *
 * 用法示例：
 * @code
 *   // 技能实现
 *   class PowerStrike : public Skill {
 *       QString name() const override { return QStringLiteral("重击"); }
 *       SkillTargetType targetType() const override { return SkillTargetType::SingleEnemy; }
 *       void cast(Unit* caster, const std::vector<Unit*>& targets) const override {
 *           for (Unit* t : targets)
 *               t->takeDamage(caster->atk() * 2.5);
 *       }
 *   };
 *
 *   // 绑定到单位
 *   warrior->setSkill(&powerStrike);
 * @endcode
 */
class Skill {
public:
    // ========== 构造/析构 ==========

    Skill() = default;
    virtual ~Skill() = default;

    // ========== 虚方法 ==========

    /// 返回技能名称（用于 UI 显示）
    virtual QString name() const = 0;

    /// 返回目标选择类型（决定 cast() 的 targets 参数如何填充）
    virtual SkillTargetType targetType() const = 0;

    /**
     * @brief 返回 AOE 技能半径（格数）
     *
     * 仅对 targetType() 为 AOEEnemy 或 AOEEnemyAroundSelf 的技能有意义。
     * 基类默认返回 0，派生类根据实际情况重写。
     * Unit::castSkill() 在解析 AOE 目标时调用此方法获取半径。
     *
     * @return 以棋盘格为单位的 AOE 半径
     */
    virtual int aoeRadius() const { return 0; }

    /**
     * @brief 执行技能效果
     *
     * 纯虚函数，每个派生类实现自己的效果逻辑。
     * 不应修改施法者之外的游戏状态（如 BattleSystem 状态）。
     *
     * @param caster  施法者（自身）
     * @param targets 技能目标列表（已由调用方按 targetType() 解析）
     *
     * 目标解析规则（由 Unit::castSkill() 在调用本方法前完成）：
     *   - SingleEnemy:          targets = { currentTarget }
     *   - AOEEnemy:             targets = enemies within AOE_RADIUS of primaryTarget
     *   - AOEEnemyAroundSelf:   targets = enemies within AOE_RADIUS of caster
     *   - HealFriendly:         targets = { lowestHpAlly }
     */
    virtual void cast(Unit *caster, const std::vector<Unit *> &targets) const = 0;
};

#endif // SKILL_H
