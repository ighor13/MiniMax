#include "nodeitem.h"
#include <QFont>
#include <QDebug>

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

void NodeItem::setPruned(const QString& reason) {
    m_isPruned = true;
    if (!reason.isEmpty()) {
        m_pruneReason = reason;
    }
    update();
}

void NodeItem::resetItem() {
    m_isEvaluated = false;
    m_isPruned = false;
    m_pruneReason = "";
    m_value = 0;
    update();
}

QRectF NodeItem::boundingRect() const {
    return QRectF(-m_size.width()/2, -m_size.height()/2, m_size.width(), m_size.height() + 40);
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    if (m_isEvaluated) {
        painter->setBrush(QBrush(QColor(230, 240, 200)));
        painter->setPen(QPen(Qt::black, 2));
        painter->drawRect(QRectF(-m_size.width()/2, -m_size.height()/2, m_size.width(), m_size.height()));

        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setBold(true);
#ifdef Q_OS_ANDROID
        font.setPointSize(26);
#else
        font.setPointSize(20);
#endif
        painter->setFont(font);
        painter->drawText(QRectF(-m_size.width()/2, -m_size.height()/2, m_size.width(), m_size.height()), Qt::AlignCenter, QString::number(m_value));
    } else {
        painter->setBrush(QBrush(QColor(245, 245, 220)));
        painter->setPen(QPen(Qt::darkGreen, 2));
        painter->drawEllipse(QRectF(-m_size.width()/2, -m_size.height()/2, m_size.width(), m_size.height()));
    }

    if (m_isPruned) {
        painter->setPen(QPen(Qt::red, 3));
        painter->drawLine(QPointF(-m_size.width()/2, -m_size.height()/2), QPointF(m_size.width()/2, m_size.height()/2));
        painter->drawLine(QPointF(m_size.width()/2, -m_size.height()/2), QPointF(-m_size.width()/2, m_size.height()/2));

        if (!m_pruneReason.isEmpty()) {
            QFont reasonFont = painter->font();
            reasonFont.setPointSize(14);
            reasonFont.setBold(true);
            painter->setFont(reasonFont);
            painter->setPen(Qt::blue);

            QRectF textRect(-60, m_size.height()/2 + 10, 120, 30);
            painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, m_pruneReason);
        }
    }
}