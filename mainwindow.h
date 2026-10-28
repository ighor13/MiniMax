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

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startAlgorithm();
    void pauseAnimation();
    void stopAnimation();
    void stepOnce();
    void stepAnimation();

private:
    void setupUi();
    void buildTree();
    void layoutTree(GameNode& node, int level, int& currentX, int widthPerLeaf);
    void drawConnections();
    void markSubtreeAsPruned(int nodeId, const QString& reason);
    int minimax(int nodeId, int depth, bool isMax, int alpha, int beta);

    QGraphicsScene *m_scene;
    QGraphicsView *m_view;
    QTimer *m_animTimer;
    QCheckBox *m_chkRootIsMax;
    QCheckBox *m_chkMarkLeaves;
    QCheckBox *m_chkShowReasons; // <--- НОВЫЙ ЧЕКБОКС

    QPushButton *m_btnStart;
    QPushButton *m_btnPause;
    QPushButton *m_btnStop;
    QPushButton *m_btnStep;

    QVector<GameNode> m_nodes;
    QVector<NodeItem*> m_items;
    QVector<AnimStep> m_animationSteps;

    int m_currentStep = 0;
    int m_rootIndex = 0;
    int m_evaluatedLeaves = 0;
};
#endif