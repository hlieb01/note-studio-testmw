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
constexpr ushort SmuflSegno = 0xE047U;
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
    setAcceptedMouseButtons(Qt::LeftButton);
}

void HexRenderer::mousePressEvent(QMouseEvent* event) {
    if (event == nullptr || notes_.isEmpty()) return;
    const qreal normalized = std::clamp(event->position().x() / std::max<qreal>(1.0, width()), 0.0, 0.999999);
    const int noteCount = static_cast<int>(notes_.size());
    const int index = std::clamp(static_cast<int>(normalized * static_cast<qreal>(noteCount)), 0, noteCount - 1);
    emit noteSelected(index);
    event->accept();
}

void HexRenderer::setHexMode(bool enabled) {
    setNotationMode(enabled ? 1 : 0);
}

void HexRenderer::setNotationMode(int mode) {
    const int bounded = std::clamp(mode, 0, 3);
    if (notationMode_ == bounded) return;
    const bool wasHex = hexMode_;
    notationMode_ = bounded;
    hexMode_ = bounded == 1;
    emit notationModeChanged();
    if (wasHex != hexMode_) emit hexModeChanged();
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

void HexRenderer::setSegno(bool value) {
    if (segno_ == value) return;
    segno_ = value;
    emit notationChanged();
    update();
}

void HexRenderer::setTempoBpm(int value) {
    if (tempoBpm_ == value) return;
    tempoBpm_ = value;
    emit notationChanged();
    update();
}

void HexRenderer::setDynamicMark(int value) {
    if (dynamicMark_ == value) return;
    dynamicMark_ = std::clamp(value, 0, 8);
    emit notationChanged();
    update();
}

void HexRenderer::setDynamicWedge(int value) {
    if (dynamicWedge_ == value) return;
    dynamicWedge_ = std::clamp(value, 0, 4);
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
    if (notationMode_ == 1) paintHex(painter);
    else if (notationMode_ == 2) paintJianpu(painter, false);
    else if (notationMode_ == 3) paintJianpu(painter, true);
    else paintStaff(painter);
}

void HexRenderer::paintJianpu(QPainter* painter, bool kanji) {
    const QColor ink(QStringLiteral("#d9f7fa"));
    const qreal baseline = height() * 0.53;
    painter->setPen(QPen(QColor(QStringLiteral("#355a6c")), 1.0));
    painter->drawLine(QPointF(width() * 0.07, baseline), QPointF(width() * 0.94, baseline));
    QFont font(kanji ? QStringLiteral("Noto Sans CJK JP") : QStringLiteral("Inter"));
    font.setPixelSize(static_cast<int>(std::clamp(height() * 0.18, 28.0, 78.0)));
    painter->setFont(font);
    painter->setPen(ink);
    static const QStringList jianpu{QStringLiteral("壱"), QStringLiteral("弐"), QStringLiteral("参"), QStringLiteral("四"),
                                    QStringLiteral("五"), QStringLiteral("六"), QStringLiteral("七")};
    if (notes_.isEmpty()) {
        painter->setFont(QFont(QStringLiteral("Inter"), 14));
        painter->setPen(QColor(QStringLiteral("#68899a")));
        painter->drawText(boundingRect(), Qt::AlignCenter, QStringLiteral("Choose a note below to begin"));
        return;
    }
    const qreal left = width() * 0.12;
    const qreal usableWidth = width() * 0.78;
    const qreal gap = usableWidth / static_cast<qreal>(std::max<qsizetype>(1, notes_.size()));
    for (qsizetype index = 0; index < notes_.size(); ++index) {
        const QVariantMap event = notes_.at(index).toMap();
        const bool isRest = event.value(QStringLiteral("isRest")).toBool();
        const int midi = event.value(QStringLiteral("midi"), 60).toInt();
        const int semitone = ((midi % 12) + 12) % 12;
        static constexpr int degrees[] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
        const int degree = degrees[semitone];
        const qreal x = left + (static_cast<qreal>(index) + 0.5) * gap;
        QString label = isRest ? QStringLiteral("0") : kanji ? jianpu.at(degree) : QString::number(degree + 1);
        if (!isRest) {
            const int octave = midi / 12 - 5;
            const int dotCount = std::min(2, std::abs(octave));
            for (int dot = 0; dot < dotCount; ++dot) {
                const qreal dotY = baseline + (octave > 0 ? -font.pixelSize() * 0.65 : font.pixelSize() * 0.45);
                painter->setBrush(ink);
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(QPointF(x + (static_cast<qreal>(dot) - 0.5) * font.pixelSize() * 0.25, dotY), 2.2, 2.2);
            }
        }
        painter->setPen(ink);
        painter->drawText(QRectF(x - gap * 0.45, baseline - font.pixelSize(), gap * 0.9, font.pixelSize() * 1.3), Qt::AlignCenter, label);
        const int denominator = event.value(QStringLiteral("denominator"), 4).toInt();
        const int dots = event.value(QStringLiteral("dots"), 0).toInt();
        if (denominator > 4) {
            painter->setFont(QFont(QStringLiteral("Inter"), 10));
            painter->drawText(QPointF(x + font.pixelSize() * 0.35, baseline + font.pixelSize() * 0.25), QString::number(denominator));
            painter->setFont(font);
        }
        for (int dot = 0; dot < dots; ++dot) painter->drawEllipse(QPointF(x + font.pixelSize() * 0.32 + dot * 6.0, baseline - 3.0), 2.0, 2.0);
    }
    if (coda_) {
        painter->setFont(QFont(QStringLiteral("Bravura"), 30));
        painter->drawText(QPointF(width() * 0.92, height() * 0.12), QString(glyph(SmuflCoda)));
    }
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
    painter->drawText(QPointF(left + staffSpace * 3.2, top - staffSpace * 0.2), QStringLiteral("♩ = %1").arg(tempoBpm_));
    static const QString dynamics[] = {QStringLiteral("ppp"),QStringLiteral("pp"),QStringLiteral("p"),QStringLiteral("mp"),QStringLiteral("mf"),QStringLiteral("f"),QStringLiteral("ff"),QStringLiteral("fff"),QStringLiteral("n")};
    painter->drawText(QPointF(right - staffSpace * 4.0, top - staffSpace * 0.2), dynamics[static_cast<std::size_t>(std::clamp(dynamicMark_, 0, 8))]);
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
        const int fermataType = note.value(QStringLiteral("fermataType"), 0).toInt();
        const int accidental = note.value(QStringLiteral("accidental"), 0).toInt();
        const int dots = note.value(QStringLiteral("dots"), 0).toInt();
        const int articulation = note.value(QStringLiteral("articulation"), 0).toInt();
        const bool selected = note.value(QStringLiteral("selected")).toBool();
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
        if (selected) {
            painter->setPen(QPen(QColor(QStringLiteral("#ffcc66")), 2.0));
            painter->setBrush(QColor(255, 204, 102, 45));
            painter->drawEllipse(QPointF(x, baseline), staffSpace * 1.05, staffSpace * 0.8);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(ink);
        }
        if (!isRest && accidental != 0) {
            const ushort accidentalGlyph = accidental == -1 ? 0xE260U : accidental == 1 ? 0xE262U :
                accidental == -2 ? 0xE264U : accidental == 2 ? 0xE263U : accidental < 0 ? 0xE264U : 0xE263U;
            const int repetitions = std::abs(accidental) == 3 ? 2 : 1;
            for (int sign = 0; sign < repetitions; ++sign) {
                const qreal accidentalX = x - staffSpace * (1.25 + static_cast<qreal>(sign) * 0.48);
                const ushort glyphCode = repetitions == 2 && sign == 1 ? (accidental < 0 ? 0xE260U : 0xE262U) : accidentalGlyph;
                painter->drawText(QPointF(accidentalX, baseline), QString(glyph(glyphCode)));
            }
        }
        painter->drawText(QPointF(x - staffSpace * 0.55, baseline), QString(glyph(isRest ? restGlyph : noteGlyph)));
        for (int dot = 0; dot < dots; ++dot) {
            painter->setBrush(ink);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(x + staffSpace * (0.55 + static_cast<qreal>(dot) * 0.24), baseline - staffSpace * 0.22), 1.8, 1.8);
            painter->setPen(ink);
        }
        if (denominator >= 8 && !isRest) {
            painter->drawText(QPointF(x + staffSpace * 0.42, baseline - staffSpace), QString(glyph(0xE240U)));
        }
        if (fermata && fermataType == 4) {
            QPolygonF triangle; triangle << QPointF(x, baseline - staffSpace * 2.0) << QPointF(x - staffSpace * 0.35, baseline - staffSpace * 1.45) << QPointF(x + staffSpace * 0.35, baseline - staffSpace * 1.45);
            painter->setBrush(ink); painter->setPen(QPen(ink, 1.0)); painter->drawPolygon(triangle);
        } else if (fermata && fermataType == 5) {
            painter->setBrush(Qt::NoBrush); painter->setPen(QPen(ink, 1.5)); painter->drawRect(QRectF(x - staffSpace * 0.28, baseline - staffSpace * 1.9, staffSpace * 0.56, staffSpace * 0.56));
        } else if (fermata) painter->drawText(QPointF(x - staffSpace * 0.5, baseline - staffSpace * 1.55), QString(glyph(SmuflFermata)));
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
    if (segno_) painter->drawText(QPointF(right - staffSpace * 3.0, top + staffSpace * 1.3), QString(glyph(SmuflSegno)));
    if (dynamicWedge_ != 0) {
        const qreal x1 = left + staffSpace * 7.0; const qreal x2 = right - staffSpace * 2.0;
        const qreal midY = top + staffSpace * 13.2; const qreal spread = staffSpace * 0.5;
        const bool crescendo = dynamicWedge_ == 1 || dynamicWedge_ == 3;
        const qreal firstSpread = crescendo ? 0.0 : spread;
        const qreal lastSpread = crescendo ? spread : 0.0;
        painter->setPen(QPen(ink, 1.2));
        painter->drawLine(QPointF(x1, midY - firstSpread), QPointF(x2, midY - lastSpread));
        painter->drawLine(QPointF(x1, midY + firstSpread), QPointF(x2, midY + lastSpread));
        if (dynamicWedge_ == 3 || dynamicWedge_ == 4) {
            const qreal ringX = dynamicWedge_ == 3 ? x1 : x2;
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(QPointF(ringX, midY), staffSpace * 0.16, staffSpace * 0.16);
        }
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
    painter->setPen(QPen(QColor(QStringLiteral("#b9fff5")), 1.5));
    for (qsizetype index = 0; index < notes_.size(); ++index) {
        const QVariantMap event = notes_.at(index).toMap();
        if (event.value(QStringLiteral("isRest")).toBool()) continue;
        const int midi = event.value(QStringLiteral("midi"), 60).toInt();
        const double angle = static_cast<double>((midi % 12 + 12) % 12) * Pi / 6.0 - Pi / 2.0;
        const double octave = static_cast<double>(std::clamp(midi / 12 - 1, 0, 8));
        const double radius = maxRadius * (0.12 + octave * 0.075);
        const QPointF point(center.x() + std::cos(angle) * radius, center.y() + std::sin(angle) * radius);
        const double dotRadius = 3.0 + std::min(4, event.value(QStringLiteral("denominator"), 4).toInt() / 16);
        painter->setBrush(QColor(QStringLiteral("#b9fff5")));
        painter->drawEllipse(point, dotRadius, dotRadius);
        painter->setFont(QFont(QStringLiteral("Inter"), 8));
        painter->setPen(QColor(QStringLiteral("#061724")));
        painter->drawText(QRectF(point.x() - 14, point.y() + dotRadius + 1, 28, 12), Qt::AlignCenter, QString::number(midi));
        painter->setPen(QPen(QColor(QStringLiteral("#b9fff5")), 1.5));
    }
}

} // namespace notestudio::ui
