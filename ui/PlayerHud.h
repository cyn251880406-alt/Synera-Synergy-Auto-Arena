#ifndef PLAYERHUD_H
#define PLAYERHUD_H

#include <QWidget>
#include <QTimer>
#include <QString>
#include <QVector>

#include "unit/Unit.h"

class Game;
class EquipWidget;

class PlayerHud : public QWidget {
    Q_OBJECT

public:
    explicit PlayerHud(Game *game, QWidget *parent = nullptr);
    ~PlayerHud() override = default;

    QSize sizeHint() const override;
    EquipWidget *equipWidget() const { return m_equipWidget; }

signals:
    void traitButtonClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onAnimationTick();
    void onHpChanged(int hp, int maxHp);
    void onGoldChanged(int gold);

private:
    Game *m_game = nullptr;
    QTimer *m_animTimer = nullptr;
    EquipWidget *m_equipWidget = nullptr;

    int m_targetHp = 100, m_targetMaxHp = 100;
    int m_targetGold = 10, m_targetLevel = 1;
    int m_targetExp = 0, m_targetExpToLevel = 4;
    int m_targetPopCap = 4, m_targetDeployed = 0;
    int m_winStreak = 0, m_lossStreak = 0;

    double m_animHp = 100.0, m_animExp = 0.0;
    int m_hpFlashCounter = 0, m_goldFlashCounter = 0;
    int m_levelFlashCounter = 0, m_prevLevel = 1;

    static constexpr int FLASH_DURATION = 8;
    static constexpr int BAR_HEIGHT = 40;

    void refreshTargets();
};

#endif
