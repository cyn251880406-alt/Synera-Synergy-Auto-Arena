#include "ui/EquipWidget.h"
#include "core/Game.h"
#include "equipment/Equipment.h"

#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>

EquipWidget::EquipWidget(Game *game, QWidget *parent)
    : QWidget(parent), m_game(game)
{
    setMouseTracking(true);
    setAcceptDrops(true);
    setToolTip(QStringLiteral("点击选中 → 点击单位穿戴 | 右键单位卸下"));

    if (m_game) {
        connect(m_game, &Game::equipmentDropped,
                this, [this](Equipment *) { update(); });
    }
}

QSize EquipWidget::sizeHint() const
{
    return QSize(180, CARD_SIZE);
}

QRect EquipWidget::cardRect(int index, int count) const
{
    Q_UNUSED(count);
    int x = index * (CARD_SIZE + CARD_GAP);
    return QRect(x, 0, CARD_SIZE, CARD_SIZE);
}

void EquipWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (!m_game) return;

    const auto &pool = m_game->equipmentPool();
    int count = pool.size();

    if (m_selectedIdx >= count) {
        m_selectedIdx = -1;
        m_game->setSelectedEquipment(nullptr);
    }

    if (count == 0) {
        p.setPen(QColor(100, 100, 110));
        p.setFont(QFont("Microsoft YaHei", 8));
        p.drawText(rect(), Qt::AlignCenter, QStringLiteral("装备池空"));
        return;
    }

    for (int i = 0; i < count; ++i) {
        QRect cr = cardRect(i, count);
        bool selected = (i == m_selectedIdx);
        const Equipment *eq = pool[i];

        // 卡片背景
        p.setBrush(QColor(42, 42, 50));
        p.setPen(QPen(selected ? QColor(100, 220, 255) : QColor(70, 70, 80),
                      selected ? 2.0 : 1.0));
        p.drawRoundedRect(cr, 3, 3);

        // 装备名前 2 字 + 颜色标记
        QColor eqColor;
        QString tn = eq->typeName();
        if (tn == QStringLiteral("Sword"))   eqColor = QColor(180, 130, 60);
        else if (tn == QStringLiteral("Armor"))   eqColor = QColor(130, 130, 160);
        else if (tn == QStringLiteral("Glove"))   eqColor = QColor(180, 150, 60);
        else if (tn == QStringLiteral("Crystal")) eqColor = QColor(80, 160, 220);
        else eqColor = QColor(140, 140, 140);

        // 小色块标记
        p.setBrush(eqColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(cr.left() + 3, cr.top() + 3, cr.width() - 6, 5, 2, 2);

        // 装备名缩写
        p.setPen(QColor(210, 210, 220));
        p.setFont(QFont("Microsoft YaHei", 8, QFont::Bold));
        p.drawText(cr.adjusted(2, 10, -2, -2), Qt::AlignCenter, eq->name().left(2));

        if (selected) {
            p.setBrush(QColor(100, 220, 255, 40));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(cr, 3, 3);
        }
    }
}

void EquipWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_game || event->button() != Qt::LeftButton) return;

    const auto &pool = m_game->equipmentPool();
    int count = pool.size();

    m_pressIndex = -1;
    for (int i = 0; i < count; ++i) {
        if (cardRect(i, count).contains(event->pos())) {
            m_pressIndex = i;
            m_pressPos = event->pos();
            return;
        }
    }
    m_selectedIdx = -1;
    m_game->setSelectedEquipment(nullptr);
    update();
}

void EquipWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressIndex < 0) return;
    if (!(event->buttons() & Qt::LeftButton)) { m_pressIndex = -1; return; }
    if ((event->pos() - m_pressPos).manhattanLength() < DRAG_THRESHOLD) return;

    const auto &pool = m_game->equipmentPool();
    if (m_pressIndex >= pool.size()) { m_pressIndex = -1; return; }

    Equipment *eq = pool[m_pressIndex];
    QMimeData *mime = new QMimeData;
    mime->setData("application/x-synera-equipment", eq->typeName().toUtf8());

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mime);

    QPixmap pix(CARD_SIZE, CARD_SIZE);
    pix.fill(Qt::transparent);
    {
        QPainter pp(&pix);
        pp.setRenderHint(QPainter::Antialiasing);
        pp.setBrush(QColor(42, 42, 50));
        pp.setPen(QPen(QColor(80, 80, 90), 1));
        pp.drawRoundedRect(1, 1, CARD_SIZE - 2, CARD_SIZE - 2, 3, 3);
        pp.setPen(Qt::white);
        pp.setFont(QFont("Microsoft YaHei", 7, QFont::Bold));
        pp.drawText(QRect(0, 0, CARD_SIZE, CARD_SIZE), Qt::AlignCenter, eq->name().left(2));
    }
    drag->setPixmap(pix);
    drag->setHotSpot(QPoint(CARD_SIZE / 2, CARD_SIZE / 2));

    m_selectedIdx = -1;
    m_game->setSelectedEquipment(nullptr);
    m_pressIndex = -1;
    update();

    drag->exec(Qt::CopyAction);
}

void EquipWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressIndex < 0) return;

    const auto &pool = m_game->equipmentPool();
    int count = pool.size();

    if (m_pressIndex < count) {
        if (m_selectedIdx == m_pressIndex) {
            m_selectedIdx = -1;
            m_game->setSelectedEquipment(nullptr);
        } else {
            m_selectedIdx = m_pressIndex;
            m_game->setSelectedEquipment(pool[m_pressIndex]);
        }
        update();
    }
    m_pressIndex = -1;
}

void EquipWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-synera-equipment"))
        event->acceptProposedAction();
}

void EquipWidget::dropEvent(QDropEvent *event)
{
    // 从 UnitItem 拖回装备到装备池卸下
    if (event->mimeData()->hasFormat("application/x-synera-equipment"))
        event->acceptProposedAction();
}
