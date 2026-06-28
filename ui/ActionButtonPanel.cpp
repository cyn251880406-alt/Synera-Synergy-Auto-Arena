#include "ui/ActionButtonPanel.h"
#include "core/Game.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

// ============================================================================
// 构造
// ============================================================================

ActionButtonPanel::ActionButtonPanel(Game *game, QWidget *parent)
    : QWidget(parent), m_game(game)
{
    setMinimumWidth(110);
    setMaximumWidth(150);
    setMouseTracking(true);

    // 监听游戏状态变化以自动刷新按钮
    if (m_game) {
        connect(m_game, &Game::goldChanged,    this, [this](int) { update(); });
        connect(m_game, &Game::phaseChanged,   this, [this](Game::Phase) { update(); });
        connect(m_game, &Game::playerHpChanged,this, [this](int,int) { update(); });
    }
}

// ============================================================================
// 尺寸
// ============================================================================

QSize ActionButtonPanel::sizeHint() const
{
    int h = PADDING * 2 + BTN_COUNT * BTN_HEIGHT + (BTN_COUNT - 1) * BTN_GAP;
    return QSize(PANEL_WIDTH, h);
}

// ============================================================================
// 按钮信息
// ============================================================================

QRect ActionButtonPanel::buttonRect(int id) const
{
    int y = PADDING + id * (BTN_HEIGHT + BTN_GAP);
    return QRect(PADDING, y, PANEL_WIDTH - PADDING * 2, BTN_HEIGHT);
}

bool ActionButtonPanel::isButtonEnabled(int id) const
{
    if (!m_game) return false;

    switch (id) {
    case IDX_REFRESH:
        // 战斗阶段禁用刷新
        return m_game->currentPhase() == Game::Phase::Preparation
               && m_game->gold() >= 2;
    case IDX_BATTLE:
        // 仅准备阶段可用
        return m_game->currentPhase() == Game::Phase::Preparation;
    case IDX_RESTART:
        // 始终可用（准备阶段或游戏结束后）
        return m_game->player().isAlive()
               || m_game->currentPhase() == Game::Phase::Preparation;
    default: return false;
    }
}

QString ActionButtonPanel::buttonText(int id) const
{
    switch (id) {
    case IDX_REFRESH: return QStringLiteral("⟳ 刷新 2💰");
    case IDX_BATTLE:  return QStringLiteral("⚔ 开始战斗");
    case IDX_RESTART: return QStringLiteral("↺ 重新开始");
    default: return QString();
    }
}

// ============================================================================
// 绘制
// ============================================================================

void ActionButtonPanel::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();

    // ——— 背景（玻璃态） ———
    QLinearGradient bgGrad(0, 0, 0, h);
    bgGrad.setColorAt(0.0, QColor(38, 40, 48, 245));
    bgGrad.setColorAt(1.0, QColor(28, 30, 36, 248));
    p.setPen(Qt::NoPen);
    p.setBrush(bgGrad);
    p.drawRoundedRect(0, 0, w, h, 8, 8);

    // 装饰顶部亮线
    p.setPen(QPen(QColor(255, 255, 255, 25), 1));
    p.drawLine(8, 1, w - 8, 1);

    // 边框
    p.setPen(QPen(QColor(80, 82, 90), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(1, 1, w - 2, h - 2, 8, 8);

    // ——— 标题 ———
    p.setPen(QColor(180, 180, 190));
    p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
    p.drawText(0, 2, w, 20, Qt::AlignHCenter | Qt::AlignVCenter,
               QStringLiteral("操作"));

    // ——— 按钮 ———
    for (int i = 0; i < BTN_COUNT; ++i) {
        QRect r = buttonRect(i);
        bool enabled = isButtonEnabled(i);
        bool hovered = (m_hoveredBtn == i);

        if (enabled) {
            // 可用态按钮渐变
            QLinearGradient btnGrad(r.topLeft(), r.bottomLeft());

            if (i == IDX_BATTLE) {
                // 开始战斗 — 金色调
                if (hovered) {
                    btnGrad.setColorAt(0.0, QColor(220, 160, 40));
                    btnGrad.setColorAt(1.0, QColor(180, 120, 20));
                } else {
                    btnGrad.setColorAt(0.0, QColor(180, 130, 30));
                    btnGrad.setColorAt(1.0, QColor(140, 100, 20));
                }
            } else if (i == IDX_RESTART) {
                // 重新开始 — 红色调
                if (hovered) {
                    btnGrad.setColorAt(0.0, QColor(200, 80, 70));
                    btnGrad.setColorAt(1.0, QColor(160, 50, 40));
                } else {
                    btnGrad.setColorAt(0.0, QColor(170, 60, 50));
                    btnGrad.setColorAt(1.0, QColor(130, 40, 30));
                }
            } else {
                // 刷新 — 蓝灰色调
                if (hovered) {
                    btnGrad.setColorAt(0.0, QColor(100, 130, 170));
                    btnGrad.setColorAt(1.0, QColor(70, 100, 140));
                } else {
                    btnGrad.setColorAt(0.0, QColor(75, 90, 120));
                    btnGrad.setColorAt(1.0, QColor(55, 70, 95));
                }
            }
            p.setBrush(btnGrad);
            p.setPen(QPen(hovered ? QColor(255, 255, 255, 80) : QColor(255, 255, 255, 25), 1));
            p.drawRoundedRect(r, 6, 6);
            p.setPen(Qt::white);
        } else {
            // 禁用态
            p.setBrush(QColor(50, 50, 55));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(r, 6, 6);
            p.setPen(QColor(120, 120, 120));
        }

        p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        p.drawText(r, Qt::AlignCenter, buttonText(i));
    }
}

// ============================================================================
// 鼠标事件
// ============================================================================

void ActionButtonPanel::mouseMoveEvent(QMouseEvent *event)
{
    int oldHover = m_hoveredBtn;
    m_hoveredBtn = -1;

    for (int i = 0; i < BTN_COUNT; ++i) {
        if (buttonRect(i).contains(event->pos()) && isButtonEnabled(i)) {
            m_hoveredBtn = i;
            break;
        }
    }

    if (m_hoveredBtn != oldHover)
        update();
}

void ActionButtonPanel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;

    for (int i = 0; i < BTN_COUNT; ++i) {
        if (buttonRect(i).contains(event->pos()) && isButtonEnabled(i)) {
            switch (i) {
            case IDX_REFRESH: emit refreshRequested(); break;
            case IDX_BATTLE:  emit startBattleRequested(); break;
            case IDX_RESTART: emit restartRequested(); break;
            }
            update();
            return;
        }
    }
}

void ActionButtonPanel::leaveEvent(QEvent * /*event*/)
{
    m_hoveredBtn = -1;
    update();
}
