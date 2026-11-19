#ifndef NODEITEM_H
#define NODEITEM_H

#include <QGraphicsItem>
#include <QPainter>
#include "gameutils.h"

class NodeItem : public QGraphicsItem {
public:
    // NoMark, а не None: None — макрос X11, ломает сборку на Linux.
    enum class Mark { NoMark, AlphaOwner, BetaOwner, Decision };

    NodeItem(int id, QGraphicsItem *parent = nullptr);
    ~NodeItem() override = default;

    void setState(bool evaluated, int value);
    void setPruned();
    void setMark(Mark mark);
    void resetItem();

    // Точка в центре верхней грани узла в координатах сцены.
    // Нужна, чтобы линии отсечений не уходили под квадрат.
    QPointF topAnchor() const;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    int nodeId() const { return m_id; }

private:
    int m_id;
    bool m_isEvaluated = false;
    bool m_isPruned = false;
    Mark m_mark = Mark::NoMark;
    int m_value = 0;
    QSizeF m_size = QSizeF(60, 60);
};

#endif // NODEITEM_H