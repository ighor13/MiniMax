#include "nodeitem.h"
#include <QFont>

NodeItem::NodeItem(int id, QGraphicsItem *parent)
    : QGraphicsItem(parent), m_id(id)
{
    // Включаем кэширование для производительности
    setCacheMode(DeviceCoordinateCache);
}

void NodeItem::setState(bool evaluated, int value) {
    m_isEvaluated = evaluated;
    m_value = value;
    update();
}

void NodeItem::setPruned() {
    m_isPruned = true;
    update();
}

void NodeItem::resetItem() {
    m_isEvaluated = false;
    m_isPruned = false;
    m_value = 0;
    update();
}

QRectF NodeItem::boundingRect() const {
    // Возвращаем прямоугольник, центрированный относительно (0,0)
    return QRectF(-m_size.width()/2, -m_size.height()/2, m_size.width(), m_size.height());
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    if (m_isEvaluated) {
        // --- РИСУЕМ КВАДРАТ (ВЫЧИСЛЕННЫЙ УЗЕЛ) ---
        painter->setBrush(QBrush(QColor(230, 240, 200))); // Светло-зеленоватый
        painter->setPen(QPen(Qt::black, 2));
        painter->drawRect(boundingRect());

        // Настраиваем шрифт
        painter->setPen(Qt::black);
        QFont font = painter->font();
        font.setBold(true);

#ifdef Q_OS_ANDROID
        font.setPointSize(26);
#else
        font.setPointSize(20);
#endif

        painter->setFont(font);

        // Рисуем текст по центру
        painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(m_value));

    } else {
        // --- РИСУЕМ КРУГ (НЕРАССЧИТАННЫЙ УЗЕЛ) ---
        painter->setBrush(QBrush(QColor(245, 245, 220))); // Бежевый
        painter->setPen(QPen(Qt::darkGreen, 2));
        painter->drawEllipse(boundingRect());
    }

    // --- РИСУЕМ КРАСНЫЙ КРЕСТ (ОТСЕЧЕННЫЙ УЗЕЛ) ---
    if (m_isPruned) {
        painter->setPen(QPen(Qt::red, 3)); // Толстый красный крест
        painter->drawLine(boundingRect().topLeft(), boundingRect().bottomRight());
        painter->drawLine(boundingRect().topRight(), boundingRect().bottomLeft());
    }
}