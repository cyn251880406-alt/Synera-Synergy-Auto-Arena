#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QShortcut>
#include <QKeySequence>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

#include "board/BoardWidget.h"
#include "bench/Bench.h"
#include "bench/BenchWidget.h"
#include "core/Game.h"
#include "ui/PlayerHud.h"
#include "ui/TraitPanel.h"
#include "shop/ShopWidget.h"
#include "ui/EquipWidget.h"
#include "ui/ActionButtonPanel.h"
#include "ui/SellZone.h"
#include "ui/ResultOverlay.h"
#include "ui/ResourceManager.h"
#include "ui/BgmPlayer.h"
#include "unit/Unit.h"
#include "unit/UnitItem.h"
#include "skill/HeroSkills.h"

// ============================================================================
// BoardContainer — 透明棋盘容器
// ============================================================================

class BoardContainer : public QWidget {
public:
    BoardContainer(BoardWidget *board, QWidget *parent = nullptr)
        : QWidget(parent), m_board(board)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet("background: transparent;");
        m_board->setParent(this);
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        m_board->setGeometry(rect());
        m_board->fitInView(m_board->scene()->sceneRect(), Qt::KeepAspectRatio);
    }

private:
    BoardWidget *m_board;
};

// ============================================================================
// BgResizer — 让背景图填满整个窗口
// ============================================================================

class BgResizer : public QObject {
public:
    BgResizer(QLabel *bgLabel) : m_bg(bgLabel) {}
    bool eventFilter(QObject *, QEvent *e) override
    {
        if (e->type() == QEvent::Resize)
            m_bg->setGeometry(QRect(QPoint(0,0), static_cast<QResizeEvent*>(e)->size()));
        return false;
    }
private:
    QLabel *m_bg;
};

// ============================================================================
// main
// ============================================================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // ========== 0. 素材 ==========
    ResourceManager::instance().init();

    // 背景音乐 — 使用绝对路径
    BgmPlayer bgm;
    QString appDir = QCoreApplication::applicationDirPath();
    // build/ 或 build2/ → 上一级就是项目根
    QString bgmPath1 = QFileInfo(appDir + "/../assets/bgm/bgm.mp3").absoluteFilePath();
    QString bgmPath2 = QFileInfo(appDir + "/assets/bgm/bgm.mp3").absoluteFilePath();
    if (QFile::exists(bgmPath1)) {
        qDebug() << "BGM loaded:" << bgmPath1;
        bgm.play(bgmPath1);
    } else if (QFile::exists(bgmPath2)) {
        qDebug() << "BGM loaded:" << bgmPath2;
        bgm.play(bgmPath2);
    } else {
        qDebug() << "BGM not found. Tried:" << bgmPath1 << bgmPath2;
    }

    // ========== 1. 主窗口 ==========
    QWidget mainWindow;
    mainWindow.setWindowTitle(QString::fromUtf8("Synera — 自走棋"));
    mainWindow.resize(960, 700);
    mainWindow.setMinimumSize(820, 580);

    // 全窗口背景图（底层，不在 layout 中）
    QLabel *bgLabel = new QLabel(&mainWindow);
    bgLabel->setScaledContents(true);
    QPixmap bg = ResourceManager::instance().boardBg();
    if (!bg.isNull())
        bgLabel->setPixmap(bg);
    else
        bgLabel->setStyleSheet("background: #1a1a2e;");
    bgLabel->setGeometry(mainWindow.rect());
    bgLabel->lower();

    BgResizer *bgResizer = new BgResizer(bgLabel);
    mainWindow.installEventFilter(bgResizer);

    // ========== 2. 创建所有控件 ==========
    BoardWidget board;
    Bench bench;
    BenchWidget benchWidget;
    benchWidget.setBench(&bench);

    Game game(&board);
    game.setBench(&bench, &benchWidget);

    BoardContainer *boardContainer = new BoardContainer(&board, &mainWindow);

    PlayerHud *hud = new PlayerHud(&game, &mainWindow);
    TraitPanel *traitDialog = new TraitPanel(&game, &mainWindow);
    SellZone *sellZone = new SellZone(&mainWindow);
    ShopWidget *shopWidget = new ShopWidget(&game, &mainWindow);
    ActionButtonPanel *actionPanel = new ActionButtonPanel(&game, &mainWindow);

    // ========== 3. 布局 ==========
    // BenchRow
    QWidget *benchRow = new QWidget(&mainWindow);
    benchRow->setFixedHeight(70);
    benchRow->setAttribute(Qt::WA_TranslucentBackground);
    benchRow->setStyleSheet("background: rgba(20,22,28,180);");
    QHBoxLayout *benchLayout = new QHBoxLayout(benchRow);
    benchLayout->setContentsMargins(4, 2, 4, 2);
    benchLayout->setSpacing(4);
    benchLayout->addWidget(&benchWidget, 1);
    benchLayout->addWidget(sellZone, 0, Qt::AlignVCenter);

    // ShopRow
    QWidget *shopRow = new QWidget(&mainWindow);
    shopRow->setFixedHeight(120);
    shopRow->setAttribute(Qt::WA_TranslucentBackground);
    shopRow->setStyleSheet("background: rgba(20,22,28,180);");
    QHBoxLayout *shopLayout = new QHBoxLayout(shopRow);
    shopLayout->setContentsMargins(4, 2, 4, 2);
    shopLayout->setSpacing(6);
    shopLayout->addWidget(shopWidget, 0, Qt::AlignCenter);
    shopLayout->addWidget(actionPanel, 0, Qt::AlignVCenter);

    // 根布局
    QVBoxLayout *rootLayout = new QVBoxLayout(&mainWindow);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(hud, 0);
    rootLayout->addWidget(boardContainer, 7);
    rootLayout->addWidget(benchRow, 0);
    rootLayout->addWidget(shopRow, 0);

    // ========== 4. 我方初始单位 ==========
    Unit *marksman = new Unit(
        QString::fromUtf8("神射手"), Owner::PlayerCtrl,
        QPoint(4, 7), 220, 32, 3, 60);
    marksman->addTrait(Trait::Archer);
    UnitItem *marksmanItem = new UnitItem(marksman);
    marksmanItem->setGame(&game);
    board.placeUnitItem(marksmanItem, 7, 4);
    game.addUnit(marksman, marksmanItem);

    Unit *heavyTank = new Unit(
        QString::fromUtf8("重装兵"), Owner::PlayerCtrl,
        QPoint(2, 7), 450, 18, 1, 70);
    heavyTank->addTrait(Trait::Tank);
    UnitItem *heavyTankItem = new UnitItem(heavyTank);
    heavyTankItem->setGame(&game);
    board.placeUnitItem(heavyTankItem, 7, 2);
    game.addUnit(heavyTank, heavyTankItem);

    Unit *swordsman = new Unit(
        QString::fromUtf8("剑术师"), Owner::PlayerCtrl,
        QPoint(3, 6), 320, 28, 1, 55);
    swordsman->addTrait(Trait::Warrior);
    UnitItem *swordsmanItem = new UnitItem(swordsman);
    swordsmanItem->setGame(&game);
    board.placeUnitItem(swordsmanItem, 6, 3);
    game.addUnit(swordsman, swordsmanItem);

    ArrowRain    arrowRainSkill;
    IronWall     ironWallSkill;
    PowerStrike  powerStrikeSkill;
    marksman->setSkill(&arrowRainSkill);
    heavyTank->setSkill(&ironWallSkill);
    swordsman->setSkill(&powerStrikeSkill);

    game.spawnEnemyWave();
    game.setSellZone(sellZone);
    marksmanItem->setBenchWidget(&benchWidget);
    marksmanItem->setSellZone(sellZone);
    heavyTankItem->setBenchWidget(&benchWidget);
    heavyTankItem->setSellZone(sellZone);
    swordsmanItem->setBenchWidget(&benchWidget);
    swordsmanItem->setSellZone(sellZone);

    // ========== 5. 信号连接 ==========
    QObject::connect(&board, &BoardWidget::startBattleRequested,
        [&game]() {
            if (game.currentPhase() == Game::Phase::Preparation)
                game.startBattle();
        });

    QObject::connect(actionPanel, &ActionButtonPanel::refreshRequested,
        [&game]() { game.refreshShop(); });

    QObject::connect(actionPanel, &ActionButtonPanel::startBattleRequested,
        [&game]() {
            if (game.currentPhase() == Game::Phase::Preparation)
                game.startBattle();
        });

    QObject::connect(actionPanel, &ActionButtonPanel::restartRequested,
        [&mainWindow]() {
            QMessageBox::information(&mainWindow,
                QString::fromUtf8("重新开始"),
                QString::fromUtf8("请关闭窗口后重新运行。"));
        });

    QObject::connect(hud, &PlayerHud::traitButtonClicked,
        [traitDialog]() { traitDialog->show(); });

    ResultOverlay *resultOverlay = new ResultOverlay(&mainWindow);

    QObject::connect(&game, &Game::battleEnded,
        [resultOverlay, &game](bool playerWin) {
            if (playerWin && game.roundNumber() < Game::MAX_ROUNDS)
                resultOverlay->showVictory();
        });

    QObject::connect(&game, &Game::gameWon,
        [resultOverlay, &app]() {
            resultOverlay->showVictory();
            QObject::connect(resultOverlay, &ResultOverlay::dismissed,
                             &app, &QApplication::quit);
        });

    QObject::connect(&game, &Game::gameOver,
        [resultOverlay, &app]() {
            resultOverlay->showDefeat();
            QObject::connect(resultOverlay, &ResultOverlay::dismissed,
                             &app, &QApplication::quit);
        });

    // ========== 6. 存档/读档快捷键 ==========

    // 浮动提示标签（短暂显示后自动消失）
    QLabel *toast = new QLabel(&mainWindow);
    toast->setAlignment(Qt::AlignCenter);
    toast->setStyleSheet(
        "QLabel { background: rgba(0,0,0,200); color: #FFD700; font-size: 16px; "
        "font-weight: bold; border-radius: 8px; padding: 10px 20px; }");
    toast->setFixedSize(200, 44);
    toast->hide();
    QTimer *toastTimer = new QTimer(toast);
    toastTimer->setSingleShot(true);
    QObject::connect(toastTimer, &QTimer::timeout, toast, &QLabel::hide);

    auto showToast = [toast, toastTimer, &mainWindow](const QString &msg) {
        toast->setText(msg);
        QPoint center = mainWindow.rect().center();
        toast->move(center.x() - toast->width() / 2,
                    mainWindow.height() - 120);
        toast->show();
        toast->raise();
        toastTimer->start(1500);
    };

    // Ctrl+S — 存档
    QShortcut *saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), &mainWindow);
    QObject::connect(saveShortcut, &QShortcut::activated,
        [&game, showToast]() {
            if (game.saveGame("save.json"))
                showToast(QString::fromUtf8("游戏已保存"));
            else
                showToast(QString::fromUtf8("保存失败"));
        });

    // Ctrl+L — 读档
    QShortcut *loadShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), &mainWindow);
    QObject::connect(loadShortcut, &QShortcut::activated,
        [&game, showToast]() {
            if (game.loadGame("save.json"))
                showToast(QString::fromUtf8("游戏已加载"));
            else
                showToast(QString::fromUtf8("未找到存档"));
        });

    game.shop().refreshShop(game.roundNumber());
    game.startLoop();

    mainWindow.show();
    return app.exec();
}
