#include "ui/TraitPanel.h"
#include "core/Game.h"
#include "core/Player.h"
#include "synergy/SynergySystem.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

TraitPanel::TraitPanel(Game *game, QWidget *parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
    , m_game(game)
{
    setWindowTitle(QStringLiteral("羁绊"));
    setFixedSize(200, 160);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, [this]() { update(); });
    m_refreshTimer->start(500);  // 每 0.5s 刷新
}

void TraitPanel::refresh()
{
    update();
}

void TraitPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width(), pad = 12, barW = w - pad * 2;

    // 背景
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(32, 34, 40));
    p.drawRoundedRect(0, 0, w, height(), 8, 8);

    if (!m_game) return;

    // 读取羁绊数据
    auto deployed = m_game->deployedPlayerUnits();
    std::vector<Unit *> units;
    units.reserve(deployed.size());
    for (Unit *u : deployed) units.push_back(u);

    auto counts = m_game->synergySystem().calculateTraitCounts(units);

    struct Row { QString name; int count; int thresh; bool active; int atk; int hp; QColor color; };
    Row rows[] = {
        {QStringLiteral("射手"), 0, 2, false, 0, 0, QColor(80, 220, 100)},
        {QStringLiteral("坦克"), 0, 2, false, 0, 0, QColor(100, 160, 240)},
        {QStringLiteral("战士"), 0, 2, false, 0, 0, QColor(255, 160, 50)},
    };

    // 填充 count
    Trait tagMap[] = {Trait::Archer, Trait::Tank, Trait::Warrior};
    for (int i = 0; i < 3; ++i) {
        auto it = counts.find(tagMap[i]);
        rows[i].count = (it != counts.end()) ? it->second : 0;
        auto *tier = SynergySystem::activeTier(tagMap[i], rows[i].count);
        if (tier) {
            rows[i].active = true;
            rows[i].thresh = tier->threshold;
            rows[i].atk    = tier->atkPercent;
            rows[i].hp     = tier->maxHpPercent;
        }
    }

    int y = pad + 4;

    // 标题
    p.setPen(QColor(200, 200, 210));
    p.setFont(QFont("Microsoft YaHei", 11, QFont::Bold));
    p.drawText(pad, y, barW, 20, Qt::AlignCenter, QStringLiteral("⚡ 羁绊"));
    y += 22;

    for (auto &r : rows) {
        // 背景行
        QRect rowR(pad, y, barW, 30);
        p.setBrush(r.active ? QColor(45, 55, 50) : QColor(38, 38, 44));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rowR, 4, 4);

        // 左侧色块
        p.setBrush(r.color);
        p.drawRoundedRect(rowR.left() + 4, rowR.top() + 6, 4, rowR.height() - 12, 2, 2);

        // 名称
        p.setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        p.setPen(r.active ? QColor(240, 240, 240) : QColor(120, 120, 130));
        p.drawText(rowR.left() + 16, rowR.top(), 50, rowR.height(),
                   Qt::AlignLeft | Qt::AlignVCenter, r.name);

        // 计数
        p.setFont(QFont("Microsoft YaHei", 9));
        p.drawText(rowR.left() + 68, rowR.top(), 40, rowR.height(),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("%1/%2").arg(r.count).arg(r.thresh));

        // 加成
        if (r.active) {
            QString bonus;
            if (r.atk > 0) bonus = QStringLiteral("ATK+%1%").arg(r.atk);
            if (r.hp  > 0) bonus = QStringLiteral("HP+%1%").arg(r.hp);
            p.setPen(QColor(255, 215, 0));
            p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
            p.drawText(rowR.right() - 80, rowR.top(), 76, rowR.height(),
                       Qt::AlignRight | Qt::AlignVCenter, bonus);
        } else if (r.count > 0) {
            p.setPen(QColor(160, 160, 140));
            p.setFont(QFont("Microsoft YaHei", 7));
            p.drawText(rowR.right() - 70, rowR.top(), 66, rowR.height(),
                       Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("未激活"));
        }

        y += 34;
    }
}
