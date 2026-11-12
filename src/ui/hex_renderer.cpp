#include "hex_renderer.hpp"

#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace notestudio::ui {
namespace {
constexpr double Pi = 3.14159265358979323846;
constexpr ushort SmuflGClef = 0xE050U;
constexpr ushort SmuflFClef = 0xE062U;
constexpr ushort SmuflFermata = 0xE4C0U;
constexpr ushort SmuflCoda = 0xE048U;
constexpr ushort SmuflNoteheadWhole = 0xE0A2U;
constexpr ushort SmuflNoteheadHalf = 0xE0A3U;
constexpr ushort SmuflNoteheadBlack = 0xE0A4U;
constexpr ushort SmuflRestWhole = 0xE4E3U;
constexpr ushort SmuflRestHalf = 0xE4E4U;
constexpr ushort SmuflRestQuarter = 0xE4E5U;
constexpr ushort SmuflRest8th = 0xE4E6U;

QChar glyph(ushort codepoint) { return QChar(codepoint); }
}

HexRenderer::HexRenderer(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setAntialiasing(true);
    setOpaquePainting(true);
}

void HexRenderer::setHexMode(bool enabled) {
    if (hexMode_ == enabled) return;
    hexMode_ = enabled;
    emit hexModeChanged();
    update();
}

void HexRenderer::setBeats(int value) {
    if (beats_ == value) return;
    beats_ = value;
    emit notationChanged();
    update();
}

void HexRenderer::setBeatUnit(int value) {
    if (beatUnit_ == value) return;
    beatUnit_ = value;
    emit notationChanged();
    update();
}

void HexRenderer::setCoda(bool value) {
    if (coda_ == value) return;
    coda_ = value;
    emit notationChanged();
    update();
}

void HexRenderer::setNotes(const QVariantList& value) {
    notes_ = value;
    emit notationChanged();
    update();
}

void HexRenderer::paint(QPainter* painter) {
    if (painter == nullptr || width() <= 0.0 || height() <= 0.0) return;
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->fillRect(boundingRect(), QColor(QStringLiteral("#081d2b")));
    hexMode_ ? paintHex(painter) : paintStaff(painter);
}

void HexRenderer::paintStaff(QPainter* painter) {
    const qreal staffSpace = std::clamp(height() / 35.0, 9.0, 18.0);
    const qreal left = width() * 0.07;
    const qreal right = width() * 0.94;
    const qreal top = std::max<qreal>(staffSpace, (height() - staffSpace * 15.0) * 0.5);
    const QColor ink(QStringLiteral("#d9f7fa"));
    const QColor lineColor(QStringLiteral("#8ab6c5"));
    painter->setPen(QPen(lineColor, 1.15));
    for (int line = 0; line < 5; ++line) {
        const qreal y = top + static_cast<qreal>(line) * staffSpace;
        painter->drawLine(QPointF(left, y), QPointF(right, y));
        const qreal bassY = top + static_cast<qreal>(line + 7) * staffSpace;
        painter->drawLine(QPointF(left, bassY), QPointF(right, bassY));
    }

    const qreal glyphSize = staffSpace * 4.2;
    QFont musicFont(QStringLiteral("Bravura"));
    musicFont.setPixelSize(static_cast<int>(std::lround(glyphSize)));
    painter->setFont(musicFont);
    painter->setPen(ink);
    const qreal clefBaseline = top + staffSpace * 4.0;
    painter->drawText(QPointF(left + staffSpace * 0.15, clefBaseline), QString(glyph(SmuflGClef)));
    painter->drawText(QPointF(left + staffSpace * 0.15, top + staffSpace * 11.0), QString(glyph(SmuflFClef)));
    painter->setFont(QFont(QStringLiteral("Inter"), static_cast<int>(std::lround(staffSpace * 0.9)), QFont::DemiBold));
    painter->drawText(QPointF(left + staffSpace * 3.2, top + staffSpace * 1.6), QStringLiteral("%1/%2").arg(beats_).arg(beatUnit_));
    painter->setPen(QPen(QColor(QStringLiteral("#355a6c")), 1.0));
    painter->drawLine(QPointF(left + staffSpace * 5.5, top - staffSpace), QPointF(left + staffSpace * 5.5, top + staffSpace * 5.0));

    musicFont.setPixelSize(static_cast<int>(std::lround(staffSpace * 1.9)));
    painter->setFont(musicFont);
    const qreal startX = left + staffSpace * 7.0;
    const qreal usableWidth = std::max<qreal>(staffSpace * 2.0, right - startX - staffSpace * 1.5);
    const qreal gap = notes_.isEmpty() ? staffSpace * 3.0 : std::min(staffSpace * 3.2, usableWidth / static_cast<qreal>(notes_.size()));
    for (qsizetype index = 0; index < notes_.size(); ++index) {
        const QVariantMap note = notes_.at(index).toMap();
        const qreal x = startX + static_cast<qreal>(index) * gap;
        const int denominator = note.value(QStringLiteral("denominator"), 4).toInt();
        const bool isRest = note.value(QStringLiteral("isRest"), false).toBool();
        const int midi = note.value(QStringLiteral("midi"), 60).toInt();
        const bool fermata = note.value(QStringLiteral("fermata"), false).toBool();
        const int articulation = note.value(QStringLiteral("articulation"), 0).toInt();
        const ushort noteGlyph = denominator <= 1 ? SmuflNoteheadWhole : denominator <= 2 ? SmuflNoteheadHalf : SmuflNoteheadBlack;
        const ushort restGlyph = denominator <= 1 ? SmuflRestWhole : denominator <= 2 ? SmuflRestHalf : denominator <= 4 ? SmuflRestQuarter : SmuflRest8th;
        const bool bassRegister = midi < 48;
        const qreal staffBottom = bassRegister ? top + staffSpace * 11.0 : top + staffSpace * 4.0;
        const int referenceMidi = bassRegister ? 43 : 64;
        const qreal baseline = isRest ? top + staffSpace * 2.0 : staffBottom - static_cast<qreal>(midi - referenceMidi) * staffSpace / 3.5;
        if (!isRest) {
            for (int ledger = 0; ledger < 4; ++ledger) {
                const qreal ledgerY = top + staffSpace * 4.0 - static_cast<qreal>(ledger + 1) * staffSpace;
                if (std::abs(baseline - ledgerY) < staffSpace * 0.28 || (baseline > top + staffSpace * 4.0 + staffSpace * 0.4 && ledger == 0)) {
                    painter->setPen(QPen(lineColor, 1.0));
                    painter->drawLine(QPointF(x - staffSpace * 0.75, ledgerY), QPointF(x + staffSpace * 0.75, ledgerY));
                }
            }
        }
        painter->setPen(ink);
        painter->drawText(QPointF(x - staffSpace * 0.55, baseline), QString(glyph(isRest ? restGlyph : noteGlyph)));
        if (denominator >= 8 && !isRest) {
            painter->drawText(QPointF(x + staffSpace * 0.42, baseline - staffSpace), QString(glyph(0xE240U)));
        }
        if (fermata) painter->drawText(QPointF(x - staffSpace * 0.5, baseline - staffSpace * 1.55), QString(glyph(SmuflFermata)));
        if (articulation == 1) painter->drawText(QPointF(x - staffSpace * 0.25, baseline - staffSpace), QString(glyph(0xE4A2U)));
        if (articulation == 2 || articulation == 3) painter->drawText(QPointF(x - staffSpace * 0.25, baseline - staffSpace), QStringLiteral(">"));
        if (index + 1 < notes_.size() && !isRest) {
            const QVariantMap next = notes_.at(index + 1).toMap();
            const qulonglong tie = note.value(QStringLiteral("tie")).toULongLong();
            const qulonglong slur = note.value(QStringLiteral("slur")).toULongLong();
            const bool connected = (tie != 0U && tie == next.value(QStringLiteral("tie")).toULongLong())
                || (slur != 0U && slur == next.value(QStringLiteral("slur")).toULongLong());
            if (connected) {
                QPainterPath curve;
                const qreal nextX = x + gap;
                const qreal curveY = baseline + staffSpace * 0.32;
                curve.moveTo(x, curveY);
                curve.cubicTo(x + gap * 0.30, curveY + staffSpace, nextX - gap * 0.30, curveY + staffSpace, nextX, curveY);
                painter->setPen(QPen(ink, 1.25));
                painter->drawPath(curve);
            }
        }
    }

    if (notes_.isEmpty()) {
        painter->setFont(QFont(QStringLiteral("Inter"), 13));
        painter->setPen(QColor(QStringLiteral("#68899a")));
        painter->drawText(QRectF(left + staffSpace * 6.0, top + staffSpace * 1.0, right - left - staffSpace * 7.0, staffSpace * 2.0), Qt::AlignCenter, QStringLiteral("Choose a note below to begin"));
    }
    painter->setFont(musicFont);
    painter->setPen(ink);
    if (coda_) {
        painter->drawText(QPointF(right - staffSpace * 1.2, top + staffSpace * 1.3), QString(glyph(SmuflCoda)));
    }
}

void HexRenderer::paintHex(QPainter* painter) {
    const QPointF center(width() * 0.5, height() * 0.5);
    const double maxRadius = std::min(width(), height()) * 0.44;
    const QColor aqua(QStringLiteral("#38d9d0"));
    for (int ring = 1; ring <= 6; ++ring) {
        const double radius = maxRadius * static_cast<double>(ring) / 6.0;
        QPolygonF polygon;
        for (int corner = 0; corner < 6; ++corner) {
            const double angle = (static_cast<double>(corner) * 60.0 - 30.0) * Pi / 180.0;
            polygon << QPointF(center.x() + radius * std::cos(angle), center.y() + radius * std::sin(angle));
        }
        QPen pen(QColor(aqua.red(), aqua.green(), aqua.blue(), 220 - ring * 22));
        pen.setWidthF(ring == 6 ? 2.0 : 1.0);
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
    for (qsizetype index = 0; index < notes_.size(); ++index) {
        const qreal angle = static_cast<qreal>(index) * 0.55;
        const qreal radius = static_cast<qreal>(maxRadius) * 0.38;
        painter->drawEllipse(QPointF(center.x() + std::cos(angle) * radius, center.y() + std::sin(angle) * radius), 5.5, 4.0);
    }
}

} // namespace notestudio::ui
