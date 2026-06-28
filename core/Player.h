#ifndef PLAYER_H
#define PLAYER_H

// ============================================================================
// Player — 玩家实体（逻辑层）
// ============================================================================

/**
 * @brief Player — 管理玩家所有属性与资源
 *
 * 纯逻辑层：血量/金币/等级/经验/人口上限/连胜连败。
 * 不依赖 Qt，所有方法返回明确状态供 Game 层使用。
 */
class Player {
public:
    // ========== 常量 ==========

    /// 初始血量 / 最大血量
    static constexpr int INITIAL_HP     = 100;
    static constexpr int INITIAL_MAX_HP = 100;

    /// 初始金币
    static constexpr int INITIAL_GOLD   = 10;

    /// 初始等级
    static constexpr int INITIAL_LEVEL  = 1;
    /// 最大等级
    static constexpr int MAX_LEVEL      = 5;

    /// 初始人口上限（等级 1 时）
    static constexpr int INITIAL_POP_CAP = 4;
    /// 最大人口上限
    static constexpr int MAX_POP_CAP    = 8;

    /// 升级人口所需金币（每次固定）
    static constexpr int POP_UPGRADE_COST = 4;

    // ========== 构造 ==========

    Player();
    ~Player() = default;

    // ========== HP（血量） ==========

    /// 当前血量
    int hp() const { return m_hp; }
    /// 最大血量
    int maxHp() const { return m_maxHp; }

    /// 是否存活（HP > 0）
    bool isAlive() const { return m_hp > 0; }

    /**
     * @brief 承受伤害
     *
     * 扣减当前血量，钳制到 [0, maxHp]。
     * 通常在敌方单位突破防线时调用。
     *
     * @param amount 伤害值（必须 ≥ 0）
     */
    void takeDamage(int amount);

    /**
     * @brief 恢复血量
     *
     * 增加当前血量，不超过 maxHp。
     * 预留：某些装备/羁绊可能提供回血效果。
     */
    void healHp(int amount);

    // ========== Gold（金币） ==========

    /// 当前金币数
    int gold() const { return m_gold; }

    /**
     * @brief 消费金币
     *
     * 用于商店购买单位、升级人口等。
     * 余额不足时操作失败。
     *
     * @param amount 消费金额（必须 ≥ 0）
     * @return true 消费成功；false 余额不足
     */
    bool spendGold(int amount);

    /**
     * @brief 获得金币
     *
     * 用于回合结算发放金币、出售单位等。
     * 结算金额由 Game 层计算后传入。
     */
    void addGold(int amount);

    // ========== Level & EXP（等级与经验） ==========

    /// 当前等级 [1, MAX_LEVEL]
    int level() const { return m_level; }
    /// 当前经验值
    int exp() const { return m_exp; }

    /**
     * @brief 升至下一级所需的经验值
     *
     * 经验公式：每级需要 level * 4 点经验
     *   1→2: 4,  2→3: 8,  3→4: 12, ...
     */
    int expToLevelUp() const;

    /**
     * @brief 获得经验
     *
     * 经验达标时自动升级（增加人口上限 + 重置经验）。
     * 满级后不再获得经验。
     *
     * @param amount 获得的经验值
     * @return 本次升级的次数（0 = 未升级）
     */
    int addExp(int amount);

    // ========== Population（人口上限） ==========

    /// 当前人口上限（可部署的最大单位数）
    int populationCap() const { return m_populationCap; }

    /**
     * @brief 检查是否可以部署更多单位
     *
     * @param currentDeployed 当前已部署的己方单位数
     * @return true 还有空位可部署
     */
    bool canDeploy(int currentDeployed) const;

    /**
     * @brief 花费金币提升人口上限
     *
     * 每次提升人口上限 +1，消耗 POP_UPGRADE_COST 金币。
     * 达到 MAX_POP_CAP 或余额不足时操作失败。
     *
     * @return true 升级成功；false 已达上限或余额不足
     */
    bool upgradePopulation();

    // ========== Streak（连胜/连败追踪） ==========

    /// 当前连胜场次
    int winStreak()  const { return m_winStreak; }
    /// 当前连败场次
    int lossStreak() const { return m_lossStreak; }

    /// 记录一场胜利（连胜+1，连败清零）
    void recordWin();
    /// 记录一场失败（连败+1，连胜清零）
    void recordLoss();
    /// 重置连胜/连败（用于调试/特殊情况）
    void resetStreak();

    // ========== 回合结算 ==========

    /**
     * @brief 回合结束结算
     *
     * 更新连胜/连败状态。
     * 阶段三将在此添加利息计算（每 10 金 +1 利息，上限 5）。
     *
     * @param isWin 本回合是否胜利
     */
    void onRoundEnd(bool isWin);

    /// 从存档数据恢复全部状态（读档用）
    void restoreState(int hp, int maxHp, int gold, int level, int exp,
                      int popCap, int winStreak, int lossStreak);

private:
    // ---- 属性 ----
    int m_hp             = INITIAL_HP;
    int m_maxHp          = INITIAL_MAX_HP;
    int m_gold           = INITIAL_GOLD;
    int m_level          = INITIAL_LEVEL;
    int m_exp            = 0;
    int m_populationCap  = INITIAL_POP_CAP;

    // ---- 连胜/连败 ----
    int m_winStreak  = 0;
    int m_lossStreak = 0;

    // ---- 内部辅助 ----

    /// 等级对应的人口上限：Lv1→3, Lv2→4, Lv3→5, Lv4→6, Lv5→8
    static int popCapForLevel(int lvl);

    /// 处理升级（增加人口上限、重置经验）
    void levelUp();
};

#endif // PLAYER_H
