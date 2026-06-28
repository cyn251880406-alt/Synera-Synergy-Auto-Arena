#ifndef RESULTOVERLAY_H
#define RESULTOVERLAY_H

#include <QWidget>
#include <QPixmap>

// ============================================================================
// ResultOverlay — 胜利/失败结算画面叠加层
// ============================================================================

/**
 * @brief ResultOverlay — 在父窗口上显示全屏结算画面
 *
 * 显示 assets/ui/victory.png 或 defeat.png 作为结算画面。
 * 点击任意位置关闭：
 *   - 胜利画面：关闭后游戏自动进入下一轮
 *   - 失败画面：关闭后退出应用
 *
 * 用法示例：
 * @code
 *   ResultOverlay *overlay = new ResultOverlay(&mainWindow);
 *   overlay->showVictory();  // 或 showDefeat()
 *   // 用户点击后 overlay 自动 deleteLater
 * @endcode
 */
class ResultOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ResultOverlay(QWidget *parent = nullptr);

    /// 显示胜利结算画面
    void showVictory();

    /// 显示失败结算画面
    void showDefeat();

signals:
    /// 用户点击关闭结算画面
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QPixmap m_image;
    bool m_isDefeat = false;
};

#endif // RESULTOVERLAY_H
