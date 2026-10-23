#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    buildTree();
    drawConnections();

    QTimer::singleShot(0, this, [this]() {
        m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    });
}

MainWindow::~MainWindow() {
    // Отвязываем сцену от вида, чтобы избежать проблем с порядком удаления
    if (m_view) {
        m_view->setScene(nullptr);
    }
}

void MainWindow::setupUi() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);

    m_btnStart = new QPushButton("Старт");
    m_btnStart->setFixedSize(80, 40);
    connect(m_btnStart, &QPushButton::clicked, this, &MainWindow::startAlgorithm);

    m_btnPause = new QPushButton("Пауза");
    m_btnPause->setFixedSize(80, 40);
    connect(m_btnPause, &QPushButton::clicked, this, &MainWindow::pauseAnimation);

    m_btnStop = new QPushButton("Стоп");
    m_btnStop->setFixedSize(80, 40);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::stopAnimation);

    m_btnStep = new QPushButton("Шаг");
    m_btnStep->setFixedSize(80, 40);
    connect(m_btnStep, &QPushButton::clicked, this, &MainWindow::stepOnce);

    m_chkRootIsMax = new QCheckBox("Корень MAX");
    m_chkRootIsMax->setChecked(false);
    connect(m_chkRootIsMax, &QCheckBox::toggled, this, [this]() {
        buildTree();
        drawConnections();
    });

    m_chkMarkLeaves = new QCheckBox("Метить листья");
    m_chkMarkLeaves->setChecked(false);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(m_btnStart);
    topLayout->addWidget(m_btnPause);
    topLayout->addWidget(m_btnStop);
    topLayout->addWidget(m_btnStep);
    topLayout->addWidget(m_chkRootIsMax);
    topLayout->addWidget(m_chkMarkLeaves);
    topLayout->addStretch();

    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);

    layout->addLayout(topLayout);
    layout->addWidget(m_view);
    setCentralWidget(central);
    resize(1400, 800);

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, &MainWindow::stepAnimation);
}

void MainWindow::buildTree() {
    m_animationSteps.clear();
    m_currentStep = 0;
    m_nodes.clear();
    m_items.clear();
    m_scene->clear();

    int id = 0;
    int leafIndex = 0;
    int idx0 = 1, idx1 = 1, idx2 = 1, idx3 = 1, idx4 = 1;

    bool rootIsMax = m_chkRootIsMax->isChecked();
    NodeType rootType = rootIsMax ? NodeType::MAX : NodeType::MIN;
    NodeType nextType = rootIsMax ? NodeType::MIN : NodeType::MAX;

    QVector<int> level1, level2, level3;

    m_nodes.append({id, 0, false, rootType, {0,0}, {}, -1, 0, idx0++});
    m_rootIndex = id++;

    for (int i = 0; i < 3; ++i) {
        m_nodes.append({id, 0, false, nextType, {0,0}, {}, m_rootIndex, 1, idx1++});
        m_nodes[m_rootIndex].children.append(id);
        level1.append(id++);
    }

    int level1Branching[] = {2, 3, 2};
    NodeType level2Type = rootType;
    for (int i = 0; i < level1.size(); ++i) {
        int parentId = level1[i];
        for (int j = 0; j < level1Branching[i]; ++j) {
            m_nodes.append({id, 0, false, level2Type, {0,0}, {}, parentId, 2, idx2++});
            m_nodes[parentId].children.append(id);
            level2.append(id++);
        }
    }

    int level2Branching[] = {3, 3, 2, 2, 2, 2, 1};
    NodeType level3Type = nextType;
    for (int i = 0; i < level2.size(); ++i) {
        int parentId = level2[i];
        for (int j = 0; j < level2Branching[i]; ++j) {
            m_nodes.append({id, 0, false, level3Type, {0,0}, {}, parentId, 3, idx3++});
            m_nodes[parentId].children.append(id);
            level3.append(id++);
        }
    }

    int level3Branching[] = {2, 2, 3, 2, 3, 3, 2, 2, 2, 3, 2, 3, 2, 2, 3};
    for (int i = 0; i < level3.size(); ++i) {
        int parentId = level3[i];
        for (int j = 0; j < level3Branching[i]; ++j) {
            int val = LEAF_VALUES[leafIndex % LEAF_VALUES.size()];
            leafIndex++;
            m_nodes.append({id, val, true, NodeType::LEAF, {0,0}, {}, parentId, 4, idx4++});
            m_nodes[parentId].children.append(id);
            id++;
        }
    }

    for(const auto& node : m_nodes) {
        NodeItem *item = new NodeItem(node.id);
        item->setState(node.isEvaluated, node.value);
        m_scene->addItem(item);
        m_items.append(item);
    }

    int currentX = 0;
    int widthPerLeaf = 60;
    layoutTree(m_nodes[m_rootIndex], 0, currentX, widthPerLeaf);

    qreal minX = m_scene->itemsBoundingRect().left();
    QStringList levelNames;
    if (rootIsMax) {
        levelNames << "MAX (Корень)" << "MIN" << "MAX" << "MIN" << "Листья";
    } else {
        levelNames << "MIN (Корень)" << "MAX" << "MIN" << "MAX" << "Листья";
    }

    for (int i = 0; i < levelNames.size(); ++i) {
        QGraphicsSimpleTextItem *label = new QGraphicsSimpleTextItem(levelNames[i]);
        QFont font = label->font();
        font.setBold(true);
#ifdef Q_OS_ANDROID
        font.setPointSize(26);
#else
        font.setPointSize(20);
#endif
        label->setFont(font);
        label->setBrush(QBrush(Qt::red));
        qreal y = i * 140;
        label->setPos(minX - 150, y - 15);
        m_scene->addItem(label);
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect());
}

void MainWindow::layoutTree(GameNode& node, int level, int& currentX, int widthPerLeaf) {
    node.pos.setY(level * 140);

    if (node.children.isEmpty()) {
        node.pos.setX(currentX);
        currentX += widthPerLeaf;
    } else {
        int startX = currentX;
        for (int childIdx : node.children) {
            layoutTree(m_nodes[childIdx], level + 1, currentX, widthPerLeaf);
        }
        int endX = currentX - widthPerLeaf;
        node.pos.setX((startX + endX) / 2.0);
    }

    if(node.id < m_items.size()) {
        m_items[node.id]->setPos(node.pos);
    }
}

void MainWindow::drawConnections() {
    QPen linePen(Qt::darkGray, 1.5);
    for (const auto& node : m_nodes) {
        if (node.parent != -1) {
            QPointF p1 = m_nodes[node.parent].pos;
            QPointF p2 = node.pos;
            QGraphicsLineItem* line = m_scene->addLine(p1.x(), p1.y(), p2.x(), p2.y(), linePen);
            line->setZValue(-1);
        }
    }
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
    m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MainWindow::markSubtreeAsPruned(int nodeId) {
    GameNode &node = m_nodes[nodeId];

    if (node.type == NodeType::LEAF && !m_chkMarkLeaves->isChecked()) {
        return;
    }

    m_animationSteps.append({nodeId, 0, false, 0, 0, true});

    for (int childId : node.children) {
        markSubtreeAsPruned(childId);
    }
}

void MainWindow::startAlgorithm() {
    if (m_animTimer->isActive()) return;

    if (m_animationSteps.isEmpty() || m_currentStep >= m_animationSteps.size()) {
        m_animationSteps.clear();
        m_currentStep = 0;
        m_evaluatedLeaves = 0;

        for(int i = 0; i < m_nodes.size(); ++i) {
            m_items[i]->resetItem();
            if (m_nodes[i].type == NodeType::LEAF) {
                m_items[i]->setState(true, m_nodes[i].value);
            }
        }

        bool rootIsMax = m_chkRootIsMax->isChecked();
        minimax(m_rootIndex, 0, rootIsMax, -1000, 1000);

        qDebug() << "=== Evaluated leaves:" << m_evaluatedLeaves << "of" << 36;
    }

    if(!m_animationSteps.isEmpty()) {
        m_animTimer->start(30);
    }
}

void MainWindow::pauseAnimation() {
    if (m_animTimer->isActive()) {
        m_animTimer->stop();
    }
}

void MainWindow::stopAnimation() {
    m_animTimer->stop();
    m_animationSteps.clear();
    m_currentStep = 0;

    for(int i = 0; i < m_nodes.size(); ++i) {
        m_items[i]->resetItem();
        if (m_nodes[i].type == NodeType::LEAF) {
            m_items[i]->setState(true, m_nodes[i].value);
        }
    }
    m_scene->update();
}

void MainWindow::stepOnce() {
    if (m_animTimer->isActive()) {
        m_animTimer->stop();
    }

    if (m_animationSteps.isEmpty() || m_currentStep >= m_animationSteps.size()) {
        m_animationSteps.clear();
        m_currentStep = 0;
        m_evaluatedLeaves = 0;

        for(int i = 0; i < m_nodes.size(); ++i) {
            m_items[i]->resetItem();
            if (m_nodes[i].type == NodeType::LEAF) {
                m_items[i]->setState(true, m_nodes[i].value);
            }
        }

        bool rootIsMax = m_chkRootIsMax->isChecked();
        minimax(m_rootIndex, 0, rootIsMax, -1000, 1000);

        qDebug() << "=== Evaluated leaves:" << m_evaluatedLeaves << "of" << 36;
    }

    if (m_currentStep < m_animationSteps.size()) {
        stepAnimation();
    }
}

int MainWindow::minimax(int nodeId, int depth, bool isMax, int alpha, int beta) {
    GameNode &node = m_nodes[nodeId];

    if (node.type == NodeType::LEAF) {
        m_evaluatedLeaves++;
        return node.value;
    }

    if (isMax) {
        int maxEval = -1000;
        for (int i = 0; i < node.children.size(); ++i) {
            int childId = node.children[i];
            int eval = minimax(childId, depth + 1, false, alpha, beta);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);

            m_animationSteps.append({nodeId, maxEval, true, alpha, beta, false});

            qDebug() << "MAX node" << nodeId << "(" << node.level << "," << node.indexInLevel << ")"
                     << "child" << childId << "eval" << eval
                     << "alpha" << alpha << "beta" << beta << "prune?" << (beta <= alpha);

            if (beta <= alpha) {
                for (int k = i + 1; k < node.children.size(); ++k) {
                    markSubtreeAsPruned(node.children[k]);
                }
                break;
            }
        }
        return maxEval;
    } else {
        int minEval = 1000;
        for (int i = 0; i < node.children.size(); ++i) {
            int childId = node.children[i];
            int eval = minimax(childId, depth + 1, true, alpha, beta);
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);

            m_animationSteps.append({nodeId, minEval, true, alpha, beta, false});

            qDebug() << "MIN node" << nodeId << "(" << node.level << "," << node.indexInLevel << ")"
                     << "child" << childId << "eval" << eval
                     << "alpha" << alpha << "beta" << beta << "prune?" << (beta <= alpha);

            if (beta <= alpha) {
                for (int k = i + 1; k < node.children.size(); ++k) {
                    markSubtreeAsPruned(node.children[k]);
                }
                break;
            }
        }
        return minEval;
    }
}

void MainWindow::stepAnimation() {
    if (m_currentStep >= m_animationSteps.size()) {
        m_animTimer->stop();

        for (int i = 0; i < m_nodes.size(); ++i) {
            if (m_nodes[i].type != NodeType::LEAF && !m_nodes[i].isEvaluated) {
                m_items[i]->setPruned();
            }
        }
        m_scene->update();
        return;
    }

    AnimStep step = m_animationSteps[m_currentStep];

    if (step.nodeId < 0 || step.nodeId >= m_items.size()) {
        m_currentStep++;
        return;
    }

    NodeItem *item = m_items[step.nodeId];

    if (step.isPruned) {
        item->setPruned();
    } else if (step.isEvaluated) {
        item->setState(true, step.value);
        m_nodes[step.nodeId].value = step.value;
        m_nodes[step.nodeId].isEvaluated = true;
    }

    m_currentStep++;
}