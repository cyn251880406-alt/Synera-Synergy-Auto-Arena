#include "UnitItem.h"
#include "Unit.h"
#include "board/BoardWidget.h"
#include "bench/BenchWidget.h"
#include "core/Game.h"
#include "equipment/Equipment.h"
#include "ui/ResourceManager.h"
#include "ui/SellZone.h"
#include <QPainter>
#include <QFont>
#include <QGraphicsSceneDragDropEvent>
#include <QMimeData>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <QWidget>
#include <QLabel>
#include <QtMath>
#include <algorithm>
// ============================================================================
// 静态成员定义
// ============================================================================
bool UnitItem::s_dragLocked = false;

// ============================================================================
// 静态颜色常量定义
// ============================================================================

const QColor UnitItem::PLAYER_COLOR  = QColor(74, 144, 217);   // 柔和蓝——我方单位
const QColor UnitItem::ENEMY_COLOR   = QColor(217, 74, 74);    // 柔和红——敌方单位
const QColor UnitItem::NEUTRAL_COLOR = QColor(180, 180, 180);  // 灰色——无绑定单位

// ============================================================================
// 构造
// ============================================================================

UnitItem::UnitItem(Unit *unit, QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    // 设定图元尺寸 = 棋盘一格大小
    setRect(0, 0, ITEM_SIZE, ITEM_SIZE);

    // 绑定逻辑单位并刷新外观
    bindUnit(unit);

    // 允许接收鼠标事件——拖拽功能的基础
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptDrops(true);  // 接受装备拖放

    // 使用 unit 的实际位置（备战区单位为 (-1,-1)，不可设为 (0,0)）
    if (m_unit) {
        QPoint pos = m_unit->position();
        if (pos.x() >= 0 && pos.y() >= 0)
            setBoardPos(pos.y(), pos.x());
        else
            setPos(0, 0);  // 备战区单位：像素位置由 BenchWidget 后续设定
    } else {
        setPos(0, 0);
    }
}

// ============================================================================
// 单位绑定
// ============================================================================

void UnitItem::bindUnit(Unit *unit)
{
    m_unit = unit;
    if (m_unit) {
        // 有单位：同步坐标并刷新外观
        m_boardRow = m_unit->position().y();
        m_boardCol = m_unit->position().x();
    }
    refreshAppearance();
}

// ============================================================================
// 坐标转换（静态方法）
// ============================================================================

QPointF UnitItem::boardToPixel(int row, int col)
{
    // 每格 CELL_SIZE_PX 像素；像素 x 由列决定，像素 y 由行决定
    return QPointF(col * CELL_SIZE_PX, row * CELL_SIZE_PX);
}

QPointF UnitItem::boardToPixel(const QPoint &boardPos)
{
    return boardToPixel(boardPos.y(), boardPos.x());
    // 注：QPoint 中 x=列, y=行，与 row/col 参数对应 y/x
}

QPoint UnitItem::pixelToBoard(const QPointF &pixelPos)
{
    // 像素除以格大小并向下取整，得到棋盘行列索引
    int col = static_cast<int>(pixelPos.x()) / CELL_SIZE_PX;
    int row = static_cast<int>(pixelPos.y()) / CELL_SIZE_PX;

    // 钳制到棋盘有效范围（8x8，与 BoardWidget 保持一致）
    constexpr int MAX_ROW = 8 - 1;
    constexpr int MAX_COL = 8 - 1;
    col = std::clamp(col, 0, MAX_COL);
    row = std::clamp(row, 0, MAX_ROW);

    return QPoint(col, row); // x=列, y=行
}

// ============================================================================
// 棋盘定位
// ============================================================================

void UnitItem::setBoardPos(int row, int col)
{
    m_boardRow = row;
    m_boardCol = col;

    // 棋盘坐标 → 像素坐标 → 设置图元在场景中的位置
    QPointF pixel = boardToPixel(row, col);
    setPos(pixel);

    // 同步回逻辑单位
    if (m_unit)
        m_unit->setPosition(QPoint(col, row));
}

// ============================================================================
// 拖拽源位置管理
// ============================================================================

void UnitItem::setDragSource(DragSource src, int slot)
{
    m_dragSource = src;
    m_benchSlot = slot;
}

// ============================================================================
// 外观刷新
// ============================================================================

void UnitItem::refreshAppearance()
{
    // 闪光倒计时递减
    if (m_attackFlashCounter > 0) --m_attackFlashCounter;
    if (m_mergeFlashCounter > 0)  --m_mergeFlashCounter;
    if (m_equipFlashCounter > 0)  --m_equipFlashCounter;

    // 触发重绘
    update();
}

void UnitItem::syncVisualPosition()
{
    if (!m_unit)
        return;

    QPoint unitPos = m_unit->position();

    // 坐标有效性检查（(-1, -1) 表示在备战区，无需同步棋盘坐标）
    if (unitPos.x() < 0 || unitPos.y() < 0) {
        m_lastBoardRow = -1;
        m_lastBoardCol = -1;
        return;
    }

    m_boardRow = unitPos.y();
    m_boardCol = unitPos.x();

    QPointF targetPx = boardToPixel(unitPos.y(), unitPos.x());
    QPointF currentPx = QGraphicsItem::pos();  // 显式调用基类 pos() 方法

    // —— 平滑移动动画（帧间插值） ——
    // 战斗中路径移动时，Unit::m_position 瞬间完成跳变，
    // 此处让 UnitItem 逐帧向目标 position 插值，形成平滑滑动效果
    double dx = targetPx.x() - currentPx.x();
    double dy = targetPx.y() - currentPx.y();
    double dist = qSqrt(dx * dx + dy * dy);

    if (dist > 1.0) {
        // 距离目标较远 → 逐帧靠近（每帧移动约 1/4 格 = 64/4 = 16 像素）
        // 约 4 帧完成一次跨格移动，与 MOVE_SPEED=20 帧/格的节奏匹配
        double step = qMin(dist, 16.0);
        double ratio = step / dist;
        setPos(currentPx.x() + dx * ratio, currentPx.y() + dy * ratio);
    } else {
        // 已接近目标 → 吸附到精确位置
        setPos(targetPx);
    }

    // 记录上一帧的棋盘位置（供外部检测位置变化）
    m_lastBoardRow = m_boardRow;
    m_lastBoardCol = m_boardCol;
}

// ============================================================================
// 鼠标事件 —— 拖拽实现
// ============================================================================

void UnitItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // —— 右键：卸下装备 / 出售备战区单位 ——
    if (event->button() == Qt::RightButton) {
        if (m_unit && m_unit->equipment() && m_game) {
            // 有装备 → 卸下装备归还池
            Equipment *removed = removeEquipment(m_unit);
            if (removed) {
                m_game->addEquipmentToPool(removed);
                update();
            }
        } else if (m_unit && m_game
                   && m_unit->owner() == Owner::PlayerCtrl
                   && m_dragSource == DragSource::Bench
                   && m_benchSlot >= 0) {
            // 无装备且在备战区 → 出售（会删除 this，必须最后执行）
            event->ignore();
            m_game->sellFromBench(m_benchSlot);
            return;
        }
        event->ignore();
        return;
    }

    // —— 左键 + 有选中装备 → 装备到该单位 ——
    if (event->button() == Qt::LeftButton && m_game && m_unit
        && m_unit->owner() == Owner::PlayerCtrl
        && m_game->selectedEquipment())
    {
        Equipment *eq = m_game->selectedEquipment();
        // 若单位已有装备，卸下并归还池
        if (m_unit->equipment()) {
            Equipment *old = removeEquipment(m_unit);
            if (old)
                m_game->addEquipmentToPool(old);
        }
        // 装备新物品
        equipItem(m_unit, eq);
        m_game->removeEquipmentFromPool(eq);
        m_game->setSelectedEquipment(nullptr);
        update();
        refreshAppearance();
        event->ignore();
        return;
    }

    // 只处理左键拖拽
    if (event->button() != Qt::LeftButton) {
        QGraphicsRectItem::mousePressEvent(event);
        return;
    }

    // 敌方单位不可拖拽
    if (m_unit && m_unit->owner() == Owner::EnemyCtrl) {
        event->ignore();
        return;
    }

    // 全局拖拽锁定（战斗阶段由 Game 设置，禁止调整站位）
    if (s_dragLocked) {
        event->ignore();
        return;
    }

    // 记录拖拽前的原始位置
    m_dragOrigRow = m_boardRow;
    m_dragOrigCol = m_boardCol;
    m_dragOrigSource = m_dragSource;
    m_dragOrigSlot  = m_benchSlot;
    m_dragging = true;

    // 提升 Z 值，使拖拽中的单位显示在所有其他图元之上
    setZValue(1.0);

    // 清除上次可能的备战区高亮残留
    if (m_benchWidget)
        m_benchWidget->clearHighlight();

    event->accept();
}

void UnitItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragging) {
        QGraphicsRectItem::mouseMoveEvent(event);
        return;
    }

    QPointF screenPt = event->screenPos();

    // ——— 跨控件场景切换 ———
    if (m_board && m_board->isPointOverWidget(screenPt)) {
        if (scene() != m_board->scene()) {
            if (scene())
                scene()->removeItem(this);
            m_board->scene()->addItem(this);
        }
        QPoint viewPt = m_board->mapFromGlobal(
            QPoint(static_cast<int>(screenPt.x()), static_cast<int>(screenPt.y())));
        QPointF scenePt = m_board->mapToScene(viewPt);
        setPos(scenePt.x() - ITEM_SIZE / 2.0, scenePt.y() - ITEM_SIZE / 2.0);
    } else if (m_benchWidget && m_benchWidget->isPointOverWidget(screenPt)) {
        if (scene() != m_benchWidget->scene()) {
            if (scene())
                scene()->removeItem(this);
            m_benchWidget->scene()->addItem(this);
        }
        QPoint viewPt = m_benchWidget->mapFromGlobal(
            QPoint(static_cast<int>(screenPt.x()), static_cast<int>(screenPt.y())));
        QPointF scenePt = m_benchWidget->mapToScene(viewPt);
        setPos(scenePt.x() - ITEM_SIZE / 2.0, scenePt.y() - ITEM_SIZE / 2.0);
    } else {
        QPointF delta = event->scenePos() - event->lastScenePos();
        setPos(pos() + delta);
    }

    // ——— 拖拽落点高亮检测 ———
    if (m_benchWidget && m_benchWidget->isPointOverWidget(screenPt)) {
        int slot = m_benchWidget->screenToSlot(screenPt);
        if (slot >= 0) {
            bool valid = !m_benchWidget->isSlotOccupied(slot);
            m_benchWidget->setHighlightSlot(slot, valid);
        }
    } else {
        if (m_benchWidget)
            m_benchWidget->clearHighlight();
    }

    event->accept();
}

void UnitItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_dragging) {
        QGraphicsRectItem::mouseReleaseEvent(event);
        return;
    }

    m_dragging = false;

    // 恢复 Z 值
    setZValue(0.0);

    // ---------- 1. 获取鼠标在屏幕上的全局坐标 ----------
    // screenPos 是相对于整个显示器的像素坐标，
    // 不依赖于当前所属的 QGraphicsScene，因此可以跨控件判断落点
    QPointF screenPt = event->screenPos();

    // ---------- 2. 判断落点区域 ----------

    // 保存局部引用（handleDropOnBoard 可能 delete this）
    BenchWidget *bw = m_benchWidget;

    // 优先检测备战区（范围更小，更具体）
    if (bw && bw->isPointOverWidget(screenPt)) {
        handleDropOnBench(screenPt);
    }
    // 其次检测棋盘
    else if (m_board && m_board->isPointOverWidget(screenPt)) {
        handleDropOnBoard(screenPt);
    }
    // 检测出售区域
    else if (m_sellZone && m_sellZone->isPointOverWidget(screenPt)
             && m_game && m_unit && m_unit->owner() == Owner::PlayerCtrl) {
        // 清理高亮（必须在 sell 前，因为 sell 会 delete this）
        if (bw)
            bw->clearHighlight();
        event->accept();

        if (m_dragOrigSource == DragSource::Bench && m_dragOrigSlot >= 0) {
            m_game->sellFromBench(m_dragOrigSlot);
        } else if (m_dragOrigSource == DragSource::Board) {
            m_game->sellDeployedUnit(m_unit);
        }
        return;  // sell 已 delete this，不可再访问成员
    }
    // 落在两者之外 → 弹回原位
    else {
        bounceBackToSource();
    }

    // 清除备战区拖拽高亮（使用局部引用，防止 this 已被删除）
    if (bw)
        bw->clearHighlight();

    event->accept();
}

// ============================================================================
// 装备拖放（从 EquipWidget 拖拽装备到 UnitItem 上）
// ============================================================================

void UnitItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
    if (m_unit && m_unit->owner() == Owner::PlayerCtrl
        && event->mimeData()->hasFormat("application/x-synera-equipment")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void UnitItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    if (m_unit && m_unit->owner() == Owner::PlayerCtrl
        && event->mimeData()->hasFormat("application/x-synera-equipment")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void UnitItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    Q_UNUSED(event);
    update();
}

void UnitItem::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    if (!m_game || !m_unit || m_unit->owner() != Owner::PlayerCtrl) {
        event->ignore();
        return;
    }

    const QMimeData *mime = event->mimeData();
    if (!mime->hasFormat("application/x-synera-equipment")) {
        event->ignore();
        return;
    }

    QString typeName = QString::fromUtf8(
        mime->data("application/x-synera-equipment"));

    const auto &pool = m_game->equipmentPool();
    for (int i = 0; i < pool.size(); ++i) {
        if (pool[i]->typeName() == typeName) {
            Equipment *eq = pool[i];
            // 先卸下旧装备
            if (m_unit->equipment()) {
                Equipment *old = removeEquipment(m_unit);
                if (old) m_game->addEquipmentToPool(old);
            }
            equipItem(m_unit, eq);
            m_game->removeEquipmentFromPool(eq);
            m_game->setSelectedEquipment(nullptr);

            triggerEquipFlash();
            refreshAppearance();
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

// ============================================================================
// 拖拽落点处理（私有辅助）
// ============================================================================

void UnitItem::handleDropOnBench(const QPointF &screenPt)
{
    // 将屏幕坐标转为备战区槽位索引
    int targetSlot = m_benchWidget->screenToSlot(screenPt);
    if (targetSlot < 0) {
        // 无法映射到有效槽位（鼠标在备战区边角）
        bounceBackToSource();
        return;
    }

    if (m_dragOrigSource == DragSource::Bench) {
        // ====== 备战区 → 备战区 ======

        // 确保 item 在备战区场景中（mouseMoveEvent 可能已将其切换到棋盘场景）
        if (scene() != m_benchWidget->scene()) {
            if (scene())
                scene()->removeItem(this);
            m_benchWidget->scene()->addItem(this);
        }

        if (targetSlot == m_dragOrigSlot) {
            // 放回原位：弹回即可（无需转移）
            bounceBackToSource();
            return;
        }

        // 目标槽位有单位 → 交换两个槽位
        // 目标槽位为空   → 移动到目标槽位
        m_benchWidget->swapOrMoveUnitItem(m_dragOrigSlot, targetSlot);
    } else {
        // ====== 棋盘 → 备战区 ======

        // 若目标槽位已被占用，拒绝放置（弹回棋盘原位）
        if (m_benchWidget->isSlotOccupied(targetSlot)) {
            bounceBackToSource();
            return;
        }

        // 1. 从棋盘占用表中释放
        m_board->freeCell(m_dragOrigRow, m_dragOrigCol);

        // 2. 从棋盘场景中移出（不 delete）
        if (m_board->scene())
            m_board->scene()->removeItem(this);

        // 3. 加入备战区（场景 + 视图注册 + 逻辑同步）
        m_benchWidget->acceptUnitItem(this, targetSlot);

        // 4. 更新自身状态
        m_dragSource = DragSource::Bench;
        m_benchSlot  = targetSlot;
    }
}

void UnitItem::handleDropOnBoard(const QPointF &screenPt)
{
    // 将屏幕坐标转为棋盘格子坐标
    QPoint cell = m_board->screenToCell(screenPt);
    int targetRow = cell.y();
    int targetCol = cell.x();

    // 合法性检查 1：必须在玩家半场（行 4~7）
    if (targetRow < 4 || targetRow > 7) {
        bounceBackToSource();
        return;
    }

    // —— 升星合并检测：拖拽到被占用的格子 → 检测同名同星合并 ——
    if (m_board->isCellOccupied(targetRow, targetCol)) {
        bool isOwnCell = (m_dragOrigSource == DragSource::Board)
                      && (targetRow == m_dragOrigRow)
                      && (targetCol == m_dragOrigCol);

        // 尝试拖拽合并（非原位，且目标格有我方单位）
        if (!isOwnCell && m_game && m_unit) {
            UnitItem *targetItem = m_board->unitItemAt(targetRow, targetCol);
            if (targetItem && targetItem->unit()) {
                // 保存局部引用（triggerDragMerge 会 delete this）
                BenchWidget *bw = m_benchWidget;
                if (m_game->triggerDragMerge(m_unit, targetItem->unit())) {
                    // 合并成功（this 已被删除，用局部引用）
                    if (bw)
                        bw->clearHighlight();
                    return;
                }
            }
        }

        if (!isOwnCell) {
            bounceBackToSource();
            return;
        }
    }

    if (m_dragOrigSource == DragSource::Board) {
        // ====== 棋盘 → 棋盘（原有逻辑） ======

        // 释放旧格子
        m_board->freeCell(m_dragOrigRow, m_dragOrigCol);
        // 占据新格子
        m_board->occupyCell(targetRow, targetCol, this);
        // 更新自身位置
        setBoardPos(targetRow, targetCol);
    } else {
        // ====== 备战区 → 棋盘 ======

        // 人口上限检查：当前已部署数已达上限则弹回
        if (m_game) {
            int deployed = m_game->deployedPlayerUnits().size();
            if (!m_game->player().canDeploy(deployed)) {
                bounceBackToSource();
                return;
            }
        }

        // 1. 从备战区移出（场景 + 视图注册 + 逻辑同步）
        m_benchWidget->releaseUnitItem(m_dragOrigSlot);

        // 2. 放入棋盘（场景转移 + 占用注册 + 定位）
        //    先加入棋盘场景（如果不在）
        if (scene() != m_board->scene()) {
            // 从当前场景（备战区场景）移出
            if (scene())
                scene()->removeItem(this);
            // 加入棋盘场景
            m_board->scene()->addItem(this);
        }
        m_board->occupyCell(targetRow, targetCol, this);
        setBoardPos(targetRow, targetCol);
        setBoard(m_board);  // 重新关联棋盘

        // 3. 更新自身状态
        m_dragSource = DragSource::Board;
        m_benchSlot  = -1;
    }
}

void UnitItem::bounceBackToSource()
{
    // 清除备战区高亮（防止弹回后残留）
    if (m_benchWidget)
        m_benchWidget->clearHighlight();

    if (m_dragOrigSource == DragSource::Board) {
        // —— 弹回棋盘原位 ——
        // 注意：若跨场景拖拽失败（Board→Bench 但 Bench 槽被占用），
        // item 可能已被移出棋盘场景，需要重新加入
        if (scene() != m_board->scene()) {
            // 从当前场景移出
            if (scene())
                scene()->removeItem(this);
            // 重新加入棋盘场景
            if (m_board->scene())
                m_board->scene()->addItem(this);
            // 重新注册占用
            m_board->occupyCell(m_dragOrigRow, m_dragOrigCol, this);
        }
        setBoardPos(m_dragOrigRow, m_dragOrigCol);
        m_dragSource = DragSource::Board;
        m_benchSlot  = -1;
    } else {
        // —— 弹回备战区原位 ——
        // 若 item 已被移出备战区场景（Bench→Board 失败），重新加入
        if (m_benchWidget && scene() != m_benchWidget->scene()) {
            if (scene())
                scene()->removeItem(this);
            m_benchWidget->acceptUnitItem(this, m_dragOrigSlot);
        } else {
            // 仍在备战区场景中，只需重新定位
            setPos(BenchWidget::slotToPixel(m_dragOrigSlot));
        }
        m_dragSource = DragSource::Bench;
        m_benchSlot  = m_dragOrigSlot;
    }
}

// ============================================================================
// 动画
// ============================================================================

void UnitItem::triggerAttackFlash()
{
    m_attackFlashCounter = FLASH_DURATION;
}

void UnitItem::triggerMergeFlash()
{
    m_mergeFlashCounter = FLASH_DURATION * 2;  // 合成闪光更久
}

void UnitItem::triggerEquipFlash()
{
    m_equipFlashCounter = FLASH_DURATION;
}

// ============================================================================
// 自定义绘制
// ============================================================================

void UnitItem::paint(QPainter *painter,
                     const QStyleOptionGraphicsItem * /*option*/,
                     QWidget * /*widget*/)
{
    const QRectF r = rect();
    const double cornerRadius = 4.0;  // 圆角半径
    const bool hasIcon = m_unit && !ResourceManager::instance().unitIcon(m_unit->name()).isNull();

    painter->setRenderHint(QPainter::Antialiasing);

    // ——— 第 1 层：底色 / 角色图标 ———
    painter->setPen(Qt::NoPen);

    if (hasIcon) {
        // —— 角色图标模式 ——
        QPixmap icon = ResourceManager::instance().unitIcon(m_unit->name());

        // 图标区域（上方主体，留出底部血条空间）
        const double iconAreaHeight = r.height() - HP_BAR_HEIGHT - MANA_BAR_HEIGHT - BAR_BOTTOM_MARGIN - 4;
        QRectF iconRect(r.left() + 4, r.top() + 4,
                        r.width() - 8, iconAreaHeight - 4);

        // 先画圆角背景（我方蓝灰 / 敌方红灰）
        QColor bgColor = (m_unit->owner() == Owner::PlayerCtrl)
            ? QColor(40, 50, 70) : QColor(60, 35, 35);
        painter->setBrush(bgColor);
        painter->drawRoundedRect(r, cornerRadius, cornerRadius);

        // 图标区域暗底
        painter->setBrush(QColor(20, 22, 28));
        painter->drawRoundedRect(iconRect, 3, 3);

        // 绘制角色图标（居中缩放）
        QPixmap scaled = icon.scaled(iconRect.size().toSize() * 0.85,
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPointF iconPos = iconRect.center() - QPointF(scaled.width() / 2.0, scaled.height() / 2.0);
        painter->drawPixmap(iconPos, scaled);

        // 单位名称（图标底部小字）
        painter->setPen(QColor(220, 220, 220));
        QFont nameFont("Microsoft YaHei", 7);
        painter->setFont(nameFont);
        QRectF nameRect(iconRect.left(), iconRect.bottom() - 10, iconRect.width(), 10);
        painter->drawText(nameRect, Qt::AlignHCenter | Qt::AlignBottom, m_unit->name());

    } else {
        // —— 程序化绘制模式（无素材时回退） ——
        QColor base = baseColor();
        // 顶部高光渐层
        QLinearGradient grad(r.topLeft(), r.bottomRight());
        grad.setColorAt(0.0, base.lighter(120));
        grad.setColorAt(0.6, base);
        grad.setColorAt(1.0, base.darker(130));
        painter->setBrush(grad);
        painter->drawRoundedRect(r, cornerRadius, cornerRadius);

        // 单位名称（居中大字）
        if (m_unit) {
            painter->setPen(Qt::white);
            QFont nameFont("Microsoft YaHei", 9, QFont::Bold);
            painter->setFont(nameFont);
            QRectF nameRect = r.adjusted(2, 2, -2,
                -(HP_BAR_HEIGHT + MANA_BAR_HEIGHT + BAR_BOTTOM_MARGIN + 2));
            painter->drawText(nameRect, Qt::AlignHCenter | Qt::AlignVCenter,
                              m_unit->name());
        }
    }

    // ——— 闪光效果（叠加在图标之上） ———
    if (m_attackFlashCounter > 0) {
        int alpha = 120 * m_attackFlashCounter / FLASH_DURATION;
        painter->setBrush(QColor(255, 255, 255, alpha));
        painter->drawRoundedRect(r, cornerRadius, cornerRadius);
    }
    if (m_mergeFlashCounter > 0) {
        int alpha = 150 * m_mergeFlashCounter / (FLASH_DURATION * 2);
        painter->setBrush(QColor(255, 215, 0, alpha));
        painter->drawRoundedRect(r, cornerRadius, cornerRadius);
    }

    // ——— 边框 ———
    painter->setBrush(Qt::NoBrush);
    if (m_unit) {
        QColor borderColor = (m_unit->owner() == Owner::PlayerCtrl)
            ? QColor(74, 144, 217, 180) : QColor(217, 74, 74, 180);
        painter->setPen(QPen(borderColor, 1.2));
    } else {
        painter->setPen(QPen(QColor(80, 80, 80), 1.0));
    }
    painter->drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), cornerRadius, cornerRadius);

    // 空单位
    if (!m_unit) {
        painter->setPen(QColor(160, 160, 160));
        painter->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
        painter->drawText(r, Qt::AlignCenter, QString::fromUtf8("空"));
        return;
    }

    // ——— 血条和蓝条（两种模式共用） ———
    const double barLeft   = r.left() + 3;
    const double barWidth  = r.width() - 6;
    const double hpBarY    = r.bottom() - HP_BAR_HEIGHT - MANA_BAR_HEIGHT - BAR_BOTTOM_MARGIN;
    const QRectF hpBgRect(barLeft, hpBarY, barWidth, HP_BAR_HEIGHT);

    // HP 背景
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(40, 0, 0));
    painter->drawRoundedRect(hpBgRect, 1.5, 1.5);

    // HP 填充
    double hpRatio = (m_unit->maxHp() > 0)
        ? static_cast<double>(m_unit->hp()) / m_unit->maxHp() : 0.0;
    if (hpRatio < 0.0) hpRatio = 0.0;
    if (hpRatio > 0.0) {
        QRectF hpFillRect(barLeft, hpBarY, barWidth * hpRatio, HP_BAR_HEIGHT);
        int red   = static_cast<int>(255 * (1.0 - hpRatio));
        int green = static_cast<int>(255 * hpRatio);
        painter->setBrush(QColor(red, green, 40));
        painter->drawRoundedRect(hpFillRect, 1.5, 1.5);
    }

    // HP 边框
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(60, 60, 60), 0.5));
    painter->drawRoundedRect(hpBgRect, 1.5, 1.5);

    // Mana 背景
    const double manaBarY = hpBarY + HP_BAR_HEIGHT + 1;
    const QRectF manaBgRect(barLeft, manaBarY, barWidth, MANA_BAR_HEIGHT);
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 50));
    painter->drawRoundedRect(manaBgRect, 1.5, 1.5);

    // Mana 填充
    double manaRatio = (m_unit->maxMana() > 0)
        ? static_cast<double>(m_unit->mana()) / m_unit->maxMana() : 0.0;
    if (manaRatio < 0.0) manaRatio = 0.0;
    if (manaRatio > 0.0) {
        QRectF manaFillRect(barLeft, manaBarY, barWidth * manaRatio, MANA_BAR_HEIGHT);
        painter->setBrush(QColor(70, 130, 240));
        painter->drawRoundedRect(manaFillRect, 1.5, 1.5);
    }

    // Mana 边框
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(60, 60, 60), 0.5));
    painter->drawRoundedRect(manaBgRect, 1.5, 1.5);

    // ——— 星级（右上角） ———
    {
        int starLevel = m_unit->starLevel();
        QColor starColor = (starLevel >= 3) ? QColor(255, 100, 50)  // 3星橙红
                         : (starLevel >= 2) ? QColor(255, 215, 0)   // 2星金
                         : QColor(180, 180, 180);                    // 1星银灰
        QFont starFont("Microsoft YaHei", 9, QFont::Bold);
        painter->setFont(starFont);
        QString stars;
        for (int i = 0; i < starLevel; ++i)
            stars += QStringLiteral("★");
        painter->setPen(starColor);
        QRectF starRect(r.right() - 26, r.top() + 1, 24, 13);
        painter->drawText(starRect, Qt::AlignRight | Qt::AlignTop, stars);
    }

    // ——— 装备标记（左下角） ———
    if (m_unit->equipment()) {
        QPixmap eqIcon = ResourceManager::instance().equipmentIcon(
            m_unit->equipment()->typeName());
        if (!eqIcon.isNull()) {
            // 装备图标模式
            QRectF eqIconRect(r.left() + 3, hpBarY - 16, 14, 14);
            painter->drawPixmap(eqIconRect.toRect(), eqIcon.scaled(14, 14,
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            // 回退：文字
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(100, 200, 255));
            QFont eqFont("Microsoft YaHei", 7);
            painter->setFont(eqFont);
            QString eqShort = m_unit->equipment()->name().left(2);
            QRectF eqRect(r.left() + 2, hpBarY - 14, 22, 12);
            painter->drawText(eqRect, Qt::AlignLeft | Qt::AlignBottom, eqShort);
        }

        if (m_equipFlashCounter > 0) {
            painter->setBrush(QColor(100, 200, 255, 60));
            painter->drawRoundedRect(r, cornerRadius, cornerRadius);
        }
    }
}

// ============================================================================
// 内部辅助方法
// ============================================================================

QColor UnitItem::baseColor() const
{
    if (!m_unit)
        return NEUTRAL_COLOR;

    switch (m_unit->owner()) {
    case Owner::PlayerCtrl: return PLAYER_COLOR;
    case Owner::EnemyCtrl:  return ENEMY_COLOR;
    default:                return NEUTRAL_COLOR;
    }
}

