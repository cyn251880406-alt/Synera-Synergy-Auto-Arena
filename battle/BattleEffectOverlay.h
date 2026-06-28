#ifndef BATTLEEFFECTOVERLAY_H
#define BATTLEEFFECTOVERLAY_H

#include <QGraphicsItem>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QPixmap>

// ============================================================================
// BattleEffectParticle — 单个战斗特效粒子
// ============================================================================

enum class EffectType {
    AttackProjectile,   ///< 攻击弹道（物理）
    SkillAOE,           ///< 技能范围特效
    SkillProjectile,    ///< 技能弹道（法术）
    DeathExplosion,     ///< 死亡消散
    HealGlow,           ///< 治疗光效
    BuffIndicator,      ///< Buff 激活提示
    SkillEffectImage    ///< 技能特效贴图（从素材加载的 PNG）
};

struct BattleEffectParticle {
    EffectType type;
    QPointF    pos;          ///< 基准位置（静态特效=中心点；projectile=起点，由 currentPos() 插值）
    QPointF    startPos;     ///< projectile 起点（单独存储，避免 updateEffects 中累积插值）
    QPointF    target;       ///< 目标位置（projectile 类型使用）
    int        elapsed = 0;  ///< 已存活帧数
    int        maxLifetime;  ///< 最大存活帧数
    int        radius  = 0;  ///< 范围/粒子大小
    QColor     color;        ///< 主色调
    QPixmap    pixmap;       ///< 技能特效贴图（SkillEffectImage 类型使用）

    bool isExpired() const { return elapsed >= maxLifetime; }
    float progress() const { return maxLifetime > 0 ? (float)elapsed / maxLifetime : 0.f; }

    /// Projectile 当前插值位置（从 startPos 线性飞到 target）
    QPointF currentPos() const {
        float t = progress();
        return QPointF(
            startPos.x() + (target.x() - startPos.x()) * t,
            startPos.y() + (target.y() - startPos.y()) * t
        );
    }
};

// ============================================================================
// BattleEffectOverlay — 战斗特效叠加层
// ============================================================================

/**
 * @brief BattleEffectOverlay — 在棋盘上绘制攻击弹道、技能AOE、死亡等视觉特效
 *
 * 职责：
 *   - 管理当前帧所有活跃特效粒子
 *   - 每帧 updateEffects() 推进动画 & 清理过期粒子
 *   - paint() 绘制所有粒子
 *
 * 绘制内容：
 *   - AttackProjectile: 从 attacker → target 的彩色点/线
 *   - SkillAOE:        以目标为中心的衰减环
 *   - SkillProjectile: 从 caster → target 的法术飞行物
 *   - DeathExplosion:  单位消失位置的红色粒子散开
 *   - HealGlow:        绿色十字+光环上升
 *   - BuffIndicator:   金色小标记闪烁
 *
 * 用法示例：
 * @code
 *   auto *overlay = new BattleEffectOverlay();
 *   board.scene()->addItem(overlay);
 *
 *   overlay->spawnAttackProjectile(fromPx, toPx, false);
 *   overlay->spawnSkillAOE(centerPx, 2, QColor(255, 100, 0));
 *   // 每帧调用 overlay->updateEffects();
 * @endcode
 */
class BattleEffectOverlay : public QGraphicsItem {
public:
    BattleEffectOverlay(QGraphicsItem *parent = nullptr);
    ~BattleEffectOverlay() override = default;

    // ========== 特效生成 ==========

    /// 攻击弹道（近战=白线，远程=黄点到目标）
    void spawnAttackProjectile(QPointF from, QPointF to, bool isRanged);

    /// 技能 AOE 范围特效（扩环+渐变颜色）
    void spawnSkillAOE(QPointF center, int boardRadius, const QColor &color);

    /// 技能弹道（法术飞行物）
    void spawnSkillProjectile(QPointF from, QPointF to, const QColor &color);

    /// 单位死亡特效（消散粒子群）
    void spawnDeathEffect(QPointF pos);

    /// 治疗光效（绿色上升光点）
    void spawnHealEffect(QPointF pos);

    /// Buff 激活提示（金色闪烁）
    void spawnBuffIndicator(QPointF pos);

    /// 技能特效贴图（从素材加载的 PNG），从中心扩展开并淡出
    void spawnSkillEffectImage(QPointF center, const QPixmap &pixmap);

    /// 技能特效飞弹——特效贴图从起点飞行到目标点，自动旋转朝向目标
    void spawnSkillEffectProjectile(QPointF from, QPointF to, const QPixmap &pixmap);

    // ========== 更新 ==========
    void updateEffects();   // 逐帧推进 & 清理过期

    // ========== QGraphicsItem 重写 ==========
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override;

private:
    QVector<BattleEffectParticle> m_particles;

    // 棋盘大小（像素）
    static constexpr int SCENE_W = 8 * 64;
    static constexpr int SCENE_H = 8 * 64;
};

#endif // BATTLEEFFECTOVERLAY_H
