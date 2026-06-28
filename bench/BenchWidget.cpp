#include "bench/BenchWidget.h"
#include "bench/Bench.h"
#include "unit/UnitItem.h"
#include "ui/ResourceManager.h"

#include <QPainter>
#include <QResizeEvent>
#include <algorithm>

// ============================================================================
// 静态颜色常量
// ============================================================================

const QColor BenchWidget::EMPTY_SLOT_COLOR  = QColor(60, 60, 60);     // 深灰底色
const QColor BenchWidget::EMPTY_SLOT_BORDER = QColor(140, 140, 140);  // 浅灰边框

const QColor BenchWidget::VALID_HIGHLIGHT_COLOR   = QColor(40, 140, 40);   // 暗绿 — 可放置
const QColor BenchWidget::INVALID_HIGHLIGHT_COLOR = QColor(140, 40, 40);  // 暗红 — 不可放置

// ============================================================================
// 构造
// ============================================================================

BenchWidget::BenchWidget(QWidget *parent)
    : QGraphicsView(parent)
{
    initScene();
    initView();
}

// ============================================================================
// 初始化
// ============================================================================

void BenchWidget::initScene()
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, SCENE_WIDTH, SCENE_HEIGHT);

    ResourceManager &res = ResourceManager::instance();
    QPixmap benchBg = res.benchSlotBg();

    // 创建 8 个槽位格子，水平排列
    for (int i = 0; i < SLOT_COUNT; ++i) {
        // 每个格子的 x 坐标 = slot 索引 * (槽宽 + 间距)
        double x = i * (SLOT_SIZE + SLOT_SPACING);

        QGraphicsRectItem *cell = new QGraphicsRectItem(x, 0, SLOT_SIZE, SLOT_SIZE);

        if (!benchBg.isNull()) {
            cell->setBrush(QBrush(benchBg.scaled(SLOT_SIZE, SLOT_SIZE,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
        } else {
            cell->setBrush(EMPTY_SLOT_COLOR);
        }
        cell->setPen(QPen(EMPTY_SLOT_BORDER, 1.0));

        m_scene->addItem(cell);
        m_slots[i] = cell;

        // 记录初始底色（供拖拽高亮恢复使用）
        m_slotBaseColors[i] = benchBg.isNull() ? EMPTY_SLOT_COLOR : QColor(60, 60, 60);
    }

    setScene(m_scene);
}

void BenchWidget::initView()
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(70);
    setMinimumWidth(SCENE_WIDTH);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void BenchWidget::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (scene())
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

// ============================================================================
// 与 Bench 绑定
// ============================================================================

void BenchWidget::setBench(Bench *bench)
{
    m_bench = bench;
}

// ============================================================================
// UnitItem 管理
// ============================================================================

bool BenchWidget::setUnitItem(int slot, UnitItem *item)
{
    if (!isValidSlot(slot) || item == nullptr)
        return false;

    // 若该槽位已有单位，先移除旧的（不 delete）
    if (m_unitItems[slot] != nullptr) {
        m_scene->removeItem(m_unitItems[slot]);
    }

    // 加入场景（如果尚未在场景中）
    if (item->scene() != m_scene) {
        m_scene->addItem(item);
    }

    // 将 UnitItem 定位到此槽位的像素坐标
    QPointF pixel = slotToPixel(slot);
    item->setPos(pixel);

    m_unitItems[slot] = item;

    // 同步 UnitItem 的拖拽源状态
    item->setDragSource(DragSource::Bench, slot);

    // 高亮槽位表示被占用
    setSlotBaseColor(slot, QColor(80, 80, 80));

    return true;
}

UnitItem *BenchWidget::takeUnitItem(int slot)
{
    if (!isValidSlot(slot))
        return nullptr;

    UnitItem *item = m_unitItems[slot];
    if (item == nullptr)
        return nullptr;

    // 从场景中移出（不 delete，所有权在调用方）
    m_scene->removeItem(item);
    m_unitItems[slot] = nullptr;

    // 恢复空槽位的默认颜色
    setSlotBaseColor(slot, EMPTY_SLOT_COLOR);

    return item;
}

UnitItem *BenchWidget::unitItemAt(int slot) const
{
    if (!isValidSlot(slot))
        return nullptr;
    return m_unitItems[slot];
}

// ============================================================================
// 槽位查询
// ============================================================================

bool BenchWidget::isSlotOccupied(int slot) const
{
    if (!isValidSlot(slot))
        return false;
    return m_unitItems[slot] != nullptr;
}

int BenchWidget::firstEmptySlot() const
{
    for (int i = 0; i < SLOT_COUNT; ++i) {
        if (m_unitItems[i] == nullptr)
            return i;
    }
    return -1;
}

// ============================================================================
// 坐标转换（静态）
// ============================================================================

QPointF BenchWidget::slotToPixel(int slot)
{
    double x = slot * (SLOT_SIZE + SLOT_SPACING);
    return QPointF(x, 1.0);  // 1px 顶部边距
}

int BenchWidget::pixelToSlot(const QPointF &pixelPos)
{
    // 像素 x / (槽宽 + 间距) 得到浮点索引，向下取整
    int slot = static_cast<int>(pixelPos.x()) / (SLOT_SIZE + SLOT_SPACING);

    // 钳制到有效范围
    if (slot < 0) slot = 0;
    if (slot >= SLOT_COUNT) slot = SLOT_COUNT - 1;

    return slot;
}

// ============================================================================
// 外观刷新
// ============================================================================

void BenchWidget::refreshDisplay()
{
    // 如果没有绑定 Bench，不做任何操作
    if (m_bench == nullptr)
        return;

    // 遍历所有槽位，将 UnitItem 重新定位到正确的像素坐标
    for (int i = 0; i < SLOT_COUNT; ++i) {
        Unit *unit = m_bench->getUnit(i);

        if (unit != nullptr) {
            // Bench 有单位：更新槽位高亮
            setSlotBaseColor(i, QColor(80, 80, 80));

            // 若视觉槽位有 UnitItem，重新定位到正确像素坐标
            if (m_unitItems[i] != nullptr) {
                QPointF pixel = slotToPixel(i);
                m_unitItems[i]->setPos(pixel);
            }
            // 注：如果 Bench 有 Unit 但视觉槽位无 UnitItem，
            // 说明调用方尚未通过 setUnitItem() 设置图元。
            // 此处在不持有 UnitItem 的情况下不做自动创建。
        } else {
            // Bench 空槽：恢复默认颜色（保留已有的 UnitItem 不删除）
            if (m_unitItems[i] == nullptr) {
                setSlotBaseColor(i, EMPTY_SLOT_COLOR);
            }
        }
    }
}

QGraphicsRectItem *BenchWidget::slotItem(int index) const
{
    if (!isValidSlot(index))
        return nullptr;
    return m_slots[index];
}

// ============================================================================
// 跨控件拖拽支持
// ============================================================================

bool BenchWidget::isPointOverWidget(const QPointF &screenPt) const
{
    // 获取本控件在屏幕上的矩形区域
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    return widgetRect.contains(screenPt.toPoint());
}

int BenchWidget::screenToSlot(const QPointF &screenPt) const
{
    // 屏幕坐标 → 视图坐标 → 场景坐标 → 槽位索引
    QPoint viewPt = mapFromGlobal(screenPt.toPoint());
    QPointF scenePt = mapToScene(viewPt);
    return pixelToSlot(scenePt);
}

void BenchWidget::acceptUnitItem(UnitItem *item, int slot)
{
    if (!isValidSlot(slot) || item == nullptr)
        return;

    // 1. 加入备战区场景（若不在当前场景中）
    if (item->scene() != m_scene) {
        m_scene->addItem(item);
    }

    // 2. 定位到指定槽位的像素坐标
    QPointF pixel = slotToPixel(slot);
    item->setPos(pixel);

    // 3. 若该槽位已有旧 UnitItem（且不是同一个item），先移出
    if (m_unitItems[slot] != nullptr && m_unitItems[slot] != item) {
        m_scene->removeItem(m_unitItems[slot]);
    }

    // 4. 注册到视觉占用表
    m_unitItems[slot] = item;

    // 5. 高亮槽位表示被占用，并记录为底色
    setSlotBaseColor(slot, QColor(80, 80, 80));

    // 6. 同步到 Bench 逻辑层：将 unit 放入指定槽位
    if (m_bench && item->unit()) {
        // 先查找 unit 是否已在备战区中（可能在其他槽位）
        int existingSlot = m_bench->indexOf(item->unit());
        if (existingSlot >= 0 && existingSlot != slot) {
            // 从旧槽位移到新槽位
            m_bench->removeUnit(existingSlot);
        }
        // 放入目标槽位
        m_bench->setUnit(slot, item->unit());
    }

    // 7. 更新 UnitItem 的拖拽源状态
    item->setDragSource(DragSource::Bench, slot);
}

UnitItem *BenchWidget::releaseUnitItem(int slot)
{
    if (!isValidSlot(slot))
        return nullptr;

    UnitItem *item = m_unitItems[slot];
    if (item == nullptr)
        return nullptr;

    // 1. 从场景中移出（不 delete）
    m_scene->removeItem(item);

    // 2. 清除视觉注册
    m_unitItems[slot] = nullptr;

    // 3. 恢复空槽位颜色，并更新底色记录
    setSlotBaseColor(slot, EMPTY_SLOT_COLOR);

    // 4. 同步到 Bench 逻辑层
    if (m_bench) {
        m_bench->removeUnit(slot);
    }

    // 5. 清除 UnitItem 的拖拽源状态
    item->setDragSource(DragSource::Board, -1);

    return item;
}

void BenchWidget::swapOrMoveUnitItem(int slotA, int slotB)
{
    if (!isValidSlot(slotA) || !isValidSlot(slotB))
        return;
    if (slotA == slotB)
        return;

    UnitItem *itemA = m_unitItems[slotA];
    UnitItem *itemB = m_unitItems[slotB];

    // 两个槽位都为空 → 无操作
    if (itemA == nullptr && itemB == nullptr)
        return;

    // —— 交换视觉注册 ——
    std::swap(m_unitItems[slotA], m_unitItems[slotB]);

    // —— 重新定位两个 UnitItem ——
    if (m_unitItems[slotA])
        m_unitItems[slotA]->setPos(slotToPixel(slotA));
    if (m_unitItems[slotB])
        m_unitItems[slotB]->setPos(slotToPixel(slotB));

    // —— 更新两个 UnitItem 的拖拽源状态 ——
    if (m_unitItems[slotA])
        m_unitItems[slotA]->setDragSource(DragSource::Bench, slotA);
    if (m_unitItems[slotB])
        m_unitItems[slotB]->setDragSource(DragSource::Bench, slotB);

    // —— 同步到 Bench 逻辑层 ——
    if (m_bench) {
        m_bench->swapUnits(slotA, slotB);
    }

    // —— 更新槽位颜色（确保有单位→高亮，空→默认） ——
    for (int s : {slotA, slotB}) {
        setSlotBaseColor(s, m_unitItems[s] != nullptr
                           ? QColor(80, 80, 80)
                           : EMPTY_SLOT_COLOR);
    }
}

// ============================================================================
// 拖拽高亮
// ============================================================================

void BenchWidget::setHighlightSlot(int slot, bool valid)
{
    // 清除旧高亮
    if (m_highlightedSlot >= 0 && m_highlightedSlot < SLOT_COUNT
        && m_highlightedSlot != slot)
    {
        // 恢复为底色
        m_slots[m_highlightedSlot]->setBrush(
            m_slotBaseColors[m_highlightedSlot]);
    }

    m_highlightedSlot = slot;

    if (slot >= 0 && slot < SLOT_COUNT) {
        m_slots[slot]->setBrush(
            valid ? VALID_HIGHLIGHT_COLOR : INVALID_HIGHLIGHT_COLOR);
    }
}

void BenchWidget::clearHighlight()
{
    if (m_highlightedSlot >= 0 && m_highlightedSlot < SLOT_COUNT) {
        m_slots[m_highlightedSlot]->setBrush(
            m_slotBaseColors[m_highlightedSlot]);
    }
    m_highlightedSlot = -1;
}

// ============================================================================
// 内部辅助
// ============================================================================

bool BenchWidget::isValidSlot(int slot) const
{
    return slot >= 0 && slot < SLOT_COUNT;
}

void BenchWidget::setSlotBaseColor(int slot, const QColor &color)
{
    if (!isValidSlot(slot))
        return;
    m_slotBaseColors[slot] = color;
    // 仅当没有高亮时才更新显示颜色
    if (m_highlightedSlot != slot)
        m_slots[slot]->setBrush(color);
}
