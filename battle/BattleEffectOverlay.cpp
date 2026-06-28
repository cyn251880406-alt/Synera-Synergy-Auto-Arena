#include "battle/BattleEffectOverlay.h"

#include <QPainter>
#include <QtMath>

// ============================================================================
// 构造
// ============================================================================

BattleEffectOverlay::BattleEffectOverlay(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setPos(0, 0);
    setZValue(2.0);  // 高于 UnitItem（1.0），显示在最上层
}

// ============================================================================
// 边界矩形
// ============================================================================

QRectF BattleEffectOverlay::boundingRect() const
{
    return QRectF(0, 0, SCENE_W, SCENE_H);
}

// ============================================================================
// 特效生成
// ============================================================================

void BattleEffectOverlay::spawnAttackProjectile(QPointF from, QPointF to,
                                                 bool isRanged)
{
    BattleEffectParticle p;
    p.type        = EffectType::AttackProjectile;
    p.pos         = from;
    p.startPos    = from;
    p.target      = to;
    p.maxLifetime = isRanged ? 10 : 4;  // 远程飞行久，近战瞬间
    p.radius      = isRanged ? 4 : 2;
    p.color       = isRanged ? QColor(255, 220, 80) : QColor(255, 255, 220);
    m_particles.append(p);
}

void BattleEffectOverlay::spawnSkillAOE(QPointF center, int boardRadius,
                                         const QColor &color)
{
    BattleEffectParticle p;
    p.type        = EffectType::SkillAOE;
    p.pos         = center;
    p.maxLifetime = 15;  // 约 0.25 秒
    p.radius      = boardRadius * 64;  // 棋盘格 → 像素
    p.color       = color;
    m_particles.append(p);
}

void BattleEffectOverlay::spawnSkillProjectile(QPointF from, QPointF to,
                                                const QColor &color)
{
    BattleEffectParticle p;
    p.type        = EffectType::SkillProjectile;
    p.pos         = from;
    p.startPos    = from;
    p.target      = to;
    p.maxLifetime = 8;
    p.radius      = 5;
    p.color       = color;
    m_particles.append(p);
}

void BattleEffectOverlay::spawnDeathEffect(QPointF pos)
{
    // 死亡时生成多个小粒子向外散开
    for (int i = 0; i < 6; ++i) {
        BattleEffectParticle p;
        p.type        = EffectType::DeathExplosion;
        p.pos         = pos;
        p.target      = pos + QPointF(
            (qCos(i * 1.047) * 20),   // 均分 360°
            (qSin(i * 1.047) * 20));
        p.maxLifetime = 12;
        p.radius      = 2;
        p.color       = QColor(200, 40, 40);
        m_particles.append(p);
    }
}

void BattleEffectOverlay::spawnHealEffect(QPointF pos)
{
    BattleEffectParticle p;
    p.type        = EffectType::HealGlow;
    p.pos         = pos;
    p.maxLifetime = 20;
    p.radius      = 20;
    p.color       = QColor(60, 220, 80);
    m_particles.append(p);
}

void BattleEffectOverlay::spawnBuffIndicator(QPointF pos)
{
    BattleEffectParticle p;
    p.type        = EffectType::BuffIndicator;
    p.pos         = pos;
    p.maxLifetime = 30;
    p.radius      = 10;
    p.color       = QColor(255, 215, 0);
    m_particles.append(p);
}

void BattleEffectOverlay::spawnSkillEffectImage(QPointF center, const QPixmap &pixmap)
{
    BattleEffectParticle p;
    p.type        = EffectType::SkillEffectImage;
    p.pos         = center;
    p.maxLifetime = 25;  // 约 0.4 秒
    p.radius      = 96;  // 显示半径（像素）
    p.pixmap      = pixmap;
    m_particles.append(p);
}

void BattleEffectOverlay::spawnSkillEffectProjectile(QPointF from, QPointF to,
                                                      const QPixmap &pixmap)
{
    BattleEffectParticle p;
    p.type        = EffectType::SkillEffectImage;
    p.pos         = from;
    p.startPos    = from;
    p.target      = to;
    p.maxLifetime = 14;  // 快速飞行
    p.radius      = 64;  // 飞弹显示尺寸
    p.pixmap      = pixmap;
    m_particles.append(p);
}

// ============================================================================
// 逐帧更新
// ============================================================================

void BattleEffectOverlay::updateEffects()
{
    // —— 推进所有粒子 ——
    for (auto &p : m_particles) {
        ++p.elapsed;
        // 位置插值由 currentPos() 在 paint 时实时计算，
        // 不再在 update 中累积修改 pos，避免非线性运动
    }

    // —— 清理过期粒子 ——
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const BattleEffectParticle &p)
                       { return p.isExpired(); }),
        m_particles.end());

    // 触发重绘
    update();
}

// ============================================================================
// 绘制
// ============================================================================

void BattleEffectOverlay::paint(QPainter *painter,
                                 const QStyleOptionGraphicsItem * /*option*/,
                                 QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    for (const auto &p : m_particles) {
        float alpha = 1.f - p.progress();  // 随时间衰减
        if (alpha < 0.f) alpha = 0.f;
        QColor c(p.color.red(), p.color.green(), p.color.blue(),
                 static_cast<int>(alpha * 220));

        switch (p.type) {
        case EffectType::AttackProjectile: {
            QPointF cur = p.currentPos();
            painter->setPen(Qt::NoPen);
            painter->setBrush(c);
            painter->drawEllipse(cur, p.radius, p.radius);
            // 拖尾
            painter->setBrush(QColor(c.red(), c.green(), c.blue(),
                                     static_cast<int>(alpha * 100)));
            painter->drawEllipse(cur, p.radius + 2, p.radius + 2);
            break;
        }
        case EffectType::SkillAOE: {
            // 扩环：半径从 0 到 max
            float expand = p.progress();
            int r = static_cast<int>(p.radius * expand);
            painter->setPen(QPen(c, 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(p.pos, r, r);
            break;
        }
        case EffectType::SkillProjectile: {
            QPointF cur = p.currentPos();
            // 拖尾光晕
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(c.red(), c.green(), c.blue(),
                                     static_cast<int>(alpha * 80)));
            painter->drawEllipse(cur, p.radius + 4, p.radius + 4);
            // 主体
            painter->setBrush(c);
            painter->drawEllipse(cur, p.radius, p.radius);
            // 中心白点
            painter->setBrush(QColor(255, 255, 255,
                                     static_cast<int>(alpha * 200)));
            painter->drawEllipse(cur, 2, 2);
            break;
        }
        case EffectType::DeathExplosion: {
            float t = p.progress();
            QPointF cur = p.pos + (p.target - p.pos) * t;
            painter->setPen(Qt::NoPen);
            painter->setBrush(c);
            painter->drawEllipse(cur, p.radius, p.radius);
            break;
        }
        case EffectType::HealGlow: {
            // 上升光环 + 十字
            float t = p.progress();
            QPointF up(p.pos.x(), p.pos.y() - t * 20);
            painter->setPen(QPen(c, 2));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(up, p.radius, static_cast<int>(p.radius * (1.f - t * 0.5f)));
            // 绿色十字
            painter->drawLine(QPointF(up.x() - 6, up.y()), QPointF(up.x() + 6, up.y()));
            painter->drawLine(QPointF(up.x(), up.y() - 6), QPointF(up.x(), up.y() + 6));
            break;
        }
        case EffectType::BuffIndicator: {
            // 闪烁星芒
            float t = p.progress();
            if (static_cast<int>(t * 10) % 2 == 0) break;  // 闪烁效果
            painter->setPen(Qt::NoPen);
            painter->setBrush(c);
            painter->drawEllipse(p.pos, p.radius, p.radius);
            break;
        }
        case EffectType::SkillEffectImage: {
            if (p.pixmap.isNull()) break;
            float t = p.progress();
            // 判断是否为飞弹模式：startPos 与 target 有实际距离
            double dx = p.target.x() - p.startPos.x();
            double dy = p.target.y() - p.startPos.y();
            bool isProjectile = (dx * dx + dy * dy > 1.0);

            if (isProjectile) {
                // —— 飞弹模式：从起点飞向目标，旋转朝向目标 ——
                QPointF cur = p.currentPos();
                float fade = 1.f - qAbs(t - 0.5f) * 2.f;  // 中段最亮，两端渐隐
                if (fade < 0.1f) fade = 0.1f;
                float alphaP = alpha * fade;
                if (alphaP > 1.f) alphaP = 1.f;
                if (alphaP < 0.f) alphaP = 0.f;

                // 计算飞行角度
                double angleDeg = qRadiansToDegrees(qAtan2(dy, dx));

                // 缩放：随着飞行逐渐变大
                float scale = 0.6f + t * 0.6f;
                int size = static_cast<int>(p.radius * scale);
                QPixmap scaled = p.pixmap.scaled(size, size,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);

                // 旋转
                QTransform tr;
                tr.translate(cur.x(), cur.y());
                tr.rotate(angleDeg);
                QPointF topLeft(-scaled.width() / 2.0, -scaled.height() / 2.0);
                painter->setOpacity(alphaP);
                painter->setTransform(tr, true);
                painter->drawPixmap(topLeft, scaled);
                painter->setTransform(QTransform());  // 恢复
                painter->setOpacity(1.0);
            } else {
                // —— 静态模式：从中心扩展开并淡出 ——
                float scale = (t < 0.3f) ? (0.5f + t / 0.3f * 0.7f)
                                         : (1.2f - (t - 0.3f) / 0.7f * 0.5f);
                int size = static_cast<int>(p.radius * scale);
                QPixmap scaled = p.pixmap.scaled(size, size,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QPointF topLeft(p.pos.x() - scaled.width() / 2.0,
                               p.pos.y() - scaled.height() / 2.0);
                painter->setOpacity(alpha);
                painter->drawPixmap(topLeft, scaled);
                painter->setOpacity(1.0);
            }
            break;
        }
        }
    }
}
