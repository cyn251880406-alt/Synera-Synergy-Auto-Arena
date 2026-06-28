#ifndef EQUIPWIDGET_H
#define EQUIPWIDGET_H

#include <QWidget>

class Game;

class EquipWidget : public QWidget {
    Q_OBJECT
public:
    explicit EquipWidget(Game *game, QWidget *parent = nullptr);
    ~EquipWidget() override = default;

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;

private:
    Game *m_game;

    static constexpr int CARD_SIZE = 32;
    static constexpr int CARD_GAP  = 4;

    int m_selectedIdx = -1;
    int m_pressIndex = -1;
    QPoint m_pressPos;
    static constexpr int DRAG_THRESHOLD = 8;

    QRect cardRect(int index, int count) const;
};

#endif
