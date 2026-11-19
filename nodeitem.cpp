#include "nodeitem.h"
#include <QFont>

NodeItem::NodeItem(int id, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_id(id)
{
    setCacheMode(DeviceCoordinateCache);
}

void NodeItem::setState(bool evaluated, int value) {
    m_isEvaluated = evaluated;
    m_value = value;
    update();
}

void NodeItem::setPruned() {
    if (m_isPruned) return;
    m_isPruned = true;
    update();
}

void NodeItem::setMark(Mark mark) {
    if (m_mark == mark) return;
    m_mark = mark;
    update();
}

void NodeItem::resetItem() {
    m_isEvaluated = false;
    m_isPruned = false;
    m_mark = Mark::NoMark;
    m_value = 0;
    update();
}

QPointF NodeItem::topAnchor() const {
    // mapToScene, а не просто pos(): если у узла когда-нибудь появится
    // родитель или смещение, якорь останется на верхней грани.
    return mapToScene(QPointF(0.0, -m_size.height() / 2.0));
}

QRectF NodeItem::boundingRect() const {
    // Рамка метки рисуется с отступом 6 px, поэтому bbox чуть больше узла.
    const qreal w = m_size.width();
    const qreal h = m_size.height();
    return QRectF(-w / 2 - 8, -h / 2 - 8, w + 16, h + 16);
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    const QRectF box(-m_size.width() / 2, -m_size.height() / 2,
                     m_size.width(), m_size.height());

    if (m_isEvaluated) {
        painter->setBrush(QBrush(QColor(230, 240, 200)));
        painter->setPen(QPen(Qt::black, 2));
        painter->drawRect(box);

        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setBold(true);
#ifdef Q_OS_ANDROID
        font.setPointSize(26);
#else
        font.setPointSize(20);
#endif
        painter->setFont(font);
        painter->drawText(box, Qt::AlignCenter, QString::number(m_value));
    } else {
        painter->setBrush(QBrush(QColor(245, 245, 220)));
        painter->setPen(QPen(Qt::darkGreen, 2));
        painter->drawEllipse(box);
    }

    if (m_isPruned) {
        painter->setPen(QPen(Qt::red, 3));
        painter->drawLine(QPointF(-m_size.width() / 2, -m_size.height() / 2),
                          QPointF(m_size.width() / 2, m_size.height() / 2));
        painter->drawLine(QPointF(m_size.width() / 2, -m_size.height() / 2),
                          QPointF(-m_size.width() / 2, m_size.height() / 2));
    }

    if (m_mark != Mark::NoMark) {
        QColor color;
        switch (m_mark) {
        case Mark::AlphaOwner: color = QColor(25, 90, 210);  break; // владелец alpha
        case Mark::BetaOwner:  color = QColor(200, 40, 40);  break; // владелец beta
        case Mark::Decision:   color = QColor(235, 140, 0);  break; // узел-решение
        case Mark::NoMark:     return;
        }
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(color, 3));
        painter->drawRect(box.adjusted(-6, -6, 6, 6));
    }
}