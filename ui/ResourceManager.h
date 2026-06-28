#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QString>
#include <QPixmap>
#include <QMap>
#include <QDir>

// ============================================================================
// ResourceManager — 游戏素材加载与缓存
// ============================================================================

/**
 * @brief ResourceManager — 单例，负责加载和管理所有外部图像素材
 *
 * 素材目录结构（相对于可执行文件或项目根目录）：
 *   assets/
 *     units/       — 单位/英雄图标 (64×64 PNG)
 *     board/       — 棋盘格子纹理
 *     ui/          — UI 面板/按钮背景
 *     equipment/   — 装备图标
 *     effects/     — 技能特效帧
 *
 * 用法：
 * @code
 *   // 初始化（在 main 中调用一次）
 *   ResourceManager::instance().init("C:/path/to/gamehomework");
 *
 *   // 获取素材
 *   QPixmap px = ResourceManager::instance().unitIcon("战士");
 *   if (!px.isNull()) { painter->drawPixmap(...); }
 * @endcode
 *
 * 所有 getter 在素材缺失时返回空的 QPixmap（isNull() == true），
 * 调用方应回退到程序化绘制。
 */
class ResourceManager
{
public:
    // ========== 单例 ==========
    static ResourceManager &instance();

    // ========== 初始化 ==========

    /**
     * @brief 设置素材根目录并预加载常用素材
     * @param projectRoot 项目根目录路径（包含 assets/ 子目录）
     *
     * 若传入空字符串，则自动探测：
     *   1. QCoreApplication::applicationDirPath() + "/../"（开发环境）
     *   2. QCoreApplication::applicationDirPath()（部署环境）
     */
    void init(const QString &projectRoot = QString());

    /// 素材系统是否已初始化
    bool isReady() const { return m_ready; }

    // ========== 单位图标 ==========

    /**
     * @brief 获取单位图标（64×64）
     * @param unitName 单位名称（如"神射手"、"鹰眼"）
     * @return 对应图标；未找到返回空 QPixmap
     *
     * 文件命名约定：assets/units/<英文名>_bench.png（棋盘/备战区用）
     * 若 _bench 版本不存在，回退到 <英文名>.png
     */
    QPixmap unitIcon(const QString &unitName) const;

    /**
     * @brief 获取单位在商店中展示的图标
     *
     * 文件命名约定：assets/units/<英文名>_shop.png
     * 若 _shop 版本不存在，回退到 unitIcon()（即 _bench 或纯名版本）
     */
    QPixmap unitIconShop(const QString &unitName) const;

    /// 获取默认我方/敌方占位图标
    QPixmap defaultPlayerIcon() const;
    QPixmap defaultEnemyIcon() const;

    // ========== 棋盘素材 ==========

    /// 棋盘整体背景图
    QPixmap boardBg() const     { return m_boardBg; }
    /// 浅色格纹理
    QPixmap lightTile() const   { return m_lightTile; }
    /// 深色格纹理
    QPixmap darkTile() const    { return m_darkTile; }
    /// 备战区格纹理
    QPixmap benchSlotBg() const { return m_benchSlotBg; }

    // ========== 装备图标 ==========

    /// 装备图标（32×32）
    QPixmap equipmentIcon(const QString &equipTypeName) const;

    // ========== UI 素材 ==========

    /// 商店卡片背景
    QPixmap shopCardBg() const      { return m_shopCardBg; }
    /// 按钮常态
    QPixmap btnNormal() const       { return m_btnNormal; }
    /// 按钮悬停
    QPixmap btnHover() const        { return m_btnHover; }
    /// 面板背景
    QPixmap panelBg() const         { return m_panelBg; }

    // ========== 特效素材 ==========

    /// 技能特效帧（可选扩展）
    QPixmap effectFrame(const QString &effectName, int frame) const;

    /// 技能特效图（根据单位名称获取对应技能特效）
    QPixmap skillEffectImage(const QString &unitName) const;

    // ========== UI 结算画面 ==========

    /// 胜利结算画面
    QPixmap victoryImage() const  { return m_victoryImage; }
    /// 失败结算画面
    QPixmap defeatImage() const   { return m_defeatImage; }

    // ========== 路径查询 ==========
    QString assetsRoot() const { return m_assetsRoot.absolutePath(); }

private:
    ResourceManager() = default;
    ~ResourceManager() = default;
    ResourceManager(const ResourceManager &) = delete;
    ResourceManager &operator=(const ResourceManager &) = delete;

    // ---- 内部辅助 ----
    QPixmap loadPixmap(const QString &relativePath) const;
    void preloadAll();

    /// 中文单位名 → 文件名映射
    static QString nameToFile(const QString &unitName);

    // ---- 数据 ----
    QDir m_assetsRoot;
    bool m_ready = false;

    // 棋盘素材缓存
    QPixmap m_boardBg;
    QPixmap m_lightTile;
    QPixmap m_darkTile;
    QPixmap m_benchSlotBg;

    // UI 素材缓存
    QPixmap m_shopCardBg;
    QPixmap m_btnNormal;
    QPixmap m_btnHover;
    QPixmap m_panelBg;
    QPixmap m_victoryImage;
    QPixmap m_defeatImage;

    // 常用单位图标缓存（懒加载）
    mutable QMap<QString, QPixmap> m_unitIconCache;
    mutable QMap<QString, QPixmap> m_unitIconShopCache;
    mutable QMap<QString, QPixmap> m_equipIconCache;
    mutable QMap<QString, QPixmap> m_skillEffectCache;
};

#endif // RESOURCEMANAGER_H
