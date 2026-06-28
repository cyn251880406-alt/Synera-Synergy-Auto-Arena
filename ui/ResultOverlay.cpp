#include "ui/ResultOverlay.h"
#include "ui/ResourceManager.h"

#include <QPainter>
#include <QMouseEvent>

ResultOverlay::ResultOverlay(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setCursor(Qt::ArrowCursor);
    hide();
}

void ResultOverlay::showVictory()
{
    m_image = ResourceManager::instance().victoryImage();
    m_isDefeat = false;

    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
        raise();
    }
    show();
}

void ResultOverlay::showDefeat()
{
    m_image = ResourceManager::instance().defeatImage();
    m_isDefeat = true;

    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
        raise();
    }
    show();
}

void ResultOverlay::paintEvent(QPaintEvent * /*event*/)
{
    // 同步到父窗口大小（处理窗口缩放）
    if (parentWidget() && size() != parentWidget()->size())
        setGeometry(parentWidget()->rect());

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // 半透明暗色遮罩
    p.fillRect(rect(), QColor(0, 0, 0, 180));

    if (m_image.isNull()) {
        // 回退：纯文字
        p.setPen(Qt::white);
        p.setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter,
                   m_isDefeat ? QString::fromUtf8("游戏结束")
                              : QString::fromUtf8("胜利！"));
        p.setFont(QFont("Microsoft YaHei", 14));
        p.drawText(rect().adjusted(0, 50, 0, 0), Qt::AlignHCenter | Qt::AlignBottom,
                   QString::fromUtf8("点击任意位置继续"));
        return;
    }

    // 图片居中缩放（保持宽高比，不超过 80% 窗口）
    QSize maxSize = size() * 0.8;
    QPixmap scaled = m_image.scaled(maxSize, Qt::KeepAspectRatio,
                                     Qt::SmoothTransformation);
    QPoint topLeft((width() - scaled.width()) / 2,
                   (height() - scaled.height()) / 2);
    p.drawPixmap(topLeft, scaled);

    // 提示文字
    p.setPen(QColor(255, 255, 255, 180));
    p.setFont(QFont("Microsoft YaHei", 13));
    int tipY = topLeft.y() + scaled.height() + 20;
    p.drawText(0, tipY, width(), 30, Qt::AlignHCenter | Qt::AlignTop,
               QString::fromUtf8("点击任意位置继续"));
}

void ResultOverlay::mousePressEvent(QMouseEvent * /*event*/)
{
    hide();
    emit dismissed();
}
