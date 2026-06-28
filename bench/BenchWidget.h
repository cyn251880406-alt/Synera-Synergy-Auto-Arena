#ifndef BENCHWIDGET_H
#define BENCHWIDGET_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>

class Bench;
class UnitItem;

// ============================================================================
// BenchWidget — 备战区可视化控件
// ============================================================================

/**
 * @brief BenchWidget — 备战区的 Qt 视图层
 *
 * 使用 QGraphicsView/QGraphicsScene 将逻辑层 Bench 的 8 个槽位
 * 渲染为一行水平排列的格子。职责：
 *   - 绘制 8 个备战区格子（64×64 像素）
 *   - 在格子中显示 UnitItem（单位图元）
 *   - 提供槽位 ↔ 像素坐标转换，供拖拽落位使用
 *   - 通过 refreshDisplay() 与 Bench 逻辑层同步
 *
 * 设计原则：
 *   - 纯视图层，不参与游戏逻辑（不操作 Unit、不管理属性）
 *   - 通过 Bench* 读取逻辑层的单位数据
 *   - 不拥有 UnitItem 的所有权（不负责 delete）
 *
 * 用法示例：
 * @code
 *   Bench bench;
 *   BenchWidget benchWidget;
 *   benchWidget.setBench(&bench);
 *
 *   UnitItem *item = new UnitItem(unit);
 *   benchWidget.setUnitItem(0, item);   // 放到槽位 0
 *   benchWidget.refreshDisplay();       // 同步 Bench → 显示
 * @endcode
 */
class BenchWidget : public QGraphicsView
{
    Q_OBJECT

public:
    // ========== 常量 ==========

    /// 备战区槽位数（与 Bench::CAPACITY 保持一致）
    static constexpr int SLOT_COUNT = 8;

    /// 每个槽位的像素边长
    static constexpr int SLOT_SIZE = 64;

    /// 槽位之间的间距（像素）
    static constexpr int SLOT_SPACING = 2;

    /// 备战区总宽度（8 * 64 + 7 * 2 = 526）
    static constexpr int SCENE_WIDTH  = SLOT_COUNT * SLOT_SIZE
                                       + (SLOT_COUNT - 1) * SLOT_SPACING;
    /// 备战区场景高度
    static constexpr int SCENE_HEIGHT = SLOT_SIZE + 2;

    /// 空槽位底色
    static const QColor EMPTY_SLOT_COLOR;
    /// 空槽位边框色
    static const QColor EMPTY_SLOT_BORDER;

    // ========== 构造/析构 ==========

    explicit BenchWidget(QWidget *parent = nullptr);
    ~BenchWidget() override = default;

    // ========== 与 Bench 逻辑层绑定 ==========

    /**
     * @brief 绑定逻辑层 Bench 对象
     *
     * BenchWidget 通过此指针读取槽位中的 Unit*，
     * refreshDisplay() 依赖此绑定才能工作。
     */
    void setBench(Bench *bench);

    /// 返回当前绑定的 Bench 指针
    Bench *bench() const { return m_bench; }

    // ========== UnitItem 管理 ==========

    /**
     * @brief 将一个 UnitItem 显示在指定槽位上
     *
     * 若该槽位已有单位，旧单位会被 removeItem() 移出场景（不 delete）。
     *
     * @param slot  槽位索引 [0, SLOT_COUNT)
     * @param item  要显示的单位图元（不能为 nullptr）
     * @return true 设置成功；false 索引越界或 item 为空
     */
    bool setUnitItem(int slot, UnitItem *item);

    /**
     * @brief 从指定槽位移除 UnitItem（不移除 Bench 数据）
     *
     * 将 UnitItem 从场景中移出但不 delete。
     * 调用方负责将 UnitItem 转移到棋盘或其他地方。
     *
     * @param slot 槽位索引 [0, SLOT_COUNT)
     * @return 被移除的 UnitItem 指针；槽位空则返回 nullptr
     */
    UnitItem *takeUnitItem(int slot);

    /// 返回指定槽位上的 UnitItem 指针（只读）
    UnitItem *unitItemAt(int slot) const;

    // ========== 槽位查询 ==========

    /// 指定槽位是否已被 UnitItem 占用
    bool isSlotOccupied(int slot) const;

    /// 返回第一个空槽位的索引 [0, 7]，全部占用则返回 -1
    int firstEmptySlot() const;

    // ========== 坐标转换（静态方法） ==========

    /**
     * @brief 槽位索引 → 像素坐标
     * @param slot 槽位索引 [0, SLOT_COUNT)
     * @return 该槽位左上角在场景中的像素坐标
     */
    static QPointF slotToPixel(int slot);

    /**
     * @brief 像素坐标 → 槽位索引
     *
     * 根据像素 x 坐标判断落在哪个槽位范围内。
     * 超出区域左侧返回 0，右侧返回 SLOT_COUNT - 1。
     *
     * @param pixelPos 场景像素坐标
     * @return 槽位索引 [0, SLOT_COUNT)
     */
    static int pixelToSlot(const QPointF &pixelPos);

    // ========== 外观刷新 ==========

    /**
     * @brief 根据绑定的 Bench 刷新显示
     *
     * 遍历 Bench 的 8 个槽位：
     *   - 若 Bench 槽位有 Unit 但视觉槽位空 → 仅更新槽位高亮
     *   - 若 Bench 槽位空但视觉槽位有 UnitItem → 仅更新槽位高亮
     *   - 重新定位所有 UnitItem 到正确的像素坐标
     *
     * 通常在 Bench 数据变更后调用（addUnit/removeUnit 之后）。
     */
    void refreshDisplay();

    /// 返回指定槽位的格子指针（供外观定制用）
    QGraphicsRectItem *slotItem(int index) const;

    // ========== 拖拽高亮 ==========

    /**
     * @brief 高亮指定槽位（拖拽经过时视觉反馈）
     *
     * @param slot  槽位索引 [0, SLOT_COUNT)
     * @param valid true = 可放置（绿色高亮），false = 不可放置（红色高亮）
     */
    void setHighlightSlot(int slot, bool valid);

    /// 清除所有槽位的高亮效果
    void clearHighlight();

    // ========== 跨控件拖拽支持 ==========

    /**
     * @brief 判断屏幕坐标是否落在本控件范围内
     *
     * 用于 UnitItem 在 mouseReleaseEvent 中判断松手位置。
     * 使用全局屏幕坐标，与 item 所属的场景无关。
     */
    bool isPointOverWidget(const QPointF &screenPt) const;

    /**
     * @brief 屏幕坐标 → 备战区槽位索引
     *
     * 内部通过 mapFromGlobal → mapToScene → pixelToSlot 链路转换。
     * @param screenPt 全局屏幕坐标
     * @return 槽位索引 [0, 7]；无法映射则返回 -1
     */
    int screenToSlot(const QPointF &screenPt) const;

    /**
     * @brief 接收一个来自棋盘的 UnitItem
     *
     * 完成：加入场景 → 定位到槽位 → 注册到 m_unitItems →
     *       更新槽位颜色 → 同步 Bench 逻辑层
     *
     * @param item 要接收的单位图元（来自棋盘场景）
     * @param slot 目标槽位索引
     */
    void acceptUnitItem(UnitItem *item, int slot);

    /**
     * @brief 释放一个 UnitItem（移出到棋盘）
     *
     * 完成：从场景移出 → 清除 m_unitItems 注册 →
     *       恢复槽位颜色 → 同步 Bench 逻辑层移除
     *
     * @param slot 要释放的槽位索引
     * @return 被释放的 UnitItem 指针
     */
    UnitItem *releaseUnitItem(int slot);

    /**
     * @brief 备战区内两个槽位的交换或移动
     *
     * 若 fromSlot 有单位、toSlot 为空 → 移动
     * 若两个槽位都有单位       → 交换
     * 若 fromSlot 为空         → 无操作
     *
     * 同时更新两个 UnitItem 的 dragSource 状态。
     */
    void swapOrMoveUnitItem(int slotA, int slotB);

    void resizeEvent(QResizeEvent *event) override;

private:
    void initScene();   // 创建场景与 8 个槽位格子
    void initView();    // 设置视图属性

    // ---- 数据 ----
    Bench   *m_bench = nullptr;          ///< 逻辑层 Bench（不拥有所有权）
    QGraphicsScene *m_scene = nullptr;   ///< 场景
    QGraphicsRectItem *m_slots[SLOT_COUNT] = {}; ///< 8 个槽位格子

    /// 各槽位原始底色（拖拽高亮时临时替换，恢复时使用）
    QColor m_slotBaseColors[SLOT_COUNT];

    // 视觉占用表：m_unitItems[slot] 指向该槽位上的 UnitItem，nullptr 表示空
    UnitItem *m_unitItems[SLOT_COUNT] = {};

    // ---- 拖拽高亮状态 ----
    int  m_highlightedSlot = -1;         ///< 当前高亮的槽位索引，-1 表示无高亮

    /// 可放置高亮色（绿色调）
    static const QColor VALID_HIGHLIGHT_COLOR;
    /// 不可放置高亮色（红色调）
    static const QColor INVALID_HIGHLIGHT_COLOR;

    // ---- 内部辅助 ----

    /// 检查槽位索引是否有效
    bool isValidSlot(int slot) const;

    /// 设置槽位显示颜色并记录为底色（供拖拽高亮恢复使用）
    void setSlotBaseColor(int slot, const QColor &color);
};

#endif // BENCHWIDGET_H
