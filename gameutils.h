#ifndef GAMEUTILS_H
#define GAMEUTILS_H

#include <QVector>
#include <QPointF>
#include <QString>

enum class NodeType { MAX, MIN, LEAF };

struct GameNode {
    int id;
    int value;
    bool isEvaluated;
    NodeType type;
    QPointF pos;
    QVector<int> children;
    int parent = -1;

    int level = 0;
    int indexInLevel = 0;
};

static const QVector<int> LEAF_VALUES = {
    3, 2, 1, 0, 3, 4, 3, 3, 2, 5, 4, 5,
    6, 5, 6, 5, 4, 2, 1, 5, 6, 1, 0, 2,
    7, 6, 4, 3, 1, 3, 2, 1, 0, 2, 2, 1
};

static constexpr int ALPHA_START = -1000;
static constexpr int BETA_START = 1000;

// Окно поиска вместе с владельцами значений:
// alphaFrom — узел, чьё значение подняло alpha,
// betaFrom  — узел, чьё значение опустило beta.
struct Window {
    int alpha = ALPHA_START;
    int beta = BETA_START;
    int alphaFrom = -1;
    int betaFrom = -1;
};

struct AnimStep {
    int nodeId = -1;
    int value = 0;
    bool isEvaluated = false;
    int alpha = 0;
    int beta = 0;
    bool isPruned = false;

    // Шаг отсечения: сравнение alpha и beta и его участники
    bool isCutoff = false;
    int decisionNode = -1;   // узел, в котором произошло сравнение
    int alphaFrom = -1;      // владелец alpha
    int betaFrom = -1;       // владелец beta
};

#endif // GAMEUTILS_H