#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QVector>
#include <QCheckBox>
#include <QPushButton>
#include "gameutils.h"
#include "nodeitem.h"

// Определение — в mainwindow.cpp, на глобальном уровне (без namespace).
class CutoffLinkItem;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void startAlgorithm();
    void pauseAnimation();
    void stopAnimation();
    void stepOnce();
    void stepAnimation();
    void updateCutoffLabels();

private:
    void setupUi();
    void buildTree();
    void layoutTree(GameNode& node, int level, int& currentX, int widthPerLeaf);
    void drawConnections();

    void prepareSteps();
    void resetAllNodes();

    int minimax(int nodeId, bool isMax, Window window);
    void appendEvalStep(int nodeId, int value, const Window& window);
    AnimStep makeCutoffStep(int decisionNode, const Window& window) const;

    // Возвращает количество узлов, реально помеченных как отсечённые.
    int markSubtreeAsPruned(int nodeId);

    void applyCutoffVisual(const AnimStep& step);
    void clearCutoffVisuals();

    // Все указатели инициализированы, иначе деструктор может удалить мусор.
    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    QTimer *m_animTimer = nullptr;
    QCheckBox *m_chkRootIsMax = nullptr;
    QCheckBox *m_chkMarkLeaves = nullptr;
    QCheckBox *m_chkShowReasons = nullptr;

    QPushButton *m_btnStart = nullptr;
    QPushButton *m_btnPause = nullptr;
    QPushButton *m_btnStop = nullptr;
    QPushButton *m_btnStep = nullptr;

    QVector<GameNode> m_nodes;
    QVector<NodeItem*> m_items;
    QVector<AnimStep> m_animationSteps;

    QVector<CutoffLinkItem*> m_cutoffItems;   // накопленные линии отсечений
    QVector<int> m_markedNodes;               // узлы, помеченные на текущем отсечении

    int m_currentStep = 0;
    int m_rootIndex = 0;
    int m_evaluatedLeaves = 0;
    int m_realCutoffs = 0;
};
#endif