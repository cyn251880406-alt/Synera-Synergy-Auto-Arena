#ifndef ACTIONBUTTONPANEL_H
#define ACTIONBUTTONPANEL_H

#include <QWidget>
#include <QColor>

class Game;

/**
 * @brief ActionButtonPanel — game action buttons in glass-morphism style
 *
 * Vertical stack: 刷新商店 | 开始战斗 | 重新开始
 * All rendering via QPainter, matching the project's existing convention.
 */
class ActionButtonPanel : public QWidget {
    Q_OBJECT

public:
    explicit ActionButtonPanel(Game *game, QWidget *parent = nullptr);

signals:
    void refreshRequested();
    void startBattleRequested();
    void restartRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sizeHint() const override;

private:
    Game *m_game = nullptr;
    int m_hoveredBtn = -1;

    static constexpr int PANEL_WIDTH   = 130;
    static constexpr int BTN_HEIGHT    = 38;
    static constexpr int BTN_GAP       = 6;
    static constexpr int PADDING       = 10;
    static constexpr int CORNER_RADIUS = 8;

    enum { IDX_REFRESH, IDX_BATTLE, IDX_RESTART, BTN_COUNT };

    QRect buttonRect(int id) const;
    bool isButtonEnabled(int id) const;
    QString buttonText(int id) const;
};

#endif // ACTIONBUTTONPANEL_H
