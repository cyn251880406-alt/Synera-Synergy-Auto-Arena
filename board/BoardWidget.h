#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QKeyEvent>
#include <QVector>

class UnitItem; // 前向声明，避免循环依赖

// 棋盘格子的边长（像素）
constexpr int CELL_SIZE = 64;
// 棋盘行列数
constexpr int BOARD_ROWS = 8;
constexpr int BOARD_COLS = 8;

// 两格交替的颜色（浅色 / 深色，类似国际象棋棋盘）
const QColor LIGHT_CELL(235, 220, 190);
const QColor DARK_CELL(120, 100, 70);

/**
 * @brief BoardWidget — 8x8 棋盘控件
 *
 * 内部持有 QGraphicsScene，并在场景中绘制 BOARD_ROWS x BOARD_COLS
 * 个 QGraphicsRectItem 格子。每个格子大小为 CELL_SIZE x CELL_SIZE。
 *
 * 棋盘分为两个半场：
 *   - 上半场（行 0~3）：敌方半场
 *   - 下半场（行 4~7）：玩家半场（可在准备阶段拖拽单位至此区域）
 *
 * 维护一个 8×8 的占用表，追踪每个格子上是否有单位放置，
 * 用于拖拽过程中的非法放置检测。
 *
 * 使用方式：
 * @code
 *   BoardWidget *board = new BoardWidget;
 *   board->show();
 *
 *   Unit *u = new Unit("战士", Owner::PlayerCtrl, QPoint(3, 0), 300, 30, 1, 60);
 *   UnitItem *item = new UnitItem(u);
 *   board->placeUnitItem(item, 7, 3);  // 放置到第 7 行第 3 列
 * @endcode
 */
class BoardWidget : public QGraphicsView
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);
    ~BoardWidget() override = default;

    /// 返回坐标为 (row, col) 的格子指针，行列范围 [0, BOARD_ROWS-1] / [0, BOARD_COLS-1]
    QGraphicsRectItem *cellAt(int row, int col) const;

    /// 将所有格子颜色重置为默认的深浅交替样式
    void resetColors();

signals:
    /// 玩家按下空格键时发出，用于触发"开始战斗"
    void startBattleRequested();

public:
    // ========== 占用管理（供 UnitItem 拖拽时调用） ==========

    /**
     * @brief 查询指定格子是否被单位占用
     * @return true 表示该格已有单位
     */
    bool isCellOccupied(int row, int col) const;

    /**
     * @brief 返回指定格子上的 UnitItem 指针
     * @return 该格上的单位图元指针；若为空地则返回 nullptr
     */
    UnitItem *unitItemAt(int row, int col) const;

    /**
     * @brief 在指定格子注册一个单位（标记为已占用）
     *
     * 调用时机：单位被拖放到该格、或通过 placeUnitItem 初始放置时。
     * 若该格已被占用，此调用不会覆盖（返回 false）。
     */
    bool occupyCell(int row, int col, UnitItem *item);

    /**
     * @brief 释放指定格子（标记为空地）
     *
     * 调用时机：单位被拖离该格时。
     */
    void freeCell(int row, int col);

    /**
     * @brief 将一个 UnitItem 放置到棋盘的指定格子上
     *
     * 这是一个便捷方法，一次性完成：
     *   1. 将 UnitItem 加入场景
     *   2. 设置其棋盘坐标
     *   3. 在占用表中注册
     *   4. 告知 UnitItem 它属于此棋盘
     *
     * @param item  要放置的单位图元（不能为 nullptr）
     * @param row   目标行号（0~7）
     * @param col   目标列号（0~7）
     * @return true 放置成功；false 目标格已被占用
     */
    bool placeUnitItem(UnitItem *item, int row, int col);

    // ========== 跨控件拖拽支持 ==========

    /**
     * @brief 判断屏幕坐标是否落在本控件范围内
     *
     * 用于 UnitItem 在 mouseReleaseEvent 中判断松手位置。
     * 使用全局屏幕坐标，与 item 所属的场景无关。
     */
    bool isPointOverWidget(const QPointF &screenPt) const;

    /**
     * @brief 屏幕坐标 → 棋盘格子坐标
     *
     * 内部通过 mapFromGlobal → mapToScene → pixelToBoard 链路转换。
     * @param screenPt 全局屏幕坐标
     * @return 棋盘坐标 QPoint(col, row)；无法映射时返回 (-1, -1)
     */
    QPoint screenToCell(const QPointF &screenPt) const;

private:
    void initScene();   // 创建场景与所有格子
    void initView();    // 设置视图属性（缩放、滚动条等）

    /// 键盘按下事件：空格键 → 发射 startBattleRequested() 信号
    void keyPressEvent(QKeyEvent *event) override;

    /// 窗口缩放时自适应棋盘大小
    void resizeEvent(QResizeEvent *event) override;

    QGraphicsScene *m_scene = nullptr;

    // BOARD_ROWS x BOARD_COLS 的格子指针，按 [row][col] 索引
    QVector<QVector<QGraphicsRectItem *>> m_cells;

    // 占用表：m_occupancy[row][col] 指向占据该格的 UnitItem
    // nullptr 表示该格为空
    UnitItem *m_occupancy[BOARD_ROWS][BOARD_COLS] = {};
};

#endif // BOARDWIDGET_H
