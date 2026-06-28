#ifndef UNITITEM_H
#define UNITITEM_H
#include <QGraphicsRectItem>
#include <QString>
class Unit;
class BoardWidget;
class BenchWidget;
class Game;
class SellZone;
class QLabel;
// 棋盘格子像素大小（与 BoardWidget.h 中的定义保持一致）
constexpr int CELL_SIZE_PX = 64;

// ============================================================================
// UnitItem — 单位的 QGraphicsRectItem 可视化封装
// ============================================================================

/**
 * @brief UnitItem — 在棋盘上可视化显示一个 Unit
 *
 * 继承 QGraphicsRectItem（Qt 场景图元），将逻辑层的 Unit 对象渲染为
 * 棋盘上的可见单位。每个 UnitItem：
 *   - 绑定一个 Unit*（可为空，表示无单位占用）
 *   - 以纯色矩形 + 名字文本 + 血条 + 蓝条的形式显示单位
 *   - 提供棋盘坐标（行/列）与像素坐标之间的静态转换方法
 *   - 支持鼠标拖拽：按住左键拖动，松手自动吸附到最近格子；
 *     若目标格被占用则弹回原位
 *
 * 血条：绿色表示 HP 比例，底色为暗红
 * 蓝条：蓝色表示 Mana 比例，底色为深蓝
 *
 * 用法示例：
 * @code
 *   Unit *u = new Unit("战士", Owner::PlayerCtrl, QPoint(3, 0), 300, 30, 1, 60);
 *   UnitItem *item = new UnitItem(u);
 *   board.placeUnitItem(item, 7, 3);  // 放到玩家半场
 * @endcode
 */
// ============================================================================
// DragSource — 记录拖拽起始位置类型（棋盘 or 备战区）
// ============================================================================

/**
 * @brief DragSource — 区分 UnitItem 当前所在的区域
 *
 * 用于拖拽松手时判断源位置，决定如何清理源占用：
 *   - Board → Bench：释放棋盘格子 + 加入备战区
 *   - Bench → Board：从备战区移除 + 占据棋盘格子
 *   - Board → Board：原有棋盘内拖拽逻辑
 *   - Bench → Bench：备战区内交换槽位
 */
enum class DragSource {
    Board,  ///< 单位当前在棋盘上
    Bench   ///< 单位当前在备战区
};

class UnitItem : public QGraphicsRectItem {
public:
    // ========== 构造/析构 ==========

    /**
     * @brief 构造一个单位图元
     * @param unit 绑定的逻辑单位指针（可为 nullptr，后续通过 bindUnit() 绑定）
     * @param parent 父图元（Qt 场景树管理，默认无父节点）
     */
    explicit UnitItem(Unit *unit = nullptr, QGraphicsItem *parent = nullptr);
    ~UnitItem() override = default;

    // ========== 单位绑定 ==========

    /// 返回当前绑定的逻辑单位指针
    Unit *unit() const { return m_unit; }

    /**
     * @brief 绑定/更换逻辑单位
     *
     * 绑定后立即刷新显示（血条、蓝条、名称）。
     * 传入 nullptr 可解除绑定（图元仍存在但显示为空状态）。
     */
    void bindUnit(Unit *unit);

    // ========== 与棋盘的关联 ==========

    /**
     * @brief 设置所属棋盘
     *
     * 当 UnitItem 被放入某个 BoardWidget 时调用，
     * 用于拖拽过程中查询格子占用状态。
     */
    void setBoard(BoardWidget *board) { m_board = board; }

    /// 返回所属棋盘指针
    BoardWidget *board() const { return m_board; }

    // ========== 与备战区的关联 ==========

    /**
     * @brief 设置所属备战区控件
     *
     * 当 UnitItem 需要感知备战区落点时调用（通常由 Game 或 main.cpp 在初始化时设置）。
     * 拖拽松手时用于判断鼠标是否落在备战区范围内。
     */
    void setBenchWidget(BenchWidget *bw) { m_benchWidget = bw; }

    /// 返回所属备战区指针
    BenchWidget *benchWidget() const { return m_benchWidget; }

    /// 设置出售区域控件引用
    void setSellZone(SellZone *sz) { m_sellZone = sz; }

    // ========== 与 Game 的关联（升星/装备交互） ==========

    void setGame(Game *game) { m_game = game; }
    Game *game() const { return m_game; }

    // ========== 拖拽源位置管理 ==========

    /**
     * @brief 设置当前所在的区域类型及位置
     *
     * 在以下时机调用：
     *   - 通过 BoardWidget::placeUnitItem() 放置到棋盘后 → setDragSource(DragSource::Board)
     *   - 通过 BenchWidget::setUnitItem() 放入备战区后 → setDragSource(DragSource::Bench, slot)
     *
     * @param src  当前所在区域
     * @param slot 若 src == Bench，记录所在的槽位索引（用于弹回）
     */
    void setDragSource(DragSource src, int slot = -1);

    /// 返回当前所在的区域类型
    DragSource dragSource() const { return m_dragSource; }

    /// 返回在备战区的槽位索引（仅在 dragSource() == Bench 时有效）
    int benchSlot() const { return m_benchSlot; }

    // ========== 坐标转换（静态方法） ==========

    /**
     * @brief 棋盘坐标 → 像素坐标
     *
     * 棋盘坐标系：row 向下增长，col 向右增长
     * 像素坐标系：x 向右增长，y 向下增长
     *
     * @param row 棋盘行号 [0, BOARD_ROWS)
     * @param col 棋盘列号 [0, BOARD_COLS)
     * @return 该格左上角在场景中的像素坐标
     */
    static QPointF boardToPixel(int row, int col);

    /**
     * @brief 棋盘坐标（QPoint）→ 像素坐标
     * @param boardPos 棋盘坐标，x=列, y=行
     */
    static QPointF boardToPixel(const QPoint &boardPos);

    /**
     * @brief 像素坐标 → 棋盘坐标
     *
     * 将场景像素坐标转换为对应的棋盘格子坐标（向下取整）。
     * 超出棋盘范围的像素会被钳制到有效范围。
     *
     * @param pixelPos 场景像素坐标
     * @return 对应的棋盘坐标（QPoint: x=列, y=行）
     */
    static QPoint pixelToBoard(const QPointF &pixelPos);

    // ========== 棋盘定位 ==========

    /**
     * @brief 将图元放置在指定棋盘格
     *
     * 内部调用 boardToPixel() 转换坐标后设置场景位置。
     * 同时更新绑定的 Unit 的 position 字段。
     */
    void setBoardPos(int row, int col);

    /// 获取当前棋盘行号
    int boardRow() const { return m_boardRow; }
    /// 获取当前棋盘列号
    int boardCol() const { return m_boardCol; }

    // ========== 外观刷新 ==========

    /// 根据绑定 Unit 的当前状态刷新血条、蓝条、颜色
    void refreshAppearance();

    /**
     * @brief 从 Unit 位置同步视觉坐标
     *
     * 战斗中 Unit 通过寻路系统移动后，棋盘坐标写入 Unit::m_position，
     * 此方法将 UnitItem 的像素位置同步到 Unit 的当前坐标。
     *
     * 与 setBoardPos() 的区别：
     *   setBoardPos() 是"双向同步"（Item → Unit），用于玩家拖拽；
     *   syncVisualPosition() 是"单向同步"（Unit → Item），用于战斗自动移动。
     */
    void syncVisualPosition();

    // ========== 动画 ==========

    /**
     * @brief 触发攻击闪光动画
     *
     * 在 paint() 中将单位底色短暂变白，持续 FLASH_DURATION 帧后恢复。
     * 由 Game::update() 在检测到 Unit::justAttacked() 时调用。
     */
    void triggerAttackFlash();

    /**
     * @brief 触发升星合成闪光动画
     */
    void triggerMergeFlash();

    /**
     * @brief 触发装备闪光动画
     */
    void triggerEquipFlash();

    // ========== 常量 ==========

    /// 单位图元边长（像素），与棋盘格子大小一致
    static constexpr int ITEM_SIZE = CELL_SIZE_PX;

    /// 血条高度（像素）
    static constexpr int HP_BAR_HEIGHT = 6;
    /// 蓝条高度（像素）
    static constexpr int MANA_BAR_HEIGHT = 4;
    /// 血条/蓝条距离底部的边距
    static constexpr int BAR_BOTTOM_MARGIN = 2;

    /// 我方单位底色
    static const QColor PLAYER_COLOR;
    /// 敌方单位底色
    static const QColor ENEMY_COLOR;
    /// 未绑定单位的默认底色
    static const QColor NEUTRAL_COLOR;

    // ========== 全局拖拽锁定（战斗阶段禁用拖拽） ==========

    /**
     * @brief 设置全局拖拽锁定状态
     *
     * 战斗阶段应锁定拖拽，防止玩家在战斗中调整站位。
     * 由 Game 在 phase 切换时调用：
     *   - Preparation → Combat:  setDragLock(true)
     *   - Combat → Preparation:  setDragLock(false)
     */
    static void setDragLock(bool locked) { s_dragLocked = locked; }

    /// 当前是否处于拖拽锁定状态
    static bool isDragLocked() { return s_dragLocked; }

protected:
    // ========== QGraphicsRectItem 重写 ==========

    /**
     * @brief 自定义绘制
     *
     * 绘制顺序：底色矩形 → 名字文本 → 血条背景 → 血条填充 → 蓝条背景 → 蓝条填充
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

    // ========== 鼠标事件（拖拽实现） ==========

    /**
     * @brief 鼠标按下：记录拖拽起始位置，准备拖拽
     */
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    /**
     * @brief 鼠标移动：跟随鼠标移动图元
     */
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    /**
     * @brief 鼠标释放：吸附到最近格子，检查占用，决定最终位置
     */
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    // ---- 装备拖放 ----
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
    void dropEvent(QGraphicsSceneDragDropEvent *event) override;

private:
    // ---- 数据 ----
    Unit *m_unit = nullptr;        ///< 绑定的逻辑单位（不拥有所有权）
    BoardWidget *m_board = nullptr;///< 所属棋盘（用于查询格子占用）
    BenchWidget *m_benchWidget = nullptr; ///< 所属备战区（用于检测落点）
    SellZone    *m_sellZone    = nullptr; ///< 出售区域（用于拖拽出售）
    Game *m_game = nullptr;       ///< 所属 Game（用于升星合并/装备交互）
    int m_boardRow = 0;            ///< 当前棋盘行号
    int m_boardCol = 0;            ///< 当前棋盘列号

    // ---- 拖拽源位置 ----
    DragSource m_dragSource = DragSource::Board; ///< 当前所在区域
    int m_benchSlot = -1;          ///< 在备战区的槽位索引（-1 表示不在备战区）

    /// 全局拖拽锁定标志（static，所有 UnitItem 共享）
    static bool s_dragLocked;

    // ---- 拖拽状态 ----
    bool m_dragging = false;
    QLabel *m_dragOverlay = nullptr;  ///< 拖拽时的屏幕层浮动预览
    DragSource m_dragOrigSource = DragSource::Board;
    int m_dragOrigRow = 0;         ///< 拖拽开始时的棋盘行号（用于弹回）
    int m_dragOrigCol = 0;         ///< 拖拽开始时的棋盘列号（用于弹回）
    int m_dragOrigSlot = -1;       ///< 拖拽开始时的备战区槽位（-1 表示不在备战区）

    // ---- 动画状态 ----
    int m_attackFlashCounter = 0;   ///< 攻击闪光倒计时（帧数），>0 时绘制闪白效果
    int m_mergeFlashCounter = 0;    ///< 升星合成闪光倒计时
    int m_equipFlashCounter = 0;    ///< 装备闪光倒计时
    int m_lastBoardRow = 0;         ///< 上一帧的棋盘行号（用于检测位置变化）
    int m_lastBoardCol = 0;         ///< 上一帧的棋盘列号（用于检测位置变化）

    /// 攻击闪光持续帧数（约 0.1 秒 @ 60fps = 6 帧）
    static constexpr int FLASH_DURATION = 6;

    // ---- 内部辅助 ----

    /// 根据 owner 返回对应的底色
    QColor baseColor() const;

    // ---- 跨控件拖拽辅助 ----

    /// 处理松手时落在备战区的情况
    void handleDropOnBench(const QPointF &screenPt);

    /// 处理松手时落在棋盘的情况
    void handleDropOnBoard(const QPointF &screenPt);

    /// 弹回拖拽前的原始位置（棋盘格或备战区槽位）
    void bounceBackToSource();
};

#endif // UNITITEM_H
