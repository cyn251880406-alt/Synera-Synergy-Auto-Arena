#include "ui/SellZone.h"

#include <QPainter>
#include <QMouseEvent>

SellZone::SellZone(QWidget *parent)
    : QWidget(parent)
{
    setToolTip(QStringLiteral("拖拽单位到这里出售（原价退款）"));
}

QSize SellZone::sizeHint() const
{
    return QSize(WIDTH, HEIGHT);
}

bool SellZone::isPointOverWidget(const QPointF &screenPt) const
{
    QRect r(mapToGlobal(QPoint(0, 0)), size());
    return r.contains(screenPt.toPoint());
}

void SellZone::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // 暗红色背景
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(80, 30, 30, 220));
    p.drawRoundedRect(0, 0, w, h, 6, 6);

    // 红色边框
    p.setPen(QPen(QColor(200, 60, 60), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(1, 1, w - 2, h - 2, 5, 5);

    // 文字
    p.setPen(QColor(255, 180, 180));
    p.setFont(QFont("Microsoft YaHei", 9, QFont::Bold));
    p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, QString::fromUtf8("💰 出售"));
}
