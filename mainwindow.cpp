#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include <QFontMetricsF>
#include <QStringList>
#include <QDebug>
#include <algorithm>

// Линия «alpha <-> beta» с подписью сравнения в середине.
// Определение на глобальном уровне: иначе это будет другой тип,
// чем форвард-объявление в mainwindow.h (ошибка C2872).
class CutoffLinkItem : public QGraphicsItem
{
public:
    CutoffLinkItem(const QPointF &from, const QPointF &to, const QString &label)
        : m_from(from), m_to(to), m_label(label)
    {
        setZValue(-0.5);   // выше рёбер дерева (-1), ниже узлов (0)
        setAcceptedMouseButtons(Qt::NoButton);
    }

    ~CutoffLinkItem() override = default;

    QRectF boundingRect() const override
    {
        const QRectF r = QRectF(m_from, m_to).normalized();
        return r.adjusted(-200.0, -80.0, 200.0, 80.0);
    }

    void setEmphasized(bool on)
    {
        if (m_emphasized == on) return;
        m_emphasized = on;
        update();
    }

    void setLabelVisible(bool on)
    {
        if (m_labelVisible == on) return;
        m_labelVisible = on;
        update();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setRenderHint(QPainter::Antialiasing);

        // Голубой в обоих состояниях, меняется только насыщенность,
        // чтобы линии не сливались с серыми рёбрами дерева.
        const QColor lineColor = m_emphasized ? QColor(20, 110, 220) : QColor(120, 175, 235);
        painter->setPen(QPen(lineColor, m_emphasized ? 2.5 : 1.6, Qt::DashLine, Qt::RoundCap));
        painter->setBrush(Qt::NoBrush);
        painter->drawLine(m_from, m_to);

        if (!m_labelVisible)
            return;

        QFont font = painter->font();
        font.setBold(m_emphasized);
        font.setPointSizeF(m_emphasized ? 14.0 : 12.0);
        painter->setFont(font);

        const QFontMetricsF fm(font);
        const QSizeF textSize = fm.size(Qt::TextSingleLine, m_label);

        const QPointF mid = (m_from + m_to) / 2.0;
        QRectF plate(QPointF(0, 0), textSize + QSizeF(16.0, 10.0));
        plate.moveCenter(QPointF(mid.x(), mid.y() - 22.0));

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(255, 255, 255, m_emphasized ? 235 : 190));
        painter->drawRoundedRect(plate, 4.0, 4.0);

        painter->setPen(m_emphasized ? QColor(15, 85, 190) : QColor(95, 150, 215));
        painter->drawText(plate, Qt::AlignCenter, m_label);
    }

private:
    QPointF m_from;
    QPointF m_to;
    QString m_label;
    bool m_emphasized = true;
    bool m_labelVisible = true;
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUi();
    buildTree();

    QTimer::singleShot(0, this, [this]() {
        m_view->fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    });
}

MainWindow::~MainWindow() {
    // Ничего не удаляем вручную: m_scene, m_animTimer и central созданы
    // с родителем this, а m_view живёт внутри layout центрального виджета.
    if (m_animTimer) {
        m_animTimer->stop();
    }

    // Отцепляем вид от сцены до её разрушения, чтобы QGraphicsView
    // не обращался к уже удалённым элементам.
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
    });

    m_chkMarkLeaves = new QCheckBox("Метить листья");
    m_chkMarkLeaves->setChecked(false);

    m_chkShowReasons = new QCheckBox("Условия отсечения");
    m_chkShowReasons->setChecked(true);
    connect(m_chkShowReasons, &QCheckBox::toggled, this, &MainWindow::updateCutoffLabels);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addWidget(m_btnStart);
    topLayout->addWidget(m_btnPause);
    topLayout->addWidget(m_btnStop);
    topLayout->addWidget(m_btnStep);
    topLayout->addWidget(m_chkRootIsMax);
    topLayout->addWidget(m_chkMarkLeaves);
    topLayout->addWidget(m_chkShowReasons);
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
    // Таймер продолжал тикать с уже очищенной очередью шагов и на следующем
    // тике красил все неоценённые узлы крестами.
    m_animTimer->stop();

    // Сначала убираем линии, пока m_items ещё валиден, потом чистим сцену.
    clearCutoffVisuals();

    m_animationSteps.clear();
    m_currentStep = 0;
    m_realCutoffs = 0;
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

    for (const auto& node : m_nodes) {
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

    // Рёбра рисуем здесь же, чтобы не вызывать drawConnections() дважды.
    drawConnections();
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

    if (node.id < m_items.size()) {
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

void MainWindow::resetAllNodes() {
    for (int i = 0; i < m_nodes.size(); ++i) {
        m_items[i]->resetItem();
        if (m_nodes[i].type == NodeType::LEAF) {
            m_items[i]->setState(true, m_nodes[i].value);
        } else {
            // Значения внутренних узлов от прошлого прогона больше не нужны.
            m_nodes[i].isEvaluated = false;
            m_nodes[i].value = 0;
        }
    }
}

void MainWindow::prepareSteps() {
    m_animationSteps.clear();
    m_currentStep = 0;
    m_evaluatedLeaves = 0;
    m_realCutoffs = 0;

    clearCutoffVisuals();
    resetAllNodes();

    Window window;
    minimax(m_rootIndex, m_chkRootIsMax->isChecked(), window);

    qDebug() << "=== Evaluated leaves:" << m_evaluatedLeaves << "of" << LEAF_VALUES.size()
             << " | real cutoffs:" << m_realCutoffs;
}

int MainWindow::markSubtreeAsPruned(int nodeId) {
    GameNode &node = m_nodes[nodeId];

    // Лист, который не метим, не считается отсечённым — иначе событие
    // отсечения появится там, где на экране ничего не перечёркнуто.
    if (node.type == NodeType::LEAF && !m_chkMarkLeaves->isChecked()) {
        return 0;
    }

    AnimStep step;
    step.nodeId = nodeId;
    step.isPruned = true;
    m_animationSteps.append(step);

    int count = 1;
    for (int childId : node.children) {
        count += markSubtreeAsPruned(childId);
    }
    return count;
}

void MainWindow::appendEvalStep(int nodeId, int value, const Window& window) {
    AnimStep step;
    step.nodeId = nodeId;
    step.value = value;
    step.isEvaluated = true;
    step.alpha = window.alpha;
    step.beta = window.beta;
    m_animationSteps.append(step);
}

AnimStep MainWindow::makeCutoffStep(int decisionNode, const Window& window) const {
    AnimStep step;
    step.isCutoff = true;
    step.nodeId = decisionNode;
    step.decisionNode = decisionNode;
    step.alpha = window.alpha;
    step.beta = window.beta;
    step.alphaFrom = window.alphaFrom;
    step.betaFrom = window.betaFrom;
    return step;
}

void MainWindow::startAlgorithm() {
    if (m_animTimer->isActive()) return;

    if (m_animationSteps.isEmpty() || m_currentStep >= m_animationSteps.size()) {
        prepareSteps();
    }

    if (!m_animationSteps.isEmpty()) {
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

    clearCutoffVisuals();
    resetAllNodes();
    m_scene->update();
}

void MainWindow::stepOnce() {
    if (m_animTimer->isActive()) {
        m_animTimer->stop();
    }

    if (m_animationSteps.isEmpty() || m_currentStep >= m_animationSteps.size()) {
        prepareSteps();
    }

    if (m_currentStep < m_animationSteps.size()) {
        stepAnimation();
    }
}

int MainWindow::minimax(int nodeId, bool isMax, Window window) {
    GameNode &node = m_nodes[nodeId];

    if (node.type == NodeType::LEAF) {
        m_evaluatedLeaves++;
        return node.value;
    }

    if (isMax) {
        int best = ALPHA_START;
        for (int i = 0; i < node.children.size(); ++i) {
            const int childId = node.children[i];
            const int eval = minimax(childId, false, window);

            best = std::max(best, eval);
            if (eval > window.alpha) {
                window.alpha = eval;
                window.alphaFrom = childId;   // alpha подняла именно эта ветвь
            }

            appendEvalStep(nodeId, best, window);

            if (window.beta <= window.alpha) {
                // Сначала помечаем, потом решаем, было ли отсечение на самом деле.
                const int cutoffIndex = m_animationSteps.size();
                int marked = 0;
                for (int k = i + 1; k < node.children.size(); ++k) {
                    marked += markSubtreeAsPruned(node.children[k]);
                }
                if (marked > 0) {
                    m_animationSteps.insert(cutoffIndex, makeCutoffStep(nodeId, window));
                    m_realCutoffs++;
                }
                break;
            }
        }
        return best;
    } else {
        int best = BETA_START;
        for (int i = 0; i < node.children.size(); ++i) {
            const int childId = node.children[i];
            const int eval = minimax(childId, true, window);

            best = std::min(best, eval);
            if (eval < window.beta) {
                window.beta = eval;
                window.betaFrom = childId;    // beta опустила именно эта ветвь
            }

            appendEvalStep(nodeId, best, window);

            if (window.beta <= window.alpha) {
                const int cutoffIndex = m_animationSteps.size();
                int marked = 0;
                for (int k = i + 1; k < node.children.size(); ++k) {
                    marked += markSubtreeAsPruned(node.children[k]);
                }
                if (marked > 0) {
                    m_animationSteps.insert(cutoffIndex, makeCutoffStep(nodeId, window));
                    m_realCutoffs++;
                }
                break;
            }
        }
        return best;
    }
}

void MainWindow::applyCutoffVisual(const AnimStep& step) {
    // Все предыдущие отсечения приглушаются, текущее будет ярким.
    for (CutoffLinkItem *link : m_cutoffItems) {
        link->setEmphasized(false);
    }

    // Снимаем метки с узлов предыдущего отсечения.
    for (int id : m_markedNodes) {
        if (id >= 0 && id < m_items.size()) {
            m_items[id]->setMark(NodeItem::Mark::NoMark);
        }
    }
    m_markedNodes.clear();

    if (step.decisionNode >= 0 && step.decisionNode < m_items.size()) {
        m_items[step.decisionNode]->setMark(NodeItem::Mark::Decision);
        m_markedNodes.append(step.decisionNode);
    }
    if (step.alphaFrom >= 0 && step.alphaFrom < m_items.size()) {
        m_items[step.alphaFrom]->setMark(NodeItem::Mark::AlphaOwner);
        m_markedNodes.append(step.alphaFrom);
    }
    if (step.betaFrom >= 0 && step.betaFrom < m_items.size()) {
        m_items[step.betaFrom]->setMark(NodeItem::Mark::BetaOwner);
        m_markedNodes.append(step.betaFrom);
    }

    // Линия нужна только когда известны оба участника.
    if (step.alphaFrom < 0 || step.betaFrom < 0 || step.alphaFrom == step.betaFrom) {
        qDebug() << "cutoff: нет полной пары alpha/beta" << step.alphaFrom << step.betaFrom;
        m_scene->update();
        return;
    }

    const QString label = QString("β=%1 ≤ α=%2  →  отрезано")
                              .arg(step.beta)
                              .arg(step.alpha);

    // Концы линии — центры верхних граней узлов, а не центры квадратов:
    // иначе линия уходит под узел и непонятно, к чему она относится.
    CutoffLinkItem *link = new CutoffLinkItem(m_items[step.alphaFrom]->topAnchor(),
                                              m_items[step.betaFrom]->topAnchor(),
                                              label);
    link->setLabelVisible(m_chkShowReasons->isChecked());
    m_scene->addItem(link);
    m_cutoffItems.append(link);

    m_scene->update();
}

void MainWindow::clearCutoffVisuals() {
    for (CutoffLinkItem *link : m_cutoffItems) {
        if (!link) continue;
        m_scene->removeItem(link);
        delete link;
    }
    m_cutoffItems.clear();

    for (int id : m_markedNodes) {
        if (id >= 0 && id < m_items.size()) {
            m_items[id]->setMark(NodeItem::Mark::NoMark);
        }
    }
    m_markedNodes.clear();
}

void MainWindow::updateCutoffLabels() {
    const bool visible = m_chkShowReasons->isChecked();
    for (CutoffLinkItem *link : m_cutoffItems) {
        link->setLabelVisible(visible);
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

    const AnimStep step = m_animationSteps[m_currentStep];

    if (step.isCutoff) {
        applyCutoffVisual(step);
        m_currentStep++;
        return;
    }

    if (step.nodeId >= 0 && step.nodeId < m_items.size()) {
        NodeItem *item = m_items[step.nodeId];

        if (step.isPruned) {
            item->setPruned();
        } else if (step.isEvaluated) {
            item->setState(true, step.value);
            m_nodes[step.nodeId].value = step.value;
            m_nodes[step.nodeId].isEvaluated = true;
        }
    }

    m_currentStep++;
}