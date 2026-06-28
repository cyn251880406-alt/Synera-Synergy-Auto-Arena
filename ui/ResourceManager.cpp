#include "ui/ResourceManager.h"

#include <QCoreApplication>
#include <QDir>

// ============================================================================
// 单例
// ============================================================================

ResourceManager &ResourceManager::instance()
{
    static ResourceManager mgr;
    return mgr;
}

// ============================================================================
// 初始化
// ============================================================================

void ResourceManager::init(const QString &projectRoot)
{
    QString root = projectRoot;

    if (root.isEmpty()) {
        // 自动探测：先尝试开发环境（exe 在 build/ 下，assets/ 在项目根）
        QDir exeDir(QCoreApplication::applicationDirPath());
        QDir devRoot(exeDir.absoluteFilePath(QStringLiteral("..")));
        if (devRoot.exists(QStringLiteral("assets"))) {
            root = devRoot.absolutePath();
        } else {
            // 部署环境：assets/ 与 exe 同目录
            root = exeDir.absolutePath();
        }
    }

    m_assetsRoot = QDir(root);
    if (!m_assetsRoot.exists(QStringLiteral("assets"))) {
        m_ready = false;
        return;
    }

    m_ready = true;
    preloadAll();
}

void ResourceManager::preloadAll()
{
    m_boardBg     = loadPixmap(QStringLiteral("assets/board/board.png"));
    m_lightTile   = loadPixmap(QStringLiteral("assets/board/light.png"));
    m_darkTile    = loadPixmap(QStringLiteral("assets/board/dark.png"));
    m_benchSlotBg = loadPixmap(QStringLiteral("assets/board/bench_slot.png"));

    m_shopCardBg  = loadPixmap(QStringLiteral("assets/ui/shop_card.png"));
    m_btnNormal   = loadPixmap(QStringLiteral("assets/ui/btn_normal.png"));
    m_btnHover    = loadPixmap(QStringLiteral("assets/ui/btn_hover.png"));
    m_panelBg     = loadPixmap(QStringLiteral("assets/ui/panel_bg.png"));

    m_victoryImage = loadPixmap(QStringLiteral("assets/ui/victory.png"));
    m_defeatImage  = loadPixmap(QStringLiteral("assets/ui/defeat.png"));
}

// ============================================================================
// 素材加载
// ============================================================================

QPixmap ResourceManager::loadPixmap(const QString &relativePath) const
{
    if (!m_ready)
        return QPixmap();

    QString fullPath = m_assetsRoot.absoluteFilePath(relativePath);
    if (!QFile::exists(fullPath))
        return QPixmap();

    QPixmap px;
    if (px.load(fullPath))
        return px;
    return QPixmap();
}

// ============================================================================
// 单位图标
// ============================================================================

QString ResourceManager::nameToFile(const QString &unitName)
{
    // 中文名 → 文件名映射
    if (unitName == QStringLiteral("神射手")) return QStringLiteral("sheshou1_xilian");
    if (unitName == QStringLiteral("鹰眼"))   return QStringLiteral("sheshou2_xiaogong");
    if (unitName == QStringLiteral("重装兵")) return QStringLiteral("tanke1_zhongli");
    if (unitName == QStringLiteral("守护者")) return QStringLiteral("tanke2_fuxuan");
    if (unitName == QStringLiteral("剑术师")) return QStringLiteral("zhanshi1_yasuo");
    if (unitName == QStringLiteral("狂战士")) return QStringLiteral("zhanshi2_ying");

    // 敌方单位
    if (unitName == QStringLiteral("敌射手")) return QStringLiteral("enemy_archer");
    if (unitName == QStringLiteral("敌坦克")) return QStringLiteral("enemy_tank");
    if (unitName == QStringLiteral("敌战士")) return QStringLiteral("enemy_warrior");

    return unitName.toLower(); // 兜底：全小写
}

QPixmap ResourceManager::unitIcon(const QString &unitName) const
{
    if (!m_ready)
        return QPixmap();

    // 检查缓存
    auto it = m_unitIconCache.constFind(unitName);
    if (it != m_unitIconCache.constEnd())
        return it.value();

    // 先尝试 _bench 版本（棋盘/备战区用）
    QString baseName = nameToFile(unitName);
    QPixmap px = loadPixmap(
        QStringLiteral("assets/units/") + baseName + QStringLiteral("_bench.png"));

    // 回退：不带后缀的版本
    if (px.isNull())
        px = loadPixmap(QStringLiteral("assets/units/") + baseName + QStringLiteral(".png"));

    m_unitIconCache.insert(unitName, px);
    return px;
}

QPixmap ResourceManager::unitIconShop(const QString &unitName) const
{
    if (!m_ready)
        return QPixmap();

    // 检查缓存
    auto it = m_unitIconShopCache.constFind(unitName);
    if (it != m_unitIconShopCache.constEnd())
        return it.value();

    // 先尝试 _shop 版本
    QString baseName = nameToFile(unitName);
    QPixmap px = loadPixmap(
        QStringLiteral("assets/units/") + baseName + QStringLiteral("_shop.png"));

    // 回退：使用 board 版本（unitIcon）
    if (px.isNull())
        px = unitIcon(unitName);

    m_unitIconShopCache.insert(unitName, px);
    return px;
}

QPixmap ResourceManager::defaultPlayerIcon() const
{
    return loadPixmap(QStringLiteral("assets/units/player_default.png"));
}

QPixmap ResourceManager::defaultEnemyIcon() const
{
    return loadPixmap(QStringLiteral("assets/units/enemy_default.png"));
}

// ============================================================================
// 装备图标
// ============================================================================

QPixmap ResourceManager::equipmentIcon(const QString &equipTypeName) const
{
    if (!m_ready)
        return QPixmap();

    auto it = m_equipIconCache.constFind(equipTypeName);
    if (it != m_equipIconCache.constEnd())
        return it.value();

    // Sword → sword, Armor → armor, Glove → glove, Crystal → crystal
    QString fileName = equipTypeName.toLower();
    QPixmap px = loadPixmap(QStringLiteral("assets/equipment/") + fileName + QStringLiteral(".png"));

    m_equipIconCache.insert(equipTypeName, px);
    return px;
}

// ============================================================================
// 特效
// ============================================================================

QPixmap ResourceManager::effectFrame(const QString &effectName, int frame) const
{
    // 命名约定：assets/effects/<name>_<frame>.png
    QString fileName = effectName + QStringLiteral("_") + QString::number(frame) + QStringLiteral(".png");
    return loadPixmap(QStringLiteral("assets/effects/") + fileName);
}

QPixmap ResourceManager::skillEffectImage(const QString &unitName) const
{
    if (!m_ready)
        return QPixmap();

    auto it = m_skillEffectCache.constFind(unitName);
    if (it != m_skillEffectCache.constEnd())
        return it.value();

    QString baseName = nameToFile(unitName);
    // 先尝试清理版（去白底裁剪后）：如 sheshou1_xilian_effect_clean.png
    QPixmap px = loadPixmap(
        QStringLiteral("assets/effects/") + baseName + QStringLiteral("_effect_clean.png"));

    // 回退：未处理的完整名称：如 sheshou1_xilian_effect.png
    if (px.isNull())
        px = loadPixmap(
            QStringLiteral("assets/effects/") + baseName + QStringLiteral("_effect.png"));

    // 回退：去掉后缀部分（如 _xilian → sheshou1_effect_clean.png）
    if (px.isNull()) {
        int underscoreIdx = baseName.lastIndexOf(QChar('_'));
        if (underscoreIdx > 0) {
            QString shortName = baseName.left(underscoreIdx);
            px = loadPixmap(
                QStringLiteral("assets/effects/") + shortName + QStringLiteral("_effect_clean.png"));
            if (px.isNull())
                px = loadPixmap(
                    QStringLiteral("assets/effects/") + shortName + QStringLiteral("_effect.png"));
        }
    }

    m_skillEffectCache.insert(unitName, px);
    return px;
}
