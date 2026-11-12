#pragma once

#include <QQuickPaintedItem>
#include <QVariantList>

namespace notestudio::ui {

class HexRenderer final : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool hexMode READ hexMode WRITE setHexMode NOTIFY hexModeChanged)
    Q_PROPERTY(int beats READ beats WRITE setBeats NOTIFY notationChanged)
    Q_PROPERTY(int beatUnit READ beatUnit WRITE setBeatUnit NOTIFY notationChanged)
    Q_PROPERTY(bool coda READ coda WRITE setCoda NOTIFY notationChanged)
    Q_PROPERTY(QVariantList notes READ notes WRITE setNotes NOTIFY notationChanged)
public:
    explicit HexRenderer(QQuickItem* parent = nullptr);
    [[nodiscard]] bool hexMode() const noexcept { return hexMode_; }
    [[nodiscard]] int beats() const noexcept { return beats_; }
    [[nodiscard]] int beatUnit() const noexcept { return beatUnit_; }
    [[nodiscard]] bool coda() const noexcept { return coda_; }
    [[nodiscard]] QVariantList notes() const { return notes_; }
    void setHexMode(bool enabled);
    void setBeats(int value);
    void setBeatUnit(int value);
    void setCoda(bool value);
    void setNotes(const QVariantList& value);
    void paint(QPainter* painter) override;

signals:
    void hexModeChanged();
    void notationChanged();

private:
    void paintStaff(QPainter* painter);
    void paintHex(QPainter* painter);
    bool hexMode_{false};
    int beats_{4};
    int beatUnit_{4};
    bool coda_{false};
    QVariantList notes_;
};

} // namespace notestudio::ui
