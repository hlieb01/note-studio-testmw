#include "hex_renderer.hpp"

#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QVector2D>

#include <cmath>

namespace notestudio::ui {

HexRenderer::HexRenderer(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
}

void HexRenderer::paint(QPainter* painter) {
    if (painter == nullptr || width() <= 0.0 || height() <= 0.0) {
        return;
    }
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(boundingRect(), QColor(QStringLiteral("#071b2b")));
    const QPointF center(width() * 0.5, height() * 0.5);
    const double maxRadius = std::min(width(), height()) * 0.44;
    const QColor aqua(QStringLiteral("#38d9d0"));

    for (int ring = 1; ring <= 6; ++ring) {
        const double radius = maxRadius * static_cast<double>(ring) / 6.0;
        QPolygonF polygon;
        for (int corner = 0; corner < 6; ++corner) {
            const double angle = (static_cast<double>(corner) * 60.0 - 30.0) * 3.14159265358979323846 / 180.0;
            polygon << QPointF(center.x() + radius * std::cos(angle), center.y() + radius * std::sin(angle));
        }
        QPen pen(aqua);
        pen.setWidthF(ring == 6 ? 2.0 : 1.0);
        pen.setColor(QColor(aqua.red(), aqua.green(), aqua.blue(), 220 - ring * 22));
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPolygon(polygon);
    }

    painter->setPen(QPen(QColor(QStringLiteral("#75a9be")), 1.0));
    for (int line = 0; line < 5; ++line) {
        const qreal y = height() * 0.25 + static_cast<qreal>(line) * height() * 0.075;
        painter->drawLine(QPointF(width() * 0.16, y), QPointF(width() * 0.84, y));
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(QStringLiteral("#b9fff5")));
    painter->drawEllipse(QPointF(width() * 0.5, height() * 0.4), 5.5, 4.0);
}

} // namespace notestudio::ui
