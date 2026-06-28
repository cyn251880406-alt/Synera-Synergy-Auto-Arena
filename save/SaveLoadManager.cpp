#include "save/SaveLoadManager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <new>
#include <exception>

// ============================================================================
// 存档 — SaveData → JSON 文件
// ============================================================================

bool SaveLoadManager::saveGame(const QString &filename, const SaveData &data)
{
    try {
        QFile file(filename);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }

        QJsonObject root = serialize(data);
        QJsonDocument doc(root);
        qint64 bytes = file.write(doc.toJson(QJsonDocument::Indented));
        file.close();

        if (bytes == -1)
            return false;
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    } catch (const std::exception &) {
        return false;
    }
}

// ============================================================================
// 读档 — JSON 文件 → SaveData
// ============================================================================

SaveData SaveLoadManager::loadGame(const QString &filename)
{
    try {
        SaveData data;
        QFile file(filename);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return data;
        }

        // 防止加载异常大的文件导致内存耗尽
        if (file.size() > 10 * 1024 * 1024) {  // 10 MB 上限
            return data;
        }

        QByteArray raw = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            return data;
        }
        if (!doc.isObject()) {
            return data;
        }

        return deserialize(doc.object());
    } catch (const std::bad_alloc &) {
        return SaveData();
    } catch (const std::exception &) {
        return SaveData();
    }
}

// ============================================================================
// 序列化：SaveData → QJsonObject
// ============================================================================

QJsonObject SaveLoadManager::serialize(const SaveData &data)
{
    QJsonObject root;

    // ---- 版本与轮次 ----
    root[QStringLiteral("saveVersion")] = data.saveVersion;
    root[QStringLiteral("round")]        = data.round;

    // ---- 玩家状态 ----
    QJsonObject playerObj;
    playerObj[QStringLiteral("hp")]             = data.playerHp;
    playerObj[QStringLiteral("maxHp")]          = data.playerMaxHp;
    playerObj[QStringLiteral("gold")]           = data.playerGold;
    playerObj[QStringLiteral("level")]          = data.playerLevel;
    playerObj[QStringLiteral("exp")]            = data.playerExp;
    playerObj[QStringLiteral("populationCap")]  = data.playerPopCap;
    playerObj[QStringLiteral("winStreak")]      = data.winStreak;
    playerObj[QStringLiteral("lossStreak")]     = data.lossStreak;
    root[QStringLiteral("player")] = playerObj;

    // ---- 单位列表 ----
    QJsonArray unitsArray;
    for (const auto &u : data.units) {
        unitsArray.append(serializeUnit(u));
    }
    root[QStringLiteral("units")] = unitsArray;

    // ---- 商店 ----
    // 保存每个槽位的英雄名称（空串表示无英雄）
    QJsonArray shopArray;
    for (const QString &name : data.shopSlots) {
        shopArray.append(name);
    }
    root[QStringLiteral("shopSlots")] = shopArray;

    return root;
}

// ============================================================================
// 反序列化：QJsonObject → SaveData
// ============================================================================

SaveData SaveLoadManager::deserialize(const QJsonObject &json)
{
    SaveData data;

    // ---- 版本校验 ----
    int version = json[QStringLiteral("saveVersion")].toInt(0);
    if (version != SaveData::CURRENT_VERSION) {
        // 版本不匹配：返回默认构造（调用方会检测到 round==0）
        return data;
    }
    data.saveVersion = version;

    // ---- 轮次 ----
    data.round = json[QStringLiteral("round")].toInt(1);

    // ---- 玩家 ----
    QJsonObject playerObj = json[QStringLiteral("player")].toObject();
    data.playerHp        = playerObj[QStringLiteral("hp")].toInt(100);
    data.playerMaxHp     = playerObj[QStringLiteral("maxHp")].toInt(100);
    data.playerGold      = playerObj[QStringLiteral("gold")].toInt(10);
    data.playerLevel     = playerObj[QStringLiteral("level")].toInt(1);
    data.playerExp       = playerObj[QStringLiteral("exp")].toInt(0);
    data.playerPopCap    = playerObj[QStringLiteral("populationCap")].toInt(4);
    data.winStreak       = playerObj[QStringLiteral("winStreak")].toInt(0);
    data.lossStreak      = playerObj[QStringLiteral("lossStreak")].toInt(0);

    // ---- 单位 ----
    QJsonArray unitsArray = json[QStringLiteral("units")].toArray();
    for (const QJsonValue &val : unitsArray) {
        data.units.append(deserializeUnit(val.toObject()));
    }

    // ---- 商店 ----
    QJsonArray shopArray = json[QStringLiteral("shopSlots")].toArray();
    for (const QJsonValue &val : shopArray) {
        data.shopSlots.append(val.toString());
    }
    // 补齐到 SLOT_COUNT（5）个（旧存档可能少于 5 个）
    while (data.shopSlots.size() < 5)
        data.shopSlots.append(QString());

    return data;
}

// ============================================================================
// UnitInfo 序列化
// ============================================================================

QJsonObject SaveLoadManager::serializeUnit(const SaveData::UnitInfo &unit)
{
    QJsonObject obj;

    obj[QStringLiteral("name")]        = unit.name;
    obj[QStringLiteral("owner")]       = unit.owner;
    obj[QStringLiteral("hp")]          = unit.hp;
    obj[QStringLiteral("maxHp")]       = unit.maxHp;
    obj[QStringLiteral("atk")]         = unit.atk;
    obj[QStringLiteral("range")]       = unit.range;
    obj[QStringLiteral("maxMana")]     = unit.maxMana;
    obj[QStringLiteral("attackSpeed")] = unit.attackSpeed;
    obj[QStringLiteral("starLevel")]   = unit.starLevel;
    obj[QStringLiteral("boardCol")]    = unit.boardCol;
    obj[QStringLiteral("boardRow")]    = unit.boardRow;

    // 羁绊标签（字符串数组）
    QJsonArray traitsArray;
    for (const QString &t : unit.traits)
        traitsArray.append(t);
    obj[QStringLiteral("traits")] = traitsArray;

    // 装备
    obj[QStringLiteral("equipment")] = unit.equipment;

    return obj;
}

SaveData::UnitInfo SaveLoadManager::deserializeUnit(const QJsonObject &obj)
{
    SaveData::UnitInfo u;

    u.name        = obj[QStringLiteral("name")].toString();
    u.owner       = obj[QStringLiteral("owner")].toInt(0);
    u.hp          = obj[QStringLiteral("hp")].toInt(100);
    u.maxHp       = obj[QStringLiteral("maxHp")].toInt(100);
    u.atk         = obj[QStringLiteral("atk")].toInt(10);
    u.range       = obj[QStringLiteral("range")].toInt(1);
    u.maxMana     = obj[QStringLiteral("maxMana")].toInt(50);
    u.attackSpeed = obj[QStringLiteral("attackSpeed")].toInt(60);
    u.starLevel   = obj[QStringLiteral("starLevel")].toInt(1);
    u.boardCol    = obj[QStringLiteral("boardCol")].toInt(-1);
    u.boardRow    = obj[QStringLiteral("boardRow")].toInt(-1);

    // 羁绊标签
    QJsonArray traitsArray = obj[QStringLiteral("traits")].toArray();
    for (const QJsonValue &val : traitsArray)
        u.traits.append(val.toString());

    // 装备
    u.equipment = obj[QStringLiteral("equipment")].toString();

    return u;
}

// ============================================================================
// Trait 枚举 ↔ 字符串 转换
// ============================================================================

QString SaveLoadManager::traitToString(Trait t)
{
    switch (t) {
    case Trait::Archer:   return QStringLiteral("Archer");
    case Trait::Tank:     return QStringLiteral("Tank");
    case Trait::Warrior:  return QStringLiteral("Warrior");
    default:              return QString();
    }
}

Trait SaveLoadManager::stringToTrait(const QString &str)
{
    if (str == QStringLiteral("Archer"))   return Trait::Archer;
    if (str == QStringLiteral("Tank"))     return Trait::Tank;
    if (str == QStringLiteral("Warrior"))  return Trait::Warrior;
    return Trait::None;
}
