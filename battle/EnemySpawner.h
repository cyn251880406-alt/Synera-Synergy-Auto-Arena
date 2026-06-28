#ifndef ENEMYSPAWNER_H
#define ENEMYSPAWNER_H

#include <QString>
#include <QVector>

// ============================================================================
// EnemySpawner — 敌方轮次生成器（逻辑层）
// ============================================================================

/**
 * @brief EnemySpawner — 按轮次配置自动生成敌方单位数据
 *
 * 维护敌方模板池，generateWave(r) 返回本轮敌方配置列表。
 * 属性每 3 轮 HP/ATK ×1.2 缩放。纯逻辑层，实际的 Unit 创建由 Game 负责。
 */
class EnemySpawner {
public:
    // ========== 数据结构 ==========

    /**
     * @brief SpawnEntry — 一个待生成的敌方单位配置
     *
     * 包含创建 Unit 所需的全部属性值（已应用轮次缩放）。
     * Game 层拿到 SpawnEntry 后创建 Unit 并放置到棋盘。
     */
    struct SpawnEntry {
        QString name;       ///< 单位名称（如"敌步兵"、"敌法师"）
        int     hp;         ///< 生命值（已缩放）
        int     atk;        ///< 攻击力（已缩放）
        int     range;      ///< 攻击距离（1=近战, 3=远程）
        int     maxMana;    ///< 最大法力值
    };

    // ========== 构造 ==========

    EnemySpawner();
    ~EnemySpawner() = default;

    // ========== 生成 ==========

    /**
     * @brief 为指定轮次生成敌方单位配置列表
     *
     * 内部流程：
     *   1. 由轮次决定本轮的敌人数量
     *   2. 从模板池中选取符合出现条件的模板
     *   3. 对选取模板的属性应用轮次缩放
     *
     * @param round 当前轮次（从 1 开始）
     * @return 本轮应生成的敌方单位配置列表
     */
    QVector<SpawnEntry> generateWave(int round) const;

    // ========== 常量 ==========

    /// 每多少轮属性提升一次
    static constexpr int SCALING_INTERVAL = 3;
    /// 每次属性提升的倍率（HP 和 ATK 乘以 1.2）
    static constexpr double SCALING_FACTOR = 1.2;
    /// 单轮最大敌人数
    static constexpr int MAX_ENEMIES = 8;

    // ========== 敌方等级与人口 ==========

    /// 根据轮次计算敌方等级（与 Player 等级对标，每 2 轮升 1 级，上限 5）
    static int enemyLevelForRound(int round);

    /// 敌方等级对应的人口上限（与 Player::popCapForLevel 一致）
    static int enemyPopCapForLevel(int level);

    /// 便捷方法：根据轮次直接获取敌方人口上限 = enemyPopCapForLevel(enemyLevelForRound(round))
    static int enemyPopCapForRound(int round);

private:
    // ========== 内部结构 ==========

    /**
     * @brief Template — 敌方单位模板
     *
     * 存储一种敌方单位的基础属性与最早出现轮次。
     * minRound 控制模板何时开始出现在随机池中。
     */
    struct Template {
        QString name;
        int     baseHp;
        int     baseAtk;
        int     range;
        int     maxMana;
        int     minRound;   ///< 最早出现在第几轮（如精英从第 3 轮开始）
    };

    // ========== 模板池 ==========

    /// 所有敌方单位模板（在构造函数中初始化）
    QVector<Template> m_templates;

    // ========== 内部辅助 ==========

    /// 根据轮次计算应生成的敌人数量
    static int enemyCountForRound(int round);

    /// 计算指定轮次的属性缩放因子
    /// 每 SCALING_INTERVAL 轮乘以 SCALING_FACTOR 一次
    static double scalingFactor(int round);

    /// 选取指定轮次可用的模板列表
    QVector<const Template *> availableTemplates(int round) const;
};

#endif // ENEMYSPAWNER_H
