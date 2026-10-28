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

struct AnimStep {
    int nodeId;
    int value;
    bool isEvaluated;
    int alpha;
    int beta;
    bool isPruned;
    QString pruneReason = "";
};

#endif // GAMEUTILS_H