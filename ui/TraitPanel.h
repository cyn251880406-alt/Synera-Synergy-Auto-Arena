#ifndef TRAITPANEL_H
#define TRAITPANEL_H

#include <QDialog>
#include <QTimer>

class Game;

class TraitPanel : public QDialog {
    Q_OBJECT
public:
    explicit TraitPanel(Game *game, QWidget *parent = nullptr);
    void refresh();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Game *m_game;
    QTimer *m_refreshTimer;
};

#endif
