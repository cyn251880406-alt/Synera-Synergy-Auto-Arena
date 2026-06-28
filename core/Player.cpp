#include "core/Player.h"

#include <algorithm> // std::clamp, std::min

// ============================================================================
// 构造
// ============================================================================

Player::Player()
{
    m_hp            = INITIAL_HP;
    m_maxHp         = INITIAL_MAX_HP;
    m_gold          = INITIAL_GOLD;
    m_level         = INITIAL_LEVEL;
    m_exp           = 0;
    m_populationCap = INITIAL_POP_CAP;
    m_winStreak     = 0;
    m_lossStreak    = 0;
}

// ============================================================================
// HP
// ============================================================================

void Player::takeDamage(int amount)
{
    if (amount <= 0)
        return;

    m_hp -= amount;
    if (m_hp < 0)
        m_hp = 0;
}

void Player::healHp(int amount)
{
    if (amount <= 0)
        return;

    m_hp += amount;
    if (m_hp > m_maxHp)
        m_hp = m_maxHp;
}

// ============================================================================
// 金币
// ============================================================================

bool Player::spendGold(int amount)
{
    if (amount <= 0)
        return true;        // 消费 0 金币始终成功

    if (m_gold < amount)
        return false;       // 余额不足

    m_gold -= amount;
    return true;
}

void Player::addGold(int amount)
{
    if (amount <= 0)
        return;

    m_gold += amount;
}

// ============================================================================
// 等级与经验
// ============================================================================

int Player::expToLevelUp() const
{
    if (m_level >= MAX_LEVEL)
        return 0;               // 满级不需要经验

    // 经验公式：每级需要 level * 4 点
    // 1→2: 4,  2→3: 8,  3→4: 12,  4→5: 16
    return m_level * 4;
}

int Player::addExp(int amount)
{
    if (amount <= 0)
        return 0;
    if (m_level >= MAX_LEVEL)
        return 0;               // 满级不再获得经验

    m_exp += amount;

    int levelUps = 0;
    while (m_level < MAX_LEVEL && m_exp >= expToLevelUp()) {
        m_exp -= expToLevelUp();    // 溢出经验保留到下一级
        levelUp();
        ++levelUps;
    }

    // 达到满级时清空多余经验
    if (m_level >= MAX_LEVEL)
        m_exp = 0;

    return levelUps;
}

// ============================================================================
// 人口
// ============================================================================

bool Player::canDeploy(int currentDeployed) const
{
    return currentDeployed < m_populationCap;
}

bool Player::upgradePopulation()
{
    // 已达最大人口上限
    if (m_populationCap >= MAX_POP_CAP)
        return false;

    // 余额不足
    if (!spendGold(POP_UPGRADE_COST))
        return false;

    ++m_populationCap;
    return true;
}

// ============================================================================
// 连胜/连败
// ============================================================================

void Player::recordWin()
{
    ++m_winStreak;
    m_lossStreak = 0;       // 胜利打断连败
}

void Player::recordLoss()
{
    ++m_lossStreak;
    m_winStreak = 0;        // 失败打断连胜
}

void Player::resetStreak()
{
    m_winStreak  = 0;
    m_lossStreak = 0;
}

// ============================================================================
// 回合结算
// ============================================================================

void Player::onRoundEnd(bool isWin)
{
    if (isWin)
        recordWin();
    else
        recordLoss();

    // 利息计算：每持有 10 金币获得 1 利息，上限 5 金币/回合
    int interest = std::min(m_gold / 10, 5);
    if (interest > 0)
        addGold(interest);

    // 每回合获得 EXP：胜 3 / 败 2（等级 1 需要 4 EXP，约 2 回合升一级）
    int expGain = isWin ? 3 : 2;
    addExp(expGain);
}

// ============================================================================
// 读档恢复
// ============================================================================

void Player::restoreState(int hp, int maxHp, int gold, int level, int exp,
                           int popCap, int winStreak, int lossStreak)
{
    m_hp            = (hp < 0) ? 0 : hp;
    m_maxHp         = (maxHp < 1) ? 1 : maxHp;
    m_gold          = (gold < 0) ? 0 : gold;
    m_level         = (level < 1) ? 1 : (level > MAX_LEVEL ? MAX_LEVEL : level);
    m_exp           = (exp < 0) ? 0 : exp;
    m_populationCap = (popCap < 1) ? 1 : (popCap > MAX_POP_CAP ? MAX_POP_CAP : popCap);
    m_winStreak     = (winStreak < 0) ? 0 : winStreak;
    m_lossStreak    = (lossStreak < 0) ? 0 : lossStreak;
}

// ============================================================================
// 内部辅助
// ============================================================================

int Player::popCapForLevel(int lvl)
{
    // 出战人数 = 等级 + 3，等级 5 时额外 +1 达到 8
    // 等级 1 → 4 人，2 → 5，3 → 6，4 → 7，5 → 8
    switch (lvl) {
        case 1: return 4;
        case 2: return 5;
        case 3: return 6;
        case 4: return 7;
        case 5: return 8;
        default: return (lvl < 1) ? 4 : MAX_POP_CAP;
    }
}

void Player::levelUp()
{
    ++m_level;

    // 升级自动同步人口上限
    int newCap = popCapForLevel(m_level);
    if (newCap > m_populationCap)
        m_populationCap = newCap;
}
