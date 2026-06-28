#include "shop/ShopWidget.h"
#include "core/Game.h"
#include "shop/Shop.h"
#include "bench/Bench.h"
#include "ui/ResourceManager.h"

#include <QPainter>
#include <QMouseEvent>

// ============================================================================
// 静态颜色
// ============================================================================

const QColor ShopWidget::BG_COLOR         = QColor(30, 30, 35, 230);  // 半透明深色背景
const QColor ShopWidget::CARD_BG          = QColor(45, 45, 55);
const QColor ShopWidget::CARD_BORDER      = QColor(80, 80, 90);
const QColor ShopWidget::CARD_COST_BG     = QColor(35, 35, 42);
const QColor ShopWidget::BUY_BTN_COLOR    = QColor(60, 140, 60);     // 绿色购买按钮
const QColor ShopWidget::BUY_BTN_DISABLED = QColor(80, 80, 80);      // 灰化按钮
const QColor ShopWidget::BUY_BTN_HOVER    = QColor(80, 180, 80);     // 悬停亮绿
const QColor ShopWidget::REFRESH_BTN_COLOR= QColor(55, 80, 120);     // 蓝色刷新按钮
const QColor ShopWidget::TEXT_WHITE       = QColor(220, 220, 220);
const QColor ShopWidget::TEXT_GOLD        = QColor(255, 215, 0);
const QColor ShopWidget::TEXT_GRAY        = QColor(140, 140, 140);
const QColor ShopWidget::TEXT_STAR        = QColor(255, 215, 0);

// ============================================================================
// 构造
// ============================================================================

ShopWidget::ShopWidget(Game *game, QWidget *parent)
    : QWidget(parent)
    , m_game(game)
{
    setMinimumSize(PANEL_WIDTH, PANEL_HEIGHT);
    setWindowTitle(QStringLiteral("商店"));
    setMouseTracking(true);

    // 监听金币变化 → 重绘（更新按钮灰化状态）
    if (m_game) {
        connect(m_game, &Game::goldChanged,
                this,   [this](int) { update(); });
        connect(m_game, &Game::phaseChanged,
                this,   [this](Game::Phase) { update(); });
    }
}

// ============================================================================
// 尺寸
// ============================================================================

QSize ShopWidget::sizeHint() const
{
    return QSize(PANEL_WIDTH, PANEL_HEIGHT);
}

// ============================================================================
// 绘制
// ============================================================================

void ShopWidget::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // ——— 背景（深色玻璃态） ———
    p.setPen(Qt::NoPen);
    QLinearGradient bgGrad(0, 0, 0, h);
    bgGrad.setColorAt(0.0, QColor(38, 40, 48, 240));
    bgGrad.setColorAt(1.0, QColor(28, 30, 36, 245));
    p.setBrush(bgGrad);
    p.drawRoundedRect(0, 0, w, h, 8, 8);

    // 装饰顶部亮线
    p.setPen(QPen(QColor(255, 255, 255, 25), 1));
    p.drawLine(8, 1, w - 8, 1);

    if (!m_game)
        return;

    const Shop &shop    = m_game->shop();
    int playerGold      = m_game->player().gold();
    int roundNumber     = m_game->roundNumber();

    // ——— 标题 ———
    p.setPen(QColor(200, 200, 210));
    p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
    p.drawText(PADDING, 2, 160, 14,
               Qt::AlignLeft | Qt::AlignBottom,
               QStringLiteral("第 %1 轮  ·  商店").arg(roundNumber));
    // 金币提示
    p.setPen(QColor(255, 215, 0));
    p.drawText(w - PADDING - 60, 2, 60, 14,
               Qt::AlignRight | Qt::AlignBottom,
               QStringLiteral("💰 %1").arg(playerGold));

    // ——— 卡片（紧凑布局） ———
    static constexpr int IMG_SIZE  = 46;
    static constexpr int IMG_TOP   = 6;
    static constexpr int NAME_TOP  = 54;
    static constexpr int STAR_TOP  = 66;
    static constexpr int BTN_TOP   = 76;

    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        QRect cr = cardRect(i);
        const HeroTemplate *tmpl = shop.slot(i);

        if (!tmpl) {
            p.setBrush(QColor(40, 42, 48));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(cr, CORNER_RADIUS, CORNER_RADIUS);
            p.setPen(QColor(100, 100, 110));
            p.setFont(QFont("Microsoft YaHei", 9));
            p.drawText(cr, Qt::AlignCenter, QString::fromUtf8("已售罄"));
            continue;
        }

        bool canAfford = (playerGold >= tmpl->cost);
        bool benchFull  = m_game && m_game->bench() && !m_game->bench()->hasEmptySlot();
        bool canBuy = canAfford && !benchFull;
        QColor tierColor = costColor(tmpl->cost);

        // ------- 卡片背景 -------
        p.setBrush(QColor(44, 46, 54));
        p.setPen(QPen(canAfford ? tierColor.darker(120) : QColor(55, 55, 60), 1));
        p.drawRoundedRect(cr, CORNER_RADIUS, CORNER_RADIUS);

        // ------- 顶部色条 -------
        QRect tierBar(cr.left() + 3, cr.top() + 3, cr.width() - 6, 3);
        p.setBrush(tierColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(tierBar, 1.5, 1.5);

        // ------- 角色大图 -------
        ResourceManager &res = ResourceManager::instance();
        QPixmap icon = res.unitIconShop(tmpl->name);

        // 角色小图（居中）
        int imgX = cr.left() + (cr.width() - IMG_SIZE) / 2;
        QRect imgRect(imgX, cr.top() + IMG_TOP, IMG_SIZE, IMG_SIZE);

        if (!icon.isNull()) {
            p.setBrush(QColor(22, 24, 30));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(imgRect.adjusted(-1, -1, 1, 1), 3, 3);
            QPixmap scaled = icon.scaled(IMG_SIZE, IMG_SIZE,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            int ox = imgRect.left() + (IMG_SIZE - scaled.width()) / 2;
            int oy = imgRect.top()  + (IMG_SIZE - scaled.height()) / 2;
            p.drawPixmap(ox, oy, scaled);
        } else {
            p.setBrush(QColor(28, 30, 38));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(imgRect, 3, 3);
        }

        // ------- 英雄名称（图片下方居中） -------
        p.setPen(QColor(230, 230, 235));
        p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
        p.drawText(cr.left() + 4, cr.top() + NAME_TOP,
                   cr.width() - 8, 16,
                   Qt::AlignHCenter | Qt::AlignTop, tmpl->name);

        // ------- 星级（名称下方居中） -------
        p.setPen(QColor(255, 200, 60));
        p.setFont(QFont("Microsoft YaHei", 8));
        p.drawText(cr.left() + 4, cr.top() + STAR_TOP,
                   cr.width() - 8, 14,
                   Qt::AlignHCenter | Qt::AlignTop, costToStars(tmpl->cost));

        // ------- 费用标签（左上角） -------
        QRect costRect(cr.left() + 3, cr.top() + 2, 26, 12);
        p.setBrush(tierColor.darker(130));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(costRect, 2, 2);
        p.setPen(Qt::white);
        p.setFont(QFont("Microsoft YaHei", 7, QFont::Bold));
        p.drawText(costRect, Qt::AlignCenter,
                   QStringLiteral("%1💰").arg(tmpl->cost));

        // ------- 购买按钮（底部居中） -------
        QRect br(cr.left() + 10, cr.top() + BTN_TOP, cr.width() - 20, 24);
        if (canBuy) {
            QLinearGradient btnGrad(br.topLeft(), br.bottomLeft());
            btnGrad.setColorAt(0.0, QColor(70, 160, 70));
            btnGrad.setColorAt(1.0, QColor(50, 120, 50));
            p.setBrush(btnGrad);
            p.setPen(QPen(QColor(80, 180, 80), 0.5));
            p.drawRoundedRect(br, 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
            p.drawText(br, Qt::AlignCenter, QString::fromUtf8("购买"));
        } else if (benchFull) {
            p.setBrush(QColor(55, 50, 50));
            p.setPen(QPen(QColor(120, 70, 70), 0.5));
            p.drawRoundedRect(br, 3, 3);
            p.setPen(QColor(200, 120, 120));
            p.setFont(QFont("Microsoft YaHei", 7, QFont::Bold));
            p.drawText(br, Qt::AlignCenter, QString::fromUtf8("备战满"));
        } else {
            p.setBrush(QColor(65, 65, 70));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(br, 3, 3);
            p.setPen(QColor(140, 140, 140));
            p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
            p.drawText(br, Qt::AlignCenter, QString::fromUtf8("💰%1").arg(tmpl->cost));
        }
    }

    // ——— 刷新按钮 ———
    QRect rr = refreshButtonRect();
    bool canRefresh = (playerGold >= Shop::REFRESH_COST);
    {
        QLinearGradient refGrad(rr.topLeft(), rr.bottomLeft());
        if (canRefresh) {
            refGrad.setColorAt(0.0, QColor(65, 95, 145));
            refGrad.setColorAt(1.0, QColor(45, 70, 110));
        } else {
            refGrad.setColorAt(0.0, QColor(65, 65, 70));
            refGrad.setColorAt(1.0, QColor(45, 45, 50));
        }
        p.setBrush(refGrad);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rr, 4, 4);
    }
    p.setPen(canRefresh ? QColor(220, 220, 230) : QColor(140, 140, 140));
    p.setFont(QFont("Microsoft YaHei", 7, QFont::Bold));
    p.drawText(rr, Qt::AlignCenter, QStringLiteral("⟳  %1💰").arg(Shop::REFRESH_COST));
}

// ============================================================================
// 鼠标事件
// ============================================================================

void ShopWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_game || event->button() != Qt::LeftButton)
        return;

    QPoint pos = event->pos();

    // —— 检测刷新按钮 ——
    if (refreshButtonRect().contains(pos)) {
        m_game->refreshShop();
        update();
        return;
    }

    // —— 检测各槽位的购买按钮 ——
    const Shop &shop = m_game->shop();
    int playerGold   = m_game->player().gold();

    for (int i = 0; i < Shop::SLOT_COUNT; ++i) {
        if (!buyButtonRect(i).contains(pos))
            continue;

        const HeroTemplate *tmpl = shop.slot(i);
        if (!tmpl)
            continue;

        if (playerGold < tmpl->cost)
            continue;

        m_game->buyFromShop(i);
        update();
        return;
    }
}

// ============================================================================
// 布局辅助
// ============================================================================

QRect ShopWidget::cardRect(int slot) const
{
    int x = PADDING + slot * (CARD_WIDTH + CARD_GAP);
    int y = 16;
    return QRect(x, y, CARD_WIDTH, CARD_HEIGHT);
}

QRect ShopWidget::buyButtonRect(int slot) const
{
    QRect cr = cardRect(slot);
    return QRect(cr.left() + 6, cr.top() + 76, cr.width() - 12, 20);
}

QRect ShopWidget::refreshButtonRect() const
{
    int x = PADDING + 5 * CARD_WIDTH + 4 * CARD_GAP + CARD_GAP;
    int y = 16 + (CARD_HEIGHT - REFRESH_BTN_H) / 2;
    return QRect(x, y, REFRESH_BTN_W, REFRESH_BTN_H);
}

// ============================================================================
// 视觉辅助
// ============================================================================

QString ShopWidget::costToStars(int)
{
    return QStringLiteral("★");  // 全部 1★ 出现在商店
}

QColor ShopWidget::costColor(int cost)
{
    switch (cost) {
    case 1:  return QColor(140, 140, 150);  // 灰（普通）
    case 2:  return QColor(60, 120, 220);   // 蓝（稀有）
    default: return QColor(140, 140, 150);
    }
}
