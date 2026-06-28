#ifndef SELLZONE_H
#define SELLZONE_H

#include <QWidget>
#include <QPointF>

// ============================================================================
// SellZone — 出售区域控件
// ============================================================================

/**
 * @brief SellZone — 拖拽单位到此区域以原价出售
 *
 * 与 BoardWidget/BenchWidget 使用相同的屏幕坐标碰撞检测方式。
 * UnitItem 在 mouseReleaseEvent 中通过 isPointOverWidget() 检测落点。
 */
class SellZone : public QWidget {
    Q_OBJECT

public:
    explicit SellZone(QWidget *parent = nullptr);
    ~SellZone() override = default;

    QSize sizeHint() const override;

    /// 判断全局屏幕坐标是否落在本控件范围内
    bool isPointOverWidget(const QPointF &screenPt) const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int WIDTH  = 70;
    static constexpr int HEIGHT = 44;
};

#endif // SELLZONE_H
