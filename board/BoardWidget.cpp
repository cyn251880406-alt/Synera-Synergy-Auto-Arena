#include "board/BoardWidget.h"
#include "unit/UnitItem.h"
#include "ui/ResourceManager.h"
#include <QVector>
#include <QPainter>
#include <QGraphicsPixmapItem>
#include <QResizeEvent>

BoardWidget::BoardWidget(QWidget *parent)
    : QGraphicsView(parent)
{
    initScene();
    initView();
}

// ---- 内部初始化 -----------------------------------------------------------

void BoardWidget::initScene()
{
    m_scene = new QGraphicsScene(this);

    const int sceneW = BOARD_COLS * CELL_SIZE;
    const int sceneH = BOARD_ROWS * CELL_SIZE;
    m_scene->setSceneRect(0, 0, sceneW, sceneH);
    m_scene->setBackgroundBrush(Qt::transparent);

    // 预加载素材
    ResourceManager &res = ResourceManager::instance();
    bool hasTiles = !res.lightTile().isNull() && !res.darkTile().isNull();

    // 创建 8×8 格子（极淡，背景图透出）
    m_cells.resize(BOARD_ROWS);
    for (int row = 0; row < BOARD_ROWS; ++row) {
        m_cells[row].resize(BOARD_COLS);
        for (int col = 0; col < BOARD_COLS; ++col) {
            QGraphicsRectItem *cell = new QGraphicsRectItem(
                col * CELL_SIZE,
                row * CELL_SIZE,
                CELL_SIZE, CELL_SIZE
            );

            if (hasTiles) {
                bool isLight = ((row + col) % 2 == 0);
                QPixmap tile = isLight ? res.lightTile() : res.darkTile();
                cell->setBrush(QBrush(tile.scaled(CELL_SIZE, CELL_SIZE,
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation)));
                cell->setPen(Qt::NoPen);
            } else {
                QColor color = ((row + col) % 2 == 0)
                    ? QColor(255, 255, 255, 40)
                    : QColor(0, 0, 0, 30);
                cell->setBrush(color);
                cell->setPen(QPen(QColor(255, 255, 255, 25), 0.5));
            }

            m_scene->addItem(cell);
            m_cells[row][col] = cell;
        }
    }

    // 半场分隔线（极淡）
    QPen dividerPen = QPen(QColor(255, 255, 255, 25), 1);
    auto *divider = new QGraphicsLineItem(0, 4 * CELL_SIZE, sceneW, 4 * CELL_SIZE);
    divider->setPen(dividerPen);
    divider->setZValue(0.5);
    m_scene->addItem(divider);

    // 初始化占用表
    for (int row = 0; row < BOARD_ROWS; ++row)
        for (int col = 0; col < BOARD_COLS; ++col)
            m_occupancy[row][col] = nullptr;

    setScene(m_scene);
}

void BoardWidget::initView()
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(BOARD_COLS * CELL_SIZE, BOARD_ROWS * CELL_SIZE);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background: transparent");
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void BoardWidget::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (scene())
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

// ---- 公开接口 -------------------------------------------------------------

QGraphicsRectItem *BoardWidget::cellAt(int row, int col) const
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS)
        return nullptr;
    return m_cells[row][col];
}

void BoardWidget::resetColors()
{
    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int col = 0; col < BOARD_COLS; ++col) {
            QColor color = ((row + col) % 2 == 0) ? LIGHT_CELL : DARK_CELL;
            m_cells[row][col]->setBrush(color);
        }
    }
}

// ---- 占用管理 -------------------------------------------------------------

bool BoardWidget::isCellOccupied(int row, int col) const
{
    // 越界检查
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS)
        return false;

    return m_occupancy[row][col] != nullptr;
}

UnitItem *BoardWidget::unitItemAt(int row, int col) const
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS)
        return nullptr;

    return m_occupancy[row][col];
}

bool BoardWidget::occupyCell(int row, int col, UnitItem *item)
{
    // 越界或空指针检查
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS)
        return false;
    if (item == nullptr)
        return false;

    // 若已被占用则不覆盖（防止多个单位叠在同一格）
    if (m_occupancy[row][col] != nullptr)
        return false;

    m_occupancy[row][col] = item;
    return true;
}

void BoardWidget::freeCell(int row, int col)
{
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS)
        return;

    m_occupancy[row][col] = nullptr;
}

bool BoardWidget::placeUnitItem(UnitItem *item, int row, int col)
{
    if (item == nullptr)
        return false;

    // 边界检查
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS)
        return false;

    // 检查目标格是否已被占用
    if (isCellOccupied(row, col))
        return false;

    // 1. 加入场景（如果尚未在场景中）
    if (item->scene() != m_scene)
        m_scene->addItem(item);

    // 2. 设置棋盘坐标并同步视觉位置
    item->setBoardPos(row, col);

    // 3. 在占用表中注册
    m_occupancy[row][col] = item;

    // 4. 告知 UnitItem 它属于此棋盘（拖拽时查询占用状态用）
    item->setBoard(this);

    // 5. 标记拖拽源为棋盘
    item->setDragSource(DragSource::Board);

    return true;
}

// ---- 跨控件拖拽支持 ---------------------------------------------------------

bool BoardWidget::isPointOverWidget(const QPointF &screenPt) const
{
    // 获取本控件在屏幕上的矩形区域
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), size());
    return widgetRect.contains(screenPt.toPoint());
}

QPoint BoardWidget::screenToCell(const QPointF &screenPt) const
{
    // 屏幕坐标 → 视图坐标 → 场景坐标 → 棋盘坐标
    QPoint viewPt = mapFromGlobal(screenPt.toPoint());
    QPointF scenePt = mapToScene(viewPt);

    // 使用 UnitItem 的静态坐标转换（CELL_SIZE_PX = CELL_SIZE = 64）
    int col = static_cast<int>(scenePt.x()) / CELL_SIZE;
    int row = static_cast<int>(scenePt.y()) / CELL_SIZE;

    // 越界检查
    if (row < 0 || row >= BOARD_ROWS || col < 0 || col >= BOARD_COLS) {
        return QPoint(-1, -1);
    }

    return QPoint(col, row);  // x=列, y=行
}

// ---- 键盘事件 ---------------------------------------------------------------

void BoardWidget::keyPressEvent(QKeyEvent *event)
{
    // 空格键 → 发出"开始战斗"信号
    if (event->key() == Qt::Key_Space) {
        emit startBattleRequested();
        event->accept();
        return;
    }

    // 其他按键交给基类处理
    QGraphicsView::keyPressEvent(event);
}
