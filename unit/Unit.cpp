#include "Unit.h"
#include "skill/Skill.h"
#include <algorithm>   // std::max, std::min_element
#include <limits>      // std::numeric_limits
#include <cmath>       // std::sqrt
// ============================================================================
// 构造与析构
// ============================================================================
/**
 * 构造时：
 *   - 初始 HP = MaxHP（满血出场）
 *   - 初始 Mana = 0（从零开始攒蓝）
 *   - 初始状态 = Idle
 *   - 攻击冷却初始为 0（落地即可攻击）
 *   - 攻击间隔 = attackSpeed（默认 60 帧 ≈ 1 秒）
 *   - 默认 1 星，无锁定目标
 */
Unit::Unit(const QString &name, Owner owner, const QPoint &pos,
           int maxHp, int atk, int range, int maxMana,
           int attackSpeed)
    : m_name(name)
    , m_hp(maxHp)
    , m_maxHp(maxHp)
    , m_atk(atk)
    , m_range(range)
    , m_mana(0)
    , m_maxMana(maxMana)
    , m_position(pos)
    , m_owner(owner)
    , m_state(UnitState::Idle)
    , m_attackSpeed(attackSpeed)
    , m_attackCooldown(0)           // 初始即可攻击
    , m_attackCooldownMax(attackSpeed)
    , m_currentTarget(nullptr)
    , m_starLevel(1)
{
}
// ============================================================================
// 战斗接口
// ============================================================================
void Unit::takeDamage(int damage)
{
    // 已死亡的单位不再受伤害（避免重复击杀）
    if (m_state == UnitState::Dead)
        return;

    // 伤害值不能为负
    if (damage < 0)
        damage = 0;

    m_hp -= damage;

    // 血量归零 → Dead（通过 FSM 确保状态转换合法性）
    if (m_hp <= 0) {
        m_hp = 0;
        changeState(UnitState::Dead);
    }
}

void Unit::attack(Unit &target)
{
    // —— 前置条件校验 ——
    if (!isAlive())
        return;
    if (!target.isAlive())
        return;

    // —— 1. 锁定目标 ——
    // 持续攻击同一目标，直到目标死亡或超出范围
    m_currentTarget = &target;

    // —— 2. FSM: Idle → Attacking（进入攻击冷却期） ——
    changeState(UnitState::Attacking);

    // —— 2b. 标记攻击闪光（供 UnitItem 视觉动画用） ——
    m_justAttacked = true;

    // —— 3. 对目标造成 ATK 点伤害 ——
    target.takeDamage(m_atk);

    // —— 4. 普攻回蓝 ——
    addMana(MANA_PER_ATTACK);

    // —— 5. 重置攻击冷却到攻击间隔 ——
    // 攻击间隔由 m_attackSpeed 决定（默认 60 帧 ≈ 1 秒）
    m_attackCooldown = m_attackCooldownMax;

    // —— 6. 法力满时触发技能 ——
    // FSM 路径：Attacking → Casting → Idle
    // Casting 为瞬时状态，技能执行后立即回到 Idle，等待 BattleSystem 下一帧重新索敌
    if (m_mana >= m_maxMana) {
        changeState(UnitState::Casting);    // Attacking → Casting
        castSkill(target);                   // 执行技能效果
        m_mana = 0;                          // 释放技能后法力清零
        m_justCasted = true;                 // 标记技能释放（BattleEffectOverlay 用）
        changeState(UnitState::Idle);        // Casting → Idle（技能就绪态）
    }

    // 注意：若法力未满，不在此处回到 Idle！
    //       冷却期间保持 Attacking 状态，
    //       冷却结束或目标死亡后由 updateState() 自动 → Idle
}

void Unit::update()
{
    // Dead 状态不执行任何逻辑
    if (m_state == UnitState::Dead)
        return;

    // 逐帧递减攻击冷却（所有非 Dead 状态共享）
    if (m_attackCooldown > 0)
        --m_attackCooldown;

    // 按当前状态分发行为（FSM 核心）
    updateState();
}

// ============================================================================
// 有限状态机 (FSM) 实现
// ============================================================================

void Unit::changeState(UnitState newState)
{
    // —— 合法性校验 ——

    // 规则：死亡状态不可逆（防止"死而复生"）
    if (m_state == UnitState::Dead && newState != UnitState::Dead)
        return;

    // 规则：不允许原地踏步
    if (m_state == newState)
        return;

    // —— 旧状态退出（on-exit） ——
    switch (m_state) {
    case UnitState::Attacking:
        // 离开 Attacking 时不做特殊清理
        // 冷却计时器在 update() 中已递减
        break;
    default:
        break;
    }

    // —— 执行转换 ——
    m_state = newState;

    // —— 新状态进入（on-enter） ——
    switch (newState) {
    case UnitState::Idle:
        // 进入 Idle：准备接受 BattleSystem 的索敌指令
        break;

    case UnitState::Attacking:
        // 进入 Attacking：开始攻击冷却
        // 实际攻击行为（伤害/回蓝）在 attack() 中已执行
        break;

    case UnitState::Casting:
        // 进入 Casting：技能即将执行
        // 实际技能在 attack() 中 changeState(Casting) 之后立即调用 castSkill()
        // 此 on-enter 钩子供未来添加施法前摇/动画触发
        break;

    case UnitState::Dead:
        // 进入 Dead：HP 归零，不再参与战斗
        // 清理操作（如果有）在此处执行
        break;

    default:
        break;
    }
}

void Unit::updateState()
{
    // 按当前状态分发每帧行为
    switch (m_state) {

    case UnitState::Idle:
        // Idle 状态：等待 BattleSystem 分配目标
        // Unit 自身不主动索敌——索敌由 BattleSystem::updateLoop() 统一调度
        // 此处仅作为占位，若派生类有持续效果（如回血光环）可在此处理
        break;

    case UnitState::Attacking:
        // —— 目标死亡检测 ——
        // 若锁定目标已死亡，清空目标并立即回到 Idle
        // BattleSystem 下一帧会重新索敌分配新目标
        if (m_currentTarget && !m_currentTarget->isAlive()) {
            m_currentTarget = nullptr;
            changeState(UnitState::Idle);
            break;
        }

        // —— 冷却计时 ——
        // 冷却结束后回到 Idle，准备下一次攻击
        if (m_attackCooldown <= 0) {
            // 目标仍然存活 → 保持锁定，下一帧继续攻击
            changeState(UnitState::Idle);
        }
        break;

    case UnitState::Dead:
        // Dead 状态：已由 update() 提前过滤，不应到达此处
        break;

    case UnitState::Moving:
        // 移动逻辑由 Pathfinder::moveStep() 统一处理（含占用检测）
        // 此处仅做防御：路径意外为空时回到 Idle
        if (m_path.empty() && m_moveTimer <= 0) {
            changeState(UnitState::Idle);
        }
        break;

    case UnitState::Casting:
        // 正常路径：Casting → Idle 已在 attack() 中 changeState(Idle) 立即完成
        // 防御路径：若因异常到达此处（如外部直接设态），回到 Idle 保持可运行
        changeState(UnitState::Idle);
        break;
    }
}

// ============================================================================
// 状态与工具方法
// ============================================================================

bool Unit::isAlive() const
{
    return m_hp > 0 && m_state != UnitState::Dead;
}

bool Unit::canAttack() const
{
    // 必须在 Idle 状态 + 冷却完毕 = 可以发起攻击
    // 注：Attacking 状态表示正在冷却中，不可攻击
    return m_state == UnitState::Idle && m_attackCooldown == 0;
}

void Unit::setPosition(const QPoint &pos)
{
    m_position = pos;
}

void Unit::addMana(int amount)
{
    if (!isAlive())
        return;

    m_mana += amount;

    // 法力值不能超过上限
    if (m_mana > m_maxMana)
        m_mana = m_maxMana;
}

void Unit::castSkill(Unit &target)
{
    // —— 无技能绑定 → 空操作 ——
    if (!m_skill)
        return;

    // —— 根据技能的目标类型解析目标列表 ——
    std::vector<Unit *> skillTargets;

    switch (m_skill->targetType()) {

    case SkillTargetType::SingleEnemy:
        // 单体伤害：目标 = 当前攻击锁定目标
        if (target.isAlive())
            skillTargets.push_back(&target);
        break;

    case SkillTargetType::AOEEnemy: {
        // 范围伤害（以主目标为中心）：选取距离主目标 AOE_RADIUS 内的所有敌方
        if (!target.isAlive())
            break;

        QPoint tp = target.position();
        int aoeRadius = m_skill->aoeRadius();
        int radiusSq = aoeRadius * aoeRadius;

        for (Unit *e : m_visibleEnemies) {
            if (!e->isAlive())
                continue;
            QPoint ep = e->position();
            int dx = tp.x() - ep.x();
            int dy = tp.y() - ep.y();
            if (dx * dx + dy * dy <= radiusSq)
                skillTargets.push_back(e);
        }
        break;
    }

    case SkillTargetType::AOEEnemyAroundSelf: {
        // 范围伤害（以自身为中心）：选取距离施法者 AOE_RADIUS 内的所有敌方
        QPoint cp = m_position;
        int aoeRadius = m_skill->aoeRadius();
        int radiusSq = aoeRadius * aoeRadius;

        for (Unit *e : m_visibleEnemies) {
            if (!e->isAlive())
                continue;
            QPoint ep = e->position();
            int dx = cp.x() - ep.x();
            int dy = cp.y() - ep.y();
            if (dx * dx + dy * dy <= radiusSq)
                skillTargets.push_back(e);
        }
        break;
    }

    case SkillTargetType::HealFriendly: {
        // 治疗：选取己方血量百分比最低的单位
        Unit *lowestHpAlly = nullptr;
        int minHp = std::numeric_limits<int>::max();

        for (Unit *a : m_visibleAllies) {
            if (!a->isAlive())
                continue;
            if (a->hp() < minHp) {
                minHp = a->hp();
                lowestHpAlly = a;
            }
        }

        if (lowestHpAlly)
            skillTargets.push_back(lowestHpAlly);
        break;
    }
    }

    // —— 无可选目标 → 跳过施法 ——
    if (skillTargets.empty())
        return;

    // —— 保存目标列表（供 Game 生成技能特效） ——
    m_lastSkillTargets = skillTargets;

    // —— 执行技能效果 ——
    m_skill->cast(this, skillTargets);
}

// ============================================================================
// 回血
// ============================================================================

void Unit::heal(int amount)
{
    if (!isAlive())
        return;

    if (amount < 0)
        amount = 0;

    m_hp += amount;

    if (m_hp > m_maxHp)
        m_hp = m_maxHp;
}

void Unit::reviveAfterBattle()
{
    if (m_state != UnitState::Dead)
        return;

    m_hp = 1;           // 直接设置，绕过 isAlive() 守卫
    m_state = UnitState::Idle;  // 绕过 changeState 的 Dead 守卫
    m_currentTarget = nullptr;
    clearPath();
    m_moveTimer = 0;
    m_attackCooldown = 0;
}

// ============================================================================
// 战斗上下文
// ============================================================================

void Unit::setCombatContext(const std::vector<Unit *> &allies,
                             const std::vector<Unit *> &enemies)
{
    // 深拷贝——调用方（BattleSystem）的列表每帧重新生成
    m_visibleAllies   = allies;
    m_visibleEnemies  = enemies;
}

// ============================================================================
// 寻路与移动
// ============================================================================

void Unit::setPath(const std::vector<QPoint> &newPath)
{
    m_path = newPath;
    // 新路径，重置移动计时器（立即可移动第一格）
    m_moveTimer = 0;
}

void Unit::clearPath()
{
    m_path.clear();
    // 不重置计时器——让调用方决定后续行为
}

QPoint Unit::nextPathCell() const
{
    if (m_path.empty())
        return QPoint(-1, -1);
    return m_path.front();
}

void Unit::popPathCell()
{
    if (!m_path.empty())
        m_path.erase(m_path.begin());
}

void Unit::setAttackSpeed(int speed)
{
    // 攻击间隔至少为 1 帧
    if (speed < 1)
        speed = 1;

    m_attackSpeed       = speed;
    m_attackCooldownMax = speed;

    // 若当前冷却超过新上限，同步削减
    if (m_attackCooldown > m_attackCooldownMax)
        m_attackCooldown = m_attackCooldownMax;
}

void Unit::setStarLevel(int level)
{
    // 星阶至少为 1
    m_starLevel = (level < 1) ? 1 : level;
}

void Unit::addTrait(Trait t)
{
    // 避免重复添加相同标签
    if (!m_traits.contains(t))
        m_traits.append(t);
}
