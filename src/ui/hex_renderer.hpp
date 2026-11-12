#pragma once

#include <QQuickPaintedItem>

namespace notestudio::ui {

class HexRenderer final : public QQuickPaintedItem {
    Q_OBJECT
public:
    explicit HexRenderer(QQuickItem* parent = nullptr);
    void paint(QPainter* painter) override;
};

} // namespace notestudio::ui
