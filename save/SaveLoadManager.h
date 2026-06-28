#ifndef SAVELOADMANAGER_H
#define SAVELOADMANAGER_H

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPoint>

#include "unit/Unit.h"  // Trait, Owner 枚举

class Player;
class Bench;
class Shop;

// ============================================================================
// SaveData — 存档数据的结构化表示
// ============================================================================

/**
 * @brief SaveData — 保存/读取游戏状态所需的所有数据
 *
 * 由 SaveLoadManager 序列化为 JSON。敌方单位不计入存档，读档后重新生成。
 */
struct SaveData {
    /// 存档格式版本（用于未来兼容性检查）
    static constexpr int CURRENT_VERSION = 1;

    int saveVersion = CURRENT_VERSION;  ///< 存档版本号
    int round       = 1;               ///< 当前轮次

    // ---- 玩家状态 ----
    int playerHp        = 100;
    int playerMaxHp     = 100;
    int playerGold      = 10;
    int playerLevel     = 1;
    int playerExp       = 0;
    int playerPopCap    = 4;
    int winStreak       = 0;
    int lossStreak      = 0;

    /**
     * @brief UnitInfo — 一个单位的完整存档信息
     *
     * 包含重建一个 Unit 及其装备、羁绊、位置所需的全部字段。
     * 不包括当前帧的瞬时状态（如移动路径、攻击冷却），
     * 因为这些会在准备阶段自动重置为初始值。
     */
    struct UnitInfo {
        QString name;          ///< 单位名称
        int     owner      = 0;///< 0=PlayerCtrl, 1=EnemyCtrl
        int     hp         = 100;
        int     maxHp      = 100;
        int     atk        = 10;
        int     range      = 1;
        int     maxMana    = 50;
        int     attackSpeed= 60;
        int     starLevel  = 1;
        int     boardCol   = -1; ///< 棋盘列（-1 表示在备战区）
        int     boardRow   = -1; ///< 棋盘行（-1 表示在备战区）
        QStringList traits;      ///< 羁绊标签名称列表（如 ["Warrior"]）
        QString     equipment;   ///< 装备类型名称（空串表示无装备）
    };

    /// 所有我方 Unit 的存档信息
    QVector<UnitInfo> units;

    // ---- 商店 ----
    /// 5 个槽位的英雄名称（空串 = 该槽无英雄）
    QStringList shopSlots;
};

// ============================================================================
// SaveLoadManager — 存档/读档管理器（逻辑层）
// ============================================================================

/**
 * @brief SaveLoadManager — 游戏状态的 JSON 序列化与反序列化
 *
 * 全部为静态方法。使用 QJsonDocument 将游戏状态序列化为 JSON 文件。
 * saveGame() 接受 SaveData，loadGame() 返回 SaveData，与 Game 层解耦。
 */
class SaveLoadManager {
public:
    // ========== 构造 ==========

    SaveLoadManager() = delete;  // 全部为静态方法，无需实例化

    // ========== 存档 / 读档 ==========

    /**
     * @brief 将游戏状态保存为 JSON 文件
     *
     * 写入流程：
     *   1. 调用 SaveData → QJsonObject（serialize）
     *   2. 将 QJsonObject 包装为 QJsonDocument
     *   3. 写入文件（UTF-8 编码，缩进格式）
     *
     * @param filename 保存路径（如 "save.json"）
     * @param data     要保存的游戏状态
     * @return true 保存成功；false 文件打开或写入失败
     */
    static bool saveGame(const QString &filename, const SaveData &data);

    /**
     * @brief 从 JSON 文件读取游戏状态
     *
     * 读取流程：
     *   1. 读取文件内容
     *   2. 解析为 QJsonDocument
     *   3. 校验 saveVersion
     *   4. 反序列化为 SaveData 结构体
     *
     * @param filename 存档路径（如 "save.json"）
     * @return 解析后的 SaveData；若解析失败 data.valid = false
     */
    static SaveData loadGame(const QString &filename);

    // ========== 序列化（SaveData ↔ QJsonObject） ==========

    /// 将 SaveData 序列化为 QJsonObject
    static QJsonObject serialize(const SaveData &data);

    /// 从 QJsonObject 反序列化为 SaveData
    static SaveData deserialize(const QJsonObject &json);

    // ========== Trait ↔ 字符串 转换（公开给 Game 层使用） ==========

    /// 将 Trait 枚举值转为字符串（如 Trait::Warrior → "Warrior"）
    static QString traitToString(Trait t);

    /// 将字符串转为 Trait 枚举值（如 "Warrior" → Trait::Warrior）
    static Trait stringToTrait(const QString &str);

private:
    // ========== 内部序列化辅助 ==========

    /// 序列化一个 UnitInfo 为 QJsonObject
    static QJsonObject serializeUnit(const SaveData::UnitInfo &unit);

    /// 从 QJsonObject 反序列化为 UnitInfo
    static SaveData::UnitInfo deserializeUnit(const QJsonObject &obj);
};

#endif // SAVELOADMANAGER_H
