#pragma once

#include <QQuickPaintedItem>
#include <QVariantList>
#include <QMouseEvent>

namespace notestudio::ui {

class HexRenderer : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(bool hexMode READ hexMode WRITE setHexMode NOTIFY hexModeChanged)
    Q_PROPERTY(int notationMode READ notationMode WRITE setNotationMode NOTIFY notationModeChanged)
    Q_PROPERTY(int beats READ beats WRITE setBeats NOTIFY notationChanged)
    Q_PROPERTY(int beatUnit READ beatUnit WRITE setBeatUnit NOTIFY notationChanged)
    Q_PROPERTY(bool coda READ coda WRITE setCoda NOTIFY notationChanged)
    Q_PROPERTY(bool segno READ segno WRITE setSegno NOTIFY notationChanged)
    Q_PROPERTY(int tempoBpm READ tempoBpm WRITE setTempoBpm NOTIFY notationChanged)
    Q_PROPERTY(int dynamicMark READ dynamicMark WRITE setDynamicMark NOTIFY notationChanged)
    Q_PROPERTY(int dynamicWedge READ dynamicWedge WRITE setDynamicWedge NOTIFY notationChanged)
    Q_PROPERTY(QVariantList notes READ notes WRITE setNotes NOTIFY notationChanged)
public:
    explicit HexRenderer(QQuickItem* parent = nullptr);
    [[nodiscard]] bool hexMode() const noexcept { return hexMode_; }
    [[nodiscard]] int notationMode() const noexcept { return notationMode_; }
    [[nodiscard]] int beats() const noexcept { return beats_; }
    [[nodiscard]] int beatUnit() const noexcept { return beatUnit_; }
    [[nodiscard]] bool coda() const noexcept { return coda_; }
    [[nodiscard]] bool segno() const noexcept { return segno_; }
    [[nodiscard]] int tempoBpm() const noexcept { return tempoBpm_; }
    [[nodiscard]] int dynamicMark() const noexcept { return dynamicMark_; }
    [[nodiscard]] int dynamicWedge() const noexcept { return dynamicWedge_; }
    [[nodiscard]] QVariantList notes() const { return notes_; }
    void setHexMode(bool enabled);
    void setNotationMode(int mode);
    void setBeats(int value);
    void setBeatUnit(int value);
    void setCoda(bool value);
    void setSegno(bool value);
    void setTempoBpm(int value);
    void setDynamicMark(int value);
    void setDynamicWedge(int value);
    void setNotes(const QVariantList& value);
    void paint(QPainter* painter) override;
protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void hexModeChanged();
    void notationModeChanged();
    void notationChanged();
    void noteSelected(int index);

private:
    void paintStaff(QPainter* painter);
    void paintHex(QPainter* painter);
    void paintJianpu(QPainter* painter, bool kanji);
    bool hexMode_{false};
    int notationMode_{0};
    int beats_{4};
    int beatUnit_{4};
    bool coda_{false};
    bool segno_{false};
    int tempoBpm_{120};
    int dynamicMark_{4};
    int dynamicWedge_{0};
    QVariantList notes_;
};

} // namespace notestudio::ui
