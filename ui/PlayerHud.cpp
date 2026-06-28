#include "ui/PlayerHud.h"
#include "ui/EquipWidget.h"
#include "core/Game.h"
#include "core/Player.h"

#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

// ============================================================================

PlayerHud::PlayerHud(Game *game, QWidget *parent)
    : QWidget(parent), m_game(game)
{
    setFixedHeight(BAR_HEIGHT);
    setMinimumWidth(600);

    // 装备子控件（嵌入右侧）
    m_equipWidget = new EquipWidget(game, this);

    if (m_game) {
        connect(m_game, &Game::playerHpChanged,
                this, [this](int hp, int maxHp) { onHpChanged(hp, maxHp); });
        connect(m_game, &Game::goldChanged,
                this, [this](int gold) { onGoldChanged(gold); });
        connect(m_game, &Game::phaseChanged,
                this, [this](Game::Phase) { update(); });
    }

    refreshTargets();
    m_animHp = static_cast<double>(m_targetHp);

    m_animTimer = new QTimer(this);
    m_animTimer->setTimerType(Qt::PreciseTimer);
    connect(m_animTimer, &QTimer::timeout, this, &PlayerHud::onAnimationTick);
    m_animTimer->start(16);
}

QSize PlayerHud::sizeHint() const
{
    return QSize(900, BAR_HEIGHT);
}

// ============================================================================

void PlayerHud::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = BAR_HEIGHT;

    // 半透明深色背景
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(22, 24, 30, 230));
    p.drawRect(0, 0, w, h);

    // 底边亮线
    p.setPen(QPen(QColor(255, 255, 255, 20), 1));
    p.drawLine(0, h - 1, w, h - 1);

    if (!m_game) return;

    const int phase = static_cast<int>(m_game->currentPhase());
    const int round = m_game->roundNumber();

    // 装备区域占右侧 180px
    const int eqAreaW = 180;
    const int infoW = w - eqAreaW - 16;
    int x = 12;

    // ---- Round ----
    p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    p.setPen(QColor(200, 200, 215));
    p.drawText(x, 0, 48, h, Qt::AlignLeft | Qt::AlignVCenter,
               QStringLiteral("Round%1").arg(round));
    x += 48;

    // 分隔线
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    p.drawLine(x + 4, 10, x + 4, h - 10);
    x += 12;

    // ---- Phase ----
    QString phaseStr = (phase == 1) ? QStringLiteral("⚔ 战斗") :
                       (phase == 2) ? QStringLiteral("✅ 结算") :
                       QStringLiteral("▶ 准备");
    QColor phaseCol = (phase == 1) ? QColor(255, 140, 60) :
                      (phase == 2) ? QColor(100, 200, 100) :
                      QColor(130, 180, 240);
    p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    p.setPen(phaseCol);
    p.drawText(x, 0, 52, h, Qt::AlignLeft | Qt::AlignVCenter, phaseStr);
    x += 52;

    // ---- HP ----
    {
        bool hpFlash = (m_hpFlashCounter > 0);
        int hpVal = static_cast<int>(m_animHp);
        double hpRatio = (m_targetMaxHp > 0) ? m_animHp / m_targetMaxHp : 0.0;

        p.setFont(QFont("Microsoft YaHei", 8));
        p.setPen(QColor(180, 180, 190));
        p.drawText(x, 0, 16, h, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("❤"));
        x += 16;

        // HP 条 (40px wide, 10px tall)
        int barY = (h - 10) / 2, barW = 46;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(50, 18, 18));
        p.drawRoundedRect(x, barY, barW, 10, 3, 3);
        int fillW = static_cast<int>(barW * hpRatio);
        if (fillW > 0) {
            p.setBrush(hpFlash ? QColor(255, 130, 130) : QColor(220, 60, 60));
            p.drawRoundedRect(x, barY, fillW, 10, 3, 3);
        }
        x += barW + 4;

        p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
        p.setPen(hpFlash ? QColor(255, 200, 200) : QColor(240, 240, 240));
        p.drawText(x, 0, 32, h, Qt::AlignLeft | Qt::AlignVCenter, QString::number(hpVal));
        x += 36;
    }

    // ---- Gold ----
    {
        bool goldFlash = (m_goldFlashCounter > 0);
        p.setFont(QFont("Microsoft YaHei", 8));
        p.setPen(QColor(255, 200, 80));
        p.drawText(x, 0, 16, h, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("💰"));
        x += 16;
        p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        p.setPen(goldFlash ? QColor(255, 255, 180) : QColor(255, 215, 0));
        p.drawText(x, 0, 30, h, Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(m_targetGold));
        x += 34;
    }

    // ---- Level ----
    {
        bool lvlFlash = (m_levelFlashCounter > 0);
        p.setFont(QFont("Microsoft YaHei", 8));
        p.setPen(QColor(160, 190, 220));
        p.drawText(x, 0, 12, h, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Lv"));
        x += 14;
        p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        p.setPen(lvlFlash ? QColor(200, 230, 255) : QColor(130, 190, 240));
        p.drawText(x, 0, 18, h, Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(m_targetLevel));
        x += 24;
    }

    // ---- Pop ----
    {
        bool popFull = (m_targetDeployed >= m_targetPopCap);
        p.setFont(QFont("Microsoft YaHei", 8));
        p.setPen(QColor(180, 190, 200));
        p.drawText(x, 0, 16, h, Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("👥"));
        x += 16;
        p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        p.setPen(popFull ? QColor(255, 120, 120) : QColor(220, 220, 210));
        p.drawText(x, 0, 40, h, Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("%1/%2").arg(m_targetDeployed).arg(m_targetPopCap));
        x += 42;
    }

    // 右侧装备区域：由 EquipWidget 子控件占据
    m_equipWidget->setGeometry(w - eqAreaW - 40, 4, eqAreaW, h - 8);

    // [羁绊] 小按钮（装备区右侧）
    QRect traitBtn(w - 36, 6, 32, h - 12);
    p.setBrush(QColor(60, 60, 72));
    p.setPen(QPen(QColor(100, 100, 120), 1));
    p.drawRoundedRect(traitBtn, 4, 4);
    p.setPen(QColor(200, 200, 210));
    p.setFont(QFont("Microsoft YaHei", 7, QFont::Bold));
    p.drawText(traitBtn, Qt::AlignCenter, QStringLiteral("羁绊"));
}

// ============================================================================

void PlayerHud::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    // 检测 [羁绊] 按钮点击
    int w = width(), h = BAR_HEIGHT;
    QRect traitBtn(w - 36, 6, 32, h - 12);
    if (traitBtn.contains(event->pos())) {
        emit traitButtonClicked();
        return;
    }
}

// ============================================================================

void PlayerHud::onAnimationTick()
{
    if (!m_game) return;

    refreshTargets();

    double hpDiff = m_targetHp - m_animHp;
    if (qAbs(hpDiff) > 0.5) m_animHp += hpDiff * 0.2;
    else m_animHp = static_cast<double>(m_targetHp);

    double expDiff = m_targetExp - m_animExp;
    if (qAbs(expDiff) > 0.5) m_animExp += expDiff * 0.2;
    else m_animExp = static_cast<double>(m_targetExp);

    if (m_hpFlashCounter > 0)    --m_hpFlashCounter;
    if (m_goldFlashCounter > 0)  --m_goldFlashCounter;
    if (m_levelFlashCounter > 0) --m_levelFlashCounter;

    update();
}

void PlayerHud::onHpChanged(int hp, int)
{
    m_targetHp = hp;
    m_hpFlashCounter = FLASH_DURATION;
}

void PlayerHud::onGoldChanged(int gold)
{
    m_targetGold = gold;
    m_goldFlashCounter = FLASH_DURATION;
}

void PlayerHud::refreshTargets()
{
    if (!m_game) return;

    const Player &pl = m_game->player();
    m_targetMaxHp    = pl.maxHp();
    m_targetGold     = pl.gold();
    m_targetLevel    = pl.level();
    m_targetExp      = pl.exp();
    m_targetExpToLevel = pl.expToLevelUp();
    m_targetPopCap   = pl.populationCap();
    m_winStreak      = pl.winStreak();
    m_lossStreak     = pl.lossStreak();
    m_targetDeployed = m_game->deployedPlayerUnits().size();

    if (m_targetLevel != m_prevLevel) {
        m_levelFlashCounter = FLASH_DURATION;
        m_prevLevel = m_targetLevel;
    }
}
