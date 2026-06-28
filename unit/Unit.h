#ifndef UNIT_H
#define UNIT_H
#include <QPoint>
#include <QString>
#include <QVector>
#include <vector>
class Skill;  // 前向声明，技能系统通过组合方式集成
class Equipment;  // 前向声明，装备系统通过组合方式集成
// ============================================================================
// 枚举定义
// ============================================================================
/**
 * @brief Owner — 区分单位归属（敌我）
 *
 * 我方与敌方单位本质上是同一类 Unit，仅通过 owner 字段区分。
 */
enum class Owner {
    PlayerCtrl, ///< 我方控制
    EnemyCtrl   ///< 敌方控制
};

/**
 * @brief UnitState — 单位战斗状态机
 *
 * 每个单位在任何时刻处于其中一种状态，构成完整的 FSM。
 */
enum class UnitState {
    Idle,      ///< 空闲——等待索敌或指令
    Moving,    ///< 移动中——正在向目标格移动
    Attacking, ///< 攻击中——普攻前摇/后摇
    Casting,   ///< 施法中——技能释放
    Dead       ///< 死亡——HP ≤ 0，不再参与战斗
};

/**
 * @brief Trait — 羁绊标签
 *
 * 羁绊标签用于羁绊系统的计数与效果激活。
 */
enum class Trait {
    None,       ///< 无标签（占位）
    Archer,     ///< 射手 — 远程高输出
    Tank,       ///< 坦克 — 高血量防御
    Warrior     ///< 战士 — 近战均衡
};

// ============================================================================
// Unit 基类
// ============================================================================

/**
 * @brief Unit — 战斗单位基类
 *
 * 所有英雄/敌方单位的公共基类，定义自走棋中一个单位的基础属性与行为接口。
 * 派生类可重写 attack() 以实现多态技能。
 */
class Unit {
public:
    // ========== 构造/析构 ==========
    /**
     * @brief 构造一个战斗单位
     * @param name        单位名称（如"战士"、"法师"）
     * @param owner       控制归属（我方/敌方）
     * @param pos         初始棋盘坐标
     * @param maxHp       最大生命值
     * @param atk         攻击力
     * @param range       攻击距离（单位：格，1 为近战）
     * @param maxMana     最大法力值（普攻回蓝，满蓝放技能）
     * @param attackSpeed 攻击间隔帧数（默认 60 帧 ≈ 1 秒 @ 60fps，越小越快）
     */
    Unit(const QString &name, Owner owner, const QPoint &pos,
         int maxHp, int atk, int range, int maxMana,
         int attackSpeed = 60);

    /// 虚析构——保证派生类对象能被正确析构
    virtual ~Unit() = default;

    // ========== 属性访问器（getter） ==========

    QString   name()     const { return m_name;     }
    int       hp()       const { return m_hp;       }
    int       maxHp()    const { return m_maxHp;    }
    int       atk()      const { return m_atk;      }

    /// 直接设置攻击力（羁绊/装备系统战斗前修改，战斗后恢复）
    /// 直接设置当前生命值（存档/装备系统使用）
    void setRawHp(int hp) { m_hp = (hp < 0) ? 0 : hp; }
    void setRawAtk(int atk) { m_atk = atk; }
    /// 直接设置最大生命值（羁绊/装备系统使用）
    void setRawMaxHp(int maxHp) { m_maxHp = maxHp; }
    /// 直接设置最大法力值（装备系统使用，如蓝水晶减最大法力）
    void setRawMaxMana(int maxMana) { m_maxMana = maxMana; }
    /// 直接设置当前法力值（存档系统使用）
    void setRawMana(int mana) { m_mana = (mana < 0) ? 0 : (mana > m_maxMana ? m_maxMana : mana); }
    /// 当前攻击冷却（存档系统查询用）
    int  attackCooldown() const { return m_attackCooldown; }
    /// 直接设置攻击冷却（存档系统使用）
    void setRawAttackCooldown(int cd) { m_attackCooldown = (cd < 0) ? 0 : cd; }
    int       range()    const { return m_range;    }
    int       mana()     const { return m_mana;     }
    int       maxMana()  const { return m_maxMana;  }
    QPoint    position() const { return m_position; }
    Owner     owner()    const { return m_owner;    }

    UnitState state()    const { return m_state;    }
    int       starLevel()const { return m_starLevel;}

    /// 返回羁绊标签列表（const 引用，避免拷贝）
    const QVector<Trait> &traits() const { return m_traits; }

    // ========== 自动攻击属性 ==========

    /// 攻击间隔帧数（越小攻击越快，默认 60 帧 ≈ 1 秒）
    int  attackSpeed()    const { return m_attackSpeed; }
    /// 设置攻击间隔（同时更新内部冷却上限）
    void setAttackSpeed(int speed);

    /// 当前锁定的攻击目标（nullptr 表示无目标）
    Unit *currentTarget() const { return m_currentTarget; }
    /// 锁定/切换攻击目标
    void  setTarget(Unit *target) { m_currentTarget = target; }

    /// 本帧是否刚执行了攻击（供 UnitItem 触发攻击闪光动画）
    bool justAttacked() const { return m_justAttacked; }
    /// 清除攻击标志（由 Game 在触发视觉动画后调用）
    void clearJustAttacked() { m_justAttacked = false; }

    /// 本帧是否刚释放了技能（供 BattleEffectOverlay 触发技能特效）
    bool justCasted() const { return m_justCasted; }
    void clearJustCasted() { m_justCasted = false; }

    /// 最近一次技能释放的目标列表（供 Game 生成技能特效）
    const std::vector<Unit *> &lastSkillTargets() const { return m_lastSkillTargets; }

    // ========== 寻路与移动 ==========

    /// 当前移动路径（从未走过的格子序列，不含已到达的格子）
    const std::vector<QPoint> &path() const { return m_path; }
    /// 是否有待完成的移动路径
    bool hasPath() const { return !m_path.empty(); }
    /// 设置移动路径（由 Pathfinder::findPath 计算，BattleSystem 调用）
    void setPath(const std::vector<QPoint> &newPath);
    /// 清空移动路径
    void clearPath();
    /// 查看路径中的下一格（不弹出）
    QPoint nextPathCell() const;
    /// 弹出路径中的下一格（到达该格后调用）
    void popPathCell();

    /// 当前移动计时器（帧数，归零时移动到下一格）
    int  moveTimer() const { return m_moveTimer; }
    /// 设置移动计时器
    void setMoveTimer(int timer) { m_moveTimer = timer; }
    /// 递减移动计时器（每帧调用）
    void decrementMoveTimer() { if (m_moveTimer > 0) --m_moveTimer; }

    // ========== 战斗接口 ==========

    /**
     * @brief 受到伤害
     *
     * 将当前 HP 减去 damage 的值（最低为 0）。
     * 若 HP 降至 0，自动将状态切换为 Dead。
     *
     * @param damage 受到的伤害值（非负）
     */
    void takeDamage(int damage);

    /**
     * @brief 对目标单位发动一次普通攻击（自动攻击系统核心）
     *
     * 前置条件（由 BattleSystem 在调用前校验）：
     *   - 自身处于 Idle 状态
     *   - 冷却计时器已归零（canAttack() == true）
     *   - 目标在攻击距离内
     *
     * FSM 驱动流程：
     *   1. 锁定目标（m_currentTarget = &target）
     *   2. Idle → Attacking（进入攻击冷却期）
     *   3. 对 target 造成 m_atk 点伤害
     *   4. 回复 MANA_PER_ATTACK 点法力
     *   5. 重置冷却计时器 = m_attackSpeed
     *   6. 冷却期间保持 Attacking 状态
     *   7. 冷却结束后由 updateState() 自动 Attacking → Idle
     *   8. 若期间目标死亡，updateState() 提前清除目标并回到 Idle
     *
     * 派生类可重写此方法以添加额外攻击效果（如暴击、溅射等）。
     *
     * @param target 被攻击的目标单位
     */
    virtual void attack(Unit &target);

    /**
     * @brief 每帧更新（BattleSystem::updateLoop 调用）
     *
     * 自动攻击流程：
     *   1. Dead 检查 → 死亡单位直接返回
     *   2. 递减攻击冷却计时器（m_attackCooldown）
     *   3. 调用 updateState() 按状态分发行为
     *   4. 派生类可重写以添加持续效果
     */
    virtual void update();

    /**
     * @brief 释放技能
     *
     * 当法力值达到 m_maxMana 时由 attack() 检测并调用。
     *
     * 流程：
     *   1. 检查 m_skill 是否存在
     *   2. 根据 m_skill->targetType() 解析目标列表
     *   3. 调用 m_skill->cast(this, resolvedTargets)
     *   4. 调用方（attack()）负责清空法力值
     *
     * 目标解析规则参见 Skill::targetType() 的文档。
     * 若 m_skill 为 nullptr（未绑定技能），此方法为空操作。
     *
     * @param target 当前攻击锁定目标（AOE/治疗技能可能不使用此参数）
     */
    void castSkill(Unit &target);

    // ========== 有限状态机 (FSM) ==========

    /**
     * @brief 切换战斗状态（有限状态机核心）
     *
     * 状态转换规则（完整的闭环 FSM）：
     *   Idle      → Attacking   攻击开始时
     *   Idle      → Moving      BattleSystem 分配寻路路径后
     *   Attacking → Casting     满蓝触发技能时
     *   Attacking → Idle        冷却结束 或 目标已死亡
     *   Casting   → Idle        技能执行完毕（attack() 内立即转换）
     *   Moving    → Idle        路径耗尽 或 进入攻击范围
     *   Any       → Dead        HP ≤ 0 时
     *   Dead      → (none)      终态，不可逆
     *
     * 闭环不变性：从任意非 Dead 状态出发，沿转换边可达 Idle，形成完整回路。
     *
     * 此方法包含：
     *   - 合法性校验（禁止 Dead → 其他）
     *   - 旧状态退出逻辑（on-exit）
     *   - 执行转换
     *   - 新状态进入逻辑（on-enter）
     *
     * @param newState 目标状态
     */
    void changeState(UnitState newState);

    /**
     * @brief 每帧按当前状态执行对应行为（自动攻击系统）
     *
     * 状态行为分发：
     *   - Idle:      等待 BattleSystem 分配目标
     *   - Attacking: 检测目标是否死亡 → 死亡则清空目标回到 Idle；
     *                检测冷却是否结束，结束则 → Idle（准备下一次攻击）
     *   - Casting:   技能已由 attack() 同步执行，回到 Idle（防御式回退）
     *   - Moving:    路径耗尽或进入攻击范围 → Idle
     *   - Dead:      不执行任何操作（由 update() 提前过滤）
     *
     * 由 update() 内部调用，通常不需要外部直接调用。
     */
    void updateState();

    // ========== 查询与工具 ==========

    /// 单位是否存活（HP > 0 且状态不是 Dead）
    bool isAlive() const;

    /// 单位是否可以发起攻击（状态为 Idle 且冷却完毕）
    bool canAttack() const;

    /// 更新棋盘坐标
    void setPosition(const QPoint &pos);

    /// 增加法力值（不超过最大法力值），同时检查是否可释放技能
    void addMana(int amount);

    /// 设置星阶（阶段三升星系统使用）
    void setStarLevel(int level);

    /// 添加羁绊标签
    void addTrait(Trait t);

    // ========== 技能系统 ==========

    /**
     * @brief 绑定技能对象
     *
     * Skill 对象的所有权不归 Unit 持有（通常为静态或栈分配），
     * 同一 Skill 实例可被多个 Unit 共享（只要技能是 stateless 的）。
     *
     * @param skill 技能对象指针；传入 nullptr 可解除绑定
     */
    void setSkill(Skill *skill) { m_skill = skill; }

    /// 返回绑定的技能对象（nullptr 表示无技能）
    Skill *skill() const { return m_skill; }

    // ========== 装备系统 ==========

    /**
     * @brief 装备一件物品
     *
     * Equipment 对象的所有权不归 Unit 持有。
     * 由 equipItem() 自由函数管理装备/卸下的 stat 变更。
     * 仅修改 Unit 的属性数值，不管理 Equipment 生命周期。
     *
     * @param eq 装备对象指针；传入 nullptr 可清空装备槽
     */
    void setEquipment(Equipment *eq) { m_equipment = eq; }

    /// 返回当前装备的物品（nullptr 表示无装备）
    Equipment *equipment() const { return m_equipment; }

    /**
     * @brief 设置战斗上下文（供技能目标解析使用）
     *
     * 由 BattleSystem 在每帧战斗更新前调用，确保技能的
     * targetType() 解析时使用敌方/友方存活单位的最新列表。
     *
     * @param allies  所有存活的己方单位
     * @param enemies 所有存活的敌方单位
     */
    void setCombatContext(const std::vector<Unit *> &allies,
                          const std::vector<Unit *> &enemies);

    // ========== 回血 ==========

    /**
     * @brief 恢复生命值（不超过最大生命值）
     *
     * 用于治疗技能（如 HealingLight）和未来的回血光环。
     * 已死亡的单位不可回血（不可起死回生）。
     *
     * @param amount 恢复量（非负；若为负值则钳制为 0）
     */
    void heal(int amount);

    /**
     * @brief 战后复活（绕过 FSM Dead 守卫）
     *
     * 仅在回合结束后调用，将阵亡单位恢复到 Idle 状态并设 HP=1，
     * 之后由 startNewRound 中的 heal() 回满血。
     */
    void reviveAfterBattle();

    // ========== 数值常量 ==========

    /// 每次普攻回复的法力值
    static constexpr int MANA_PER_ATTACK = 10;

protected:
    // ---- 基础属性 ----
    QString m_name;       ///< 单位名称
    int     m_hp;         ///< 当前生命值
    int     m_maxHp;      ///< 最大生命值
    int     m_atk;        ///< 攻击力
    int     m_range;      ///< 攻击距离（棋盘格数，1 = 近战）
    int     m_mana;       ///< 当前法力值
    int     m_maxMana;    ///< 最大法力值
    QPoint  m_position;   ///< 棋盘坐标（行/列，使用 QPoint 的 x=列, y=行）

    // ---- 归属与状态 ----
    Owner     m_owner;     ///< 控制归属（我方/敌方）
    UnitState m_state;     ///< 当前战斗状态

    // ---- 自动攻击系统 ----
    int   m_attackSpeed;       ///< 攻击间隔帧数（默认 60，越小越快）
    int   m_attackCooldown;    ///< 当前攻击冷却帧数（0 表示可以攻击）
    int   m_attackCooldownMax; ///< 攻击冷却上限帧数（初始 = m_attackSpeed）
    Unit *m_currentTarget = nullptr; ///< 当前锁定的攻击目标（不拥有所有权）
    bool m_justAttacked = false;      ///< 本帧是否刚执行了攻击（视觉动画用）
    bool m_justCasted   = false;      ///< 本帧是否刚释放了技能（技能特效用）

    // ---- 寻路移动 ----
    std::vector<QPoint> m_path;       ///< 当前移动路径（不含已走过的格子）
    int m_moveTimer = 0;              ///< 移动计时器（归零时移动至下一格，MOVE_SPEED 帧/格）

    // ---- 技能系统 ----
    Skill *m_skill = nullptr;                    ///< 绑定的技能对象（不拥有所有权）
    std::vector<Unit *> m_visibleEnemies;        ///< 可见敌方存活单位列表（战斗上下文）
    std::vector<Unit *> m_visibleAllies;         ///< 可见己方存活单位列表（战斗上下文）
    std::vector<Unit *> m_lastSkillTargets;      ///< 最近一次技能释放的目标（供特效生成）

    // ---- 装备系统 ----
    Equipment *m_equipment = nullptr;            ///< 当前装备的物品（不拥有所有权）

    // ---- 进阶属性（阶段三使用） ----
    int m_starLevel;                  ///< 星阶（1 或 2）
    QVector<Trait> m_traits;          ///< 羁绊标签集合
};

#endif // UNIT_H
