#ifndef NODEITEM_H
#define NODEITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include "gameutils.h"

class NodeItem : public QGraphicsItem {
public:
    NodeItem(int id, QGraphicsItem *parent = nullptr);

    void setState(bool evaluated, int value);
    void setPruned(const QString& reason = "");
    void resetItem();

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int nodeId() const { return m_id; }

private:
    int m_id;
    bool m_isEvaluated = false;
    bool m_isPruned = false;
    int m_value = 0;
    QString m_pruneReason = "";
    QSizeF m_size = QSizeF(60, 60);
};

#endif // NODEITEM_H