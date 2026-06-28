#include "battle/EnemySpawner.h"

#include <QtMath>        // qPow
#include <QRandomGenerator>
#include <algorithm>     // std::min

// ============================================================================
// 构造 — 初始化敌方模板池
// ============================================================================

EnemySpawner::EnemySpawner()
{
    // 模板池：名称、基础HP、基础ATK、范围、法力、最早出现轮次
    // 3 种敌方类型对应 3 职业：射手(远程高伤)/坦克(近战肉盾)/战士(近战均衡)
    m_templates = {
        // ===== 第 1 轮起可用 =====
        {"敌射手",   220,  28,  3,  60,  1},   // 远程高伤脆皮（对应射手）
        {"敌坦克",   430,  16,  1,  70,  1},   // 近战肉盾（对应坦克）
        {"敌战士",   300,  24,  1,  55,  1},   // 近战均衡（对应战士）

        // ===== 第 3 轮起可用（强化版） =====
        {"敌射手",   260,  38,  3,  55,  3},   // 远程精英射手
        {"敌坦克",   520,  22,  1,  75,  3},   // 重装坦克
        {"敌战士",   380,  36,  1,  50,  3},   // 精英战士
    };
}

// ============================================================================
// 公开接口
// ============================================================================

QVector<EnemySpawner::SpawnEntry> EnemySpawner::generateWave(int round) const
{
    QVector<SpawnEntry> wave;

    // ——— 1. 决定本轮敌人数量 ———
    int count = enemyCountForRound(round);

    // ——— 2. 筛选当前轮次可用的模板 ———
    QVector<const Template *> pool = availableTemplates(round);
    if (pool.isEmpty())
        return wave;            // 没有可用模板（不应发生）

    // ——— 3. 随机选取模板并应用缩放 ———
    QRandomGenerator *rng = QRandomGenerator::global();
    double scale = scalingFactor(round);

    for (int i = 0; i < count; ++i) {
        // 随机从可用池中选取一个模板
        int idx = rng->bounded(static_cast<int>(pool.size()));
        const Template &tmpl = *pool[idx];

        SpawnEntry entry;
        entry.name    = tmpl.name;
        entry.hp      = static_cast<int>(tmpl.baseHp  * scale);
        entry.atk     = static_cast<int>(tmpl.baseAtk * scale);
        entry.range   = tmpl.range;
        entry.maxMana = tmpl.maxMana;           // 法力值不随轮次缩放

        wave.append(entry);
    }

    return wave;
}

// ============================================================================
// 内部辅助
// ============================================================================

int EnemySpawner::enemyLevelForRound(int round)
{
    // 每 2 轮升 1 级，上限 5
    // 轮次 1-2 → 等级 1，3-4 → 2，5-6 → 3，7-8 → 4，9+ → 5
    int level = (round + 1) / 2;
    if (level < 1) level = 1;
    if (level > 5) level = 5;
    return level;
}

int EnemySpawner::enemyPopCapForLevel(int level)
{
    // 与 Player::popCapForLevel 一致
    switch (level) {
        case 1: return 3;
        case 2: return 4;
        case 3: return 5;
        case 4: return 6;
        case 5: return 8;
        default: return (level < 1) ? 3 : 8;
    }
}

int EnemySpawner::enemyPopCapForRound(int round)
{
    return enemyPopCapForLevel(enemyLevelForRound(round));
}

int EnemySpawner::enemyCountForRound(int round)
{
    // 敌方出战数 = 该轮次敌方等级对应的人口上限
    return enemyPopCapForRound(round);
}

double EnemySpawner::scalingFactor(int round)
{
    // 每 SCALING_INTERVAL 轮属性提升 SCALING_FACTOR 倍
    // 第 1~3 轮：scale = 1.0（基础值）
    // 第 4~6 轮：scale = 1.2
    // 第 7~9 轮：scale = 1.2^2 = 1.44
    // ...
    int tiers = (round - 1) / SCALING_INTERVAL;     // 第 1 轮 tier=0
    if (tiers <= 0)
        return 1.0;

    return qPow(SCALING_FACTOR, tiers);
}

QVector<const EnemySpawner::Template *> EnemySpawner::availableTemplates(int round) const
{
    QVector<const Template *> result;
    for (const Template &t : m_templates) {
        if (t.minRound <= round)
            result.append(&t);
    }
    return result;
}
