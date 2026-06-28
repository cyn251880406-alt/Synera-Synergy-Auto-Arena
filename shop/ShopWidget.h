#ifndef SHOPWIDGET_H
#define SHOPWIDGET_H

#include <QWidget>
#include <QString>

class Game;

// ============================================================================
// ShopWidget — 商店招募面板（视图层）
// ============================================================================

/**
 * @brief ShopWidget — 商店招募面板（视图层）
 *
 * 展示 5 个可购买单位卡片 + 刷新按钮。点击购买调用 Game::buyFromShop()。
 */
class ShopWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造商店面板
     * @param game  游戏控制器（用于读取 Shop 状态与购买交互）
     * @param parent Qt 父控件
     */
    explicit ShopWidget(Game *game, QWidget *parent = nullptr);
    ~ShopWidget() override = default;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSize sizeHint() const override;

private:
    // ---- 引用 ----
    Game *m_game = nullptr;

    // ---- 布局常量 ----
    static constexpr int PADDING      = 8;    ///< 面板边距
    static constexpr int CARD_WIDTH   = 90;   ///< 每张卡片的宽度
    static constexpr int CARD_HEIGHT  = 100;  ///< 每张卡片的高度
    static constexpr int CARD_GAP     = 5;    ///< 卡片间距
    static constexpr int CORNER_RADIUS = 4;   ///< 卡片圆角半径

    static constexpr int PANEL_WIDTH =
        5 * CARD_WIDTH + 4 * CARD_GAP + 48 + CARD_GAP + 16;
    static constexpr int PANEL_HEIGHT = CARD_HEIGHT + 10;

    /// 刷新按钮尺寸
    static constexpr int REFRESH_BTN_W = 42;
    static constexpr int REFRESH_BTN_H = 28;

    /// 购买按钮尺寸（位于每张卡片底部）
    static constexpr int BUY_BTN_W = 80;
    static constexpr int BUY_BTN_H = 24;

    // ---- 静态颜色 ----
    static const QColor BG_COLOR;
    static const QColor CARD_BG;
    static const QColor CARD_BORDER;
    static const QColor CARD_COST_BG;
    static const QColor BUY_BTN_COLOR;
    static const QColor BUY_BTN_DISABLED;
    static const QColor BUY_BTN_HOVER;
    static const QColor REFRESH_BTN_COLOR;
    static const QColor TEXT_WHITE;
    static const QColor TEXT_GOLD;
    static const QColor TEXT_GRAY;
    static const QColor TEXT_STAR;

    // ---- 内部辅助 ----

    /// 获取指定槽位卡片的矩形区域（像素坐标）
    QRect cardRect(int slot) const;

    /// 获取卡片内购买按钮的矩形区域
    QRect buyButtonRect(int slot) const;

    /// 获取刷新按钮的矩形区域
    QRect refreshButtonRect() const;

    /// 将费用等级（1-4）映射为星级字符串
    static QString costToStars(int cost);

    /// 将费用等级映射为颜色
    static QColor costColor(int cost);
};

#endif // SHOPWIDGET_H
