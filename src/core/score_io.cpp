#include "score_io.hpp"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <variant>

namespace notestudio::core {
namespace {
constexpr int XmlDivisions = 2048;
constexpr int MidiPpqn = 9600;

void setError(QString* target, const QString& value) { if (target) *target = value; }

QJsonObject pitchToJson(const Pitch& pitch) {
    return {{QStringLiteral("step"), static_cast<int>(pitch.step)},
            {QStringLiteral("accidental"), static_cast<int>(pitch.accidental)},
            {QStringLiteral("octave"), static_cast<int>(pitch.octave)}};
}

bool pitchFromJson(const QJsonObject& object, Pitch& pitch) {
    const int step = object.value(QStringLiteral("step")).toInt(-1);
    const int accidental = object.value(QStringLiteral("accidental")).toInt(99);
    const int octave = object.value(QStringLiteral("octave")).toInt(99);
    if (step < 0 || step > 6 || accidental < -3 || accidental > 3 || octave < -1 || octave > 9) return false;
    pitch.step = static_cast<Step>(step);
    pitch.accidental = static_cast<Accidental>(accidental);
    pitch.octave = static_cast<std::int8_t>(octave);
    return true;
}

QJsonObject durationToJson(const Duration& duration) {
    return {{QStringLiteral("denominator"), static_cast<int>(duration.denominator)},
            {QStringLiteral("dots"), static_cast<int>(duration.dots)}};
}

bool durationFromJson(const QJsonObject& object, Duration& duration) {
    const int denominator = object.value(QStringLiteral("denominator")).toInt(0);
    const int dots = object.value(QStringLiteral("dots")).toInt(-1);
    if (denominator < 1 || denominator > 1024 || dots < 0 || dots > 3) return false;
    duration = Duration{static_cast<std::uint16_t>(denominator), static_cast<std::uint8_t>(dots)};
    return duration.valid();
}

QJsonObject fermataToJson(const std::optional<Fermata>& fermata) {
    if (!fermata) return {};
    return {{QStringLiteral("type"), static_cast<int>(fermata->type)},
            {QStringLiteral("lengthFactor"), fermata->lengthFactor}};
}

bool fermataFromJson(const QJsonObject& object, std::optional<Fermata>& fermata) {
    if (object.isEmpty()) { fermata.reset(); return true; }
    const int type = object.value(QStringLiteral("type")).toInt(-1);
    const double factor = object.value(QStringLiteral("lengthFactor")).toDouble(0.0);
    if (type < 0 || type > 5 || !std::isfinite(factor) || factor < 0.25 || factor > 16.0) return false;
    fermata = Fermata{static_cast<FermataType>(type), factor};
    return true;
}

QJsonObject eventToJson(const MeasureEvent& event) {
    QJsonObject result;
    std::visit([&result](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        result.insert(QStringLiteral("duration"), durationToJson(item.duration));
        result.insert(QStringLiteral("fermata"), fermataToJson(item.fermata));
        if constexpr (std::is_same_v<T, Note>) {
            result.insert(QStringLiteral("kind"), QStringLiteral("note"));
            result.insert(QStringLiteral("pitch"), pitchToJson(item.pitch));
            result.insert(QStringLiteral("articulation"), static_cast<int>(item.articulation));
            result.insert(QStringLiteral("tieGroup"), static_cast<qint64>(item.tieGroup));
            result.insert(QStringLiteral("slurGroup"), static_cast<qint64>(item.slurGroup));
        } else if constexpr (std::is_same_v<T, Chord>) {
            result.insert(QStringLiteral("kind"), QStringLiteral("chord"));
            result.insert(QStringLiteral("articulation"), static_cast<int>(item.articulation));
            result.insert(QStringLiteral("stem"), static_cast<int>(item.stem));
            QJsonArray pitches;
            for (const Pitch& pitch : item.pitches) pitches.append(pitchToJson(pitch));
            result.insert(QStringLiteral("pitches"), pitches);
        } else {
            result.insert(QStringLiteral("kind"), QStringLiteral("rest"));
        }
    }, event);
    return result;
}

bool eventFromJson(const QJsonObject& object, MeasureEvent& event) {
    Duration duration;
    if (!durationFromJson(object.value(QStringLiteral("duration")).toObject(), duration)) return false;
    std::optional<Fermata> fermata;
    if (!fermataFromJson(object.value(QStringLiteral("fermata")).toObject(), fermata)) return false;
    const QString kind = object.value(QStringLiteral("kind")).toString();
    const int articulationValue = object.value(QStringLiteral("articulation")).toInt(0);
    if (articulationValue < 0 || articulationValue > 3) return false;
    if (kind == QStringLiteral("note")) {
        Pitch pitch;
        if (!pitchFromJson(object.value(QStringLiteral("pitch")).toObject(), pitch)) return false;
        const qint64 tie = object.value(QStringLiteral("tieGroup")).toInteger();
        const qint64 slur = object.value(QStringLiteral("slurGroup")).toInteger();
        if (tie < 0 || slur < 0 || tie > std::numeric_limits<std::uint32_t>::max() || slur > std::numeric_limits<std::uint32_t>::max()) return false;
        event = Note{pitch, duration, static_cast<Articulation>(articulationValue), fermata,
                     static_cast<std::uint32_t>(tie), static_cast<std::uint32_t>(slur)};
    } else if (kind == QStringLiteral("rest")) {
        event = Rest{duration, fermata};
    } else if (kind == QStringLiteral("chord")) {
        const int stemValue = object.value(QStringLiteral("stem")).toInt(0);
        const QJsonArray pitchesJson = object.value(QStringLiteral("pitches")).toArray();
        if (stemValue < 0 || stemValue > 2 || pitchesJson.isEmpty() || pitchesJson.size() > 32) return false;
        std::vector<Pitch> pitches;
        for (const QJsonValue& value : pitchesJson) {
            Pitch pitch;
            if (!pitchFromJson(value.toObject(), pitch)) return false;
            pitches.push_back(pitch);
        }
        event = Chord{std::move(pitches), duration, static_cast<StemDirection>(stemValue),
                      static_cast<Articulation>(articulationValue), fermata};
    } else return false;
    return true;
}

QString stepText(Step step) {
    static const QString steps[] = {QStringLiteral("C"), QStringLiteral("D"), QStringLiteral("E"),
                                    QStringLiteral("F"), QStringLiteral("G"), QStringLiteral("A"), QStringLiteral("B")};
    const int index = static_cast<int>(step);
    return index >= 0 && index < 7 ? steps[index] : QStringLiteral("C");
}

int durationTicks(const Duration& duration, int ppqn) {
    return static_cast<int>(std::llround(duration.quarterNotes() * static_cast<double>(ppqn)));
}

void writeBe16(QByteArray& out, std::uint16_t value) {
    out.append(static_cast<char>((value >> 8U) & 0xffU)); out.append(static_cast<char>(value & 0xffU));
}
void writeBe32(QByteArray& out, std::uint32_t value) {
    out.append(static_cast<char>((value >> 24U) & 0xffU)); out.append(static_cast<char>((value >> 16U) & 0xffU));
    out.append(static_cast<char>((value >> 8U) & 0xffU)); out.append(static_cast<char>(value & 0xffU));
}
void writeVlq(QByteArray& out, std::uint32_t value) {
    std::uint8_t bytes[4]{};
    int count = 0;
    bytes[count++] = static_cast<std::uint8_t>(value & 0x7fU);
    while ((value >>= 7U) != 0U && count < 4) bytes[count++] = static_cast<std::uint8_t>((value & 0x7fU) | 0x80U);
    while (count > 0) out.append(static_cast<char>(bytes[--count]));
}

QString musicType(std::uint16_t denominator) {
    switch (denominator) {
    case 1: return QStringLiteral("whole"); case 2: return QStringLiteral("half");
    case 4: return QStringLiteral("quarter"); case 8: return QStringLiteral("eighth");
    case 16: return QStringLiteral("16th"); case 32: return QStringLiteral("32nd");
    case 64: return QStringLiteral("64th"); case 128: return QStringLiteral("128th");
    case 256: return QStringLiteral("256th"); case 512: return QStringLiteral("512th");
    default: return QStringLiteral("1024th");
    }
}

std::uint16_t denominatorForType(const QString& type) {
    if (type == QStringLiteral("whole")) return 1; if (type == QStringLiteral("half")) return 2;
    if (type == QStringLiteral("quarter")) return 4; if (type == QStringLiteral("eighth")) return 8;
    if (type == QStringLiteral("16th")) return 16; if (type == QStringLiteral("32nd")) return 32;
    if (type == QStringLiteral("64th")) return 64; if (type == QStringLiteral("128th")) return 128;
    if (type == QStringLiteral("256th")) return 256; if (type == QStringLiteral("512th")) return 512;
    if (type == QStringLiteral("1024th")) return 1024;
    return 0;
}

} // namespace

bool ScoreIO::saveJson(const Measure& measure, const QString& path, QString* error) {
    QJsonArray events;
    for (const MeasureEvent& event : measure.events) events.append(eventToJson(event));
    const QJsonObject root{{QStringLiteral("format"), QStringLiteral("NoteStudioProject")},
        {QStringLiteral("version"), 1}, {QStringLiteral("beats"), static_cast<int>(measure.beats)},
        {QStringLiteral("beatUnit"), static_cast<int>(measure.beatUnit)},
        {QStringLiteral("repeatStart"), static_cast<int>(measure.repeatStart)},
        {QStringLiteral("repeatEnd"), static_cast<int>(measure.repeatEnd)},
        {QStringLiteral("repeatCount"), static_cast<int>(measure.repeatCount)},
        {QStringLiteral("coda"), measure.coda}, {QStringLiteral("segno"), measure.segno},
        {QStringLiteral("tempoBpm"), static_cast<int>(measure.tempoBpm)},
        {QStringLiteral("dynamic"), static_cast<int>(measure.dynamic)}, {QStringLiteral("wedge"), static_cast<int>(measure.wedge)},
        {QStringLiteral("events"), events}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { setError(error, file.errorString()); return false; }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) { setError(error, file.errorString()); return false; }
    if (!file.commit()) { setError(error, file.errorString()); return false; }
    return true;
}

bool ScoreIO::loadJson(Measure& measure, const QString& path, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { setError(error, file.errorString()); return false; }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(error, QStringLiteral("Invalid project JSON: %1").arg(parseError.errorString())); return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("NoteStudioProject") ||
        root.value(QStringLiteral("version")).toInt() != 1) { setError(error, QStringLiteral("Unsupported project format/version")); return false; }
    const int beats = root.value(QStringLiteral("beats")).toInt(0);
    const int beatUnit = root.value(QStringLiteral("beatUnit")).toInt(0);
    const int repeatStart = root.value(QStringLiteral("repeatStart")).toInt(-1);
    const int repeatEnd = root.value(QStringLiteral("repeatEnd")).toInt(-1);
    const int repeatCount = root.value(QStringLiteral("repeatCount")).toInt(0);
    const int tempoBpm = root.value(QStringLiteral("tempoBpm")).toInt(120);
    const int dynamic = root.value(QStringLiteral("dynamic")).toInt(static_cast<int>(DynamicMark::MF));
    const int wedge = root.value(QStringLiteral("wedge")).toInt(0);
    if (beats < 1 || beats > 32 || beatUnit < 1 || beatUnit > 512 || repeatStart < 0 || repeatStart > 1 ||
        repeatEnd < 0 || repeatEnd > 2 || repeatCount < 1 || repeatCount > 16 || tempoBpm < 20 || tempoBpm > 300 ||
        dynamic < 0 || dynamic > static_cast<int>(DynamicMark::Niente) || wedge < 0 || wedge > static_cast<int>(DynamicWedge::DiminuendoNiente)) {
        setError(error, QStringLiteral("Project contains invalid meter or navigation values")); return false;
    }
    Measure loaded;
    loaded.beats = static_cast<std::uint8_t>(beats); loaded.beatUnit = static_cast<std::uint16_t>(beatUnit);
    loaded.repeatStart = static_cast<RepeatStart>(repeatStart); loaded.repeatEnd = static_cast<RepeatEnd>(repeatEnd);
    loaded.repeatCount = static_cast<std::uint8_t>(repeatCount); loaded.coda = root.value(QStringLiteral("coda")).toBool();
    loaded.segno = root.value(QStringLiteral("segno")).toBool(); loaded.tempoBpm = static_cast<std::uint16_t>(tempoBpm);
    loaded.dynamic = static_cast<DynamicMark>(dynamic); loaded.wedge = static_cast<DynamicWedge>(wedge);
    if (!loaded.validTimeSignature()) { setError(error, QStringLiteral("Invalid time signature")); return false; }
    const QJsonArray events = root.value(QStringLiteral("events")).toArray();
    if (events.size() > 2048) { setError(error, QStringLiteral("Too many events")); return false; }
    double total = 0.0;
    for (const QJsonValue& value : events) {
        MeasureEvent event;
        if (!eventFromJson(value.toObject(), event)) { setError(error, QStringLiteral("Invalid event data")); return false; }
        total += std::visit([](const auto& item) { return item.duration.quarterNotes(); }, event);
        loaded.events.push_back(std::move(event));
    }
    if (total > loaded.capacityInQuarterNotes() + 1.0e-8) { setError(error, QStringLiteral("Events exceed measure capacity")); return false; }
    measure = std::move(loaded);
    return true;
}

bool ScoreIO::exportMusicXml(const Measure& measure, const QString& path, QString* error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { setError(error, file.errorString()); return false; }
    QXmlStreamWriter xml(&file); xml.setAutoFormatting(true);
    xml.writeStartDocument(); xml.writeStartElement(QStringLiteral("score-partwise")); xml.writeAttribute(QStringLiteral("version"), QStringLiteral("4.0"));
    xml.writeStartElement(QStringLiteral("part-list")); xml.writeStartElement(QStringLiteral("score-part")); xml.writeAttribute(QStringLiteral("id"), QStringLiteral("P1"));
    xml.writeTextElement(QStringLiteral("part-name"), QStringLiteral("Piano")); xml.writeEndElement(); xml.writeEndElement();
    xml.writeStartElement(QStringLiteral("part")); xml.writeAttribute(QStringLiteral("id"), QStringLiteral("P1"));
    xml.writeStartElement(QStringLiteral("measure")); xml.writeAttribute(QStringLiteral("number"), QStringLiteral("1"));
    xml.writeStartElement(QStringLiteral("attributes")); xml.writeTextElement(QStringLiteral("divisions"), QString::number(XmlDivisions));
    xml.writeStartElement(QStringLiteral("time")); xml.writeTextElement(QStringLiteral("beats"), QString::number(measure.beats));
    xml.writeTextElement(QStringLiteral("beat-type"), QString::number(measure.beatUnit)); xml.writeEndElement(); xml.writeEndElement();
    xml.writeStartElement(QStringLiteral("direction")); xml.writeStartElement(QStringLiteral("direction-type"));
    xml.writeStartElement(QStringLiteral("metronome")); xml.writeTextElement(QStringLiteral("beat-unit"), QStringLiteral("quarter"));
    xml.writeTextElement(QStringLiteral("per-minute"), QString::number(measure.tempoBpm)); xml.writeEndElement();
    static const QString dynamicNames[] = {QStringLiteral("ppp"),QStringLiteral("pp"),QStringLiteral("p"),QStringLiteral("mp"),QStringLiteral("mf"),QStringLiteral("f"),QStringLiteral("ff"),QStringLiteral("fff"),QStringLiteral("n")};
    xml.writeStartElement(QStringLiteral("dynamics"));
    if (measure.dynamic == DynamicMark::Niente) xml.writeTextElement(QStringLiteral("other-dynamics"), QStringLiteral("niente"));
    else xml.writeEmptyElement(dynamicNames[static_cast<std::size_t>(measure.dynamic)]);
    xml.writeEndElement();
    if (measure.wedge != DynamicWedge::None) {
        xml.writeStartElement(QStringLiteral("wedge"));
        const QString wedgeName = measure.wedge == DynamicWedge::Crescendo || measure.wedge == DynamicWedge::CrescendoNiente ? QStringLiteral("crescendo") : QStringLiteral("diminuendo");
        xml.writeAttribute(QStringLiteral("type"), wedgeName);
        if (measure.wedge == DynamicWedge::CrescendoNiente || measure.wedge == DynamicWedge::DiminuendoNiente) xml.writeAttribute(QStringLiteral("niente"), QStringLiteral("yes"));
        xml.writeEndElement();
    }
    if (measure.segno) xml.writeEmptyElement(QStringLiteral("segno"));
    if (measure.coda) xml.writeEmptyElement(QStringLiteral("coda"));
    xml.writeEndElement(); xml.writeEndElement();
    for (const MeasureEvent& event : measure.events) {
        std::visit([&xml](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            const auto writeNote = [&xml, &item](const Pitch* pitch, bool chordTone) {
                xml.writeStartElement(QStringLiteral("note"));
                if (chordTone) xml.writeEmptyElement(QStringLiteral("chord"));
                if (pitch == nullptr) xml.writeEmptyElement(QStringLiteral("rest"));
                else {
                    xml.writeStartElement(QStringLiteral("pitch")); xml.writeTextElement(QStringLiteral("step"), stepText(pitch->step));
                    const int alter = static_cast<int>(pitch->accidental);
                    if (alter != 0) xml.writeTextElement(QStringLiteral("alter"), QString::number(alter));
                    xml.writeTextElement(QStringLiteral("octave"), QString::number(static_cast<int>(pitch->octave))); xml.writeEndElement();
                }
                xml.writeTextElement(QStringLiteral("duration"), QString::number(durationTicks(item.duration, XmlDivisions)));
                xml.writeTextElement(QStringLiteral("type"), musicType(item.duration.denominator));
                for (std::uint8_t i = 0; i < item.duration.dots; ++i) xml.writeEmptyElement(QStringLiteral("dot"));
                if constexpr (std::is_same_v<T, Note>) {
                    if (item.tieGroup != 0U) { xml.writeStartElement(QStringLiteral("tie")); xml.writeAttribute(QStringLiteral("type"), QStringLiteral("start")); xml.writeEndElement(); }
                    if (item.fermata) { xml.writeStartElement(QStringLiteral("notations")); xml.writeEmptyElement(QStringLiteral("fermata")); xml.writeEndElement(); }
                } else if constexpr (std::is_same_v<T, Chord>) {
                    if (item.fermata) { xml.writeStartElement(QStringLiteral("notations")); xml.writeEmptyElement(QStringLiteral("fermata")); xml.writeEndElement(); }
                } else if (item.fermata) { xml.writeStartElement(QStringLiteral("notations")); xml.writeEmptyElement(QStringLiteral("fermata")); xml.writeEndElement(); }
                xml.writeEndElement();
            };
            if constexpr (std::is_same_v<T, Note>) writeNote(&item.pitch, false);
            else if constexpr (std::is_same_v<T, Chord>) {
                bool first = true; for (const Pitch& pitch : item.pitches) { writeNote(&pitch, !first); first = false; }
            } else writeNote(nullptr, false);
        }, event);
    }
    if (measure.repeatStart != RepeatStart::None || measure.repeatEnd != RepeatEnd::None) {
        xml.writeStartElement(QStringLiteral("barline")); xml.writeAttribute(QStringLiteral("location"), QStringLiteral("right"));
        if (measure.repeatStart != RepeatStart::None) { xml.writeStartElement(QStringLiteral("repeat")); xml.writeAttribute(QStringLiteral("direction"), QStringLiteral("forward")); xml.writeEndElement(); }
        if (measure.repeatEnd != RepeatEnd::None) { xml.writeStartElement(QStringLiteral("repeat")); xml.writeAttribute(QStringLiteral("direction"), QStringLiteral("backward")); xml.writeEndElement(); }
        xml.writeEndElement();
    }
    xml.writeEndElement(); xml.writeEndElement(); xml.writeEndElement(); xml.writeEndDocument();
    if (!file.commit()) { setError(error, file.errorString().isEmpty() ? QStringLiteral("MusicXML write failed") : file.errorString()); return false; }
    return true;
}

bool ScoreIO::importMusicXml(Measure& measure, const QString& path, QString* error) {
    QFile file(path); if (!file.open(QIODevice::ReadOnly)) { setError(error, file.errorString()); return false; }
    QXmlStreamReader xml(&file); Measure loaded; int divisions = XmlDivisions; int beats = 4; int beatUnit = 4; int tempoBpm = 120;
    struct Parsed { bool rest{false}; bool chordTone{false}; bool hasPitch{false}; Step step{Step::C}; int alter{0}; int octave{4}; int ticks{0}; std::uint16_t denominator{4}; std::uint8_t dots{0}; bool fermata{false}; bool tie{false}; };
    std::optional<Parsed> pending;
    auto addPending = [&]() {
        if (!pending) return;
        Parsed item = *pending; pending.reset();
        if (item.ticks <= 0) return;
        double bestError = std::numeric_limits<double>::max();
        Duration best{4, 0};
        const double importedQuarterNotes = static_cast<double>(item.ticks) / static_cast<double>(divisions);
        for (std::uint16_t denominator = 1; denominator <= 1024; denominator = static_cast<std::uint16_t>(denominator * 2U)) {
            for (std::uint8_t dots = 0; dots <= 3U; ++dots) {
                const Duration candidate{denominator, dots};
                const double difference = std::abs(candidate.quarterNotes() - importedQuarterNotes);
                if (difference < bestError) { bestError = difference; best = candidate; }
            }
        }
        Duration d = bestError < 0.002 ? best : Duration{item.denominator, item.dots};
        if (!d.valid()) { d = Duration{4, 0}; }
        std::optional<Fermata> fermata = item.fermata ? std::optional<Fermata>(Fermata{}) : std::nullopt;
        if (item.rest) loaded.events.emplace_back(Rest{d, fermata});
        else if (item.hasPitch) {
            Pitch p{item.step, static_cast<Accidental>(std::clamp(item.alter, -3, 3)), static_cast<std::int8_t>(std::clamp(item.octave, -1, 9))};
            if (item.chordTone && !loaded.events.empty()) {
                if (auto* chord = std::get_if<Chord>(&loaded.events.back())) chord->pitches.push_back(p);
                else if (auto* note = std::get_if<Note>(&loaded.events.back())) loaded.events.back() = Chord{{note->pitch, p}, note->duration, StemDirection::Automatic, note->articulation, note->fermata};
            } else {
                Note n{p, d}; if (fermata) n.fermata = fermata;
                if (item.tie) n.tieGroup = static_cast<std::uint32_t>(loaded.events.size() + 1U);
                loaded.events.emplace_back(n);
            }
        }
    };
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            const QString name = xml.name().toString();
            if (name == QStringLiteral("note")) { addPending(); pending = Parsed{}; }
            else if (name == QStringLiteral("chord") && pending) pending->chordTone = true;
            else if (name == QStringLiteral("rest") && pending) pending->rest = true;
            else if (name == QStringLiteral("fermata") && pending) pending->fermata = true;
            else if (name == QStringLiteral("tie") && pending && xml.attributes().value(QStringLiteral("type")) == QStringLiteral("start")) pending->tie = true;
            else if (name == QStringLiteral("step") && pending) {
                const QString s = xml.readElementText(); const QStringList names{QStringLiteral("C"),QStringLiteral("D"),QStringLiteral("E"),QStringLiteral("F"),QStringLiteral("G"),QStringLiteral("A"),QStringLiteral("B")};
                const int idx = names.indexOf(s); if (idx >= 0) { pending->step = static_cast<Step>(idx); pending->hasPitch = true; }
            } else if (name == QStringLiteral("alter") && pending) pending->alter = xml.readElementText().toInt();
            else if (name == QStringLiteral("octave") && pending) pending->octave = xml.readElementText().toInt();
            else if (name == QStringLiteral("duration") && pending) pending->ticks = xml.readElementText().toInt();
            else if (name == QStringLiteral("divisions")) divisions = std::clamp(xml.readElementText().toInt(XmlDivisions), 1, 1000000);
            else if (name == QStringLiteral("beats")) beats = xml.readElementText().toInt(4);
            else if (name == QStringLiteral("beat-type")) beatUnit = xml.readElementText().toInt(4);
            else if (name == QStringLiteral("per-minute")) tempoBpm = xml.readElementText().toInt(120);
            else if (name == QStringLiteral("segno")) loaded.segno = true;
            else if (name == QStringLiteral("coda")) loaded.coda = true;
            else if (name == QStringLiteral("wedge")) {
                const QString type = xml.attributes().value(QStringLiteral("type")).toString();
                const bool niente = xml.attributes().value(QStringLiteral("niente")) == QStringLiteral("yes");
                if (type == QStringLiteral("crescendo")) loaded.wedge = niente ? DynamicWedge::CrescendoNiente : DynamicWedge::Crescendo;
                else if (type == QStringLiteral("diminuendo")) loaded.wedge = niente ? DynamicWedge::DiminuendoNiente : DynamicWedge::Diminuendo;
            } else if (name == QStringLiteral("ppp")) loaded.dynamic = DynamicMark::PPP;
            else if (name == QStringLiteral("pp")) loaded.dynamic = DynamicMark::PP;
            else if (name == QStringLiteral("p")) loaded.dynamic = DynamicMark::P;
            else if (name == QStringLiteral("mp")) loaded.dynamic = DynamicMark::MP;
            else if (name == QStringLiteral("mf")) loaded.dynamic = DynamicMark::MF;
            else if (name == QStringLiteral("f")) loaded.dynamic = DynamicMark::F;
            else if (name == QStringLiteral("ff")) loaded.dynamic = DynamicMark::FF;
            else if (name == QStringLiteral("fff")) loaded.dynamic = DynamicMark::FFF;
            else if (name == QStringLiteral("n")) loaded.dynamic = DynamicMark::Niente;
            else if (name == QStringLiteral("other-dynamics") && xml.readElementText() == QStringLiteral("niente")) loaded.dynamic = DynamicMark::Niente;
            else if (name == QStringLiteral("type") && pending) pending->denominator = denominatorForType(xml.readElementText());
            else if (name == QStringLiteral("dot") && pending && pending->dots < 3U) ++pending->dots;
            else if (name == QStringLiteral("repeat")) {
                const QString direction = xml.attributes().value(QStringLiteral("direction")).toString();
                if (direction == QStringLiteral("forward")) loaded.repeatStart = RepeatStart::Begin;
                if (direction == QStringLiteral("backward")) loaded.repeatEnd = RepeatEnd::End;
            }
        } else if (xml.isEndElement() && xml.name() == QStringLiteral("note")) addPending();
    }
    addPending();
    if (xml.hasError()) { setError(error, QStringLiteral("MusicXML parse error: %1").arg(xml.errorString())); return false; }
    if (beats < 1 || beats > 32 || beatUnit < 1 || beatUnit > 512) { setError(error, QStringLiteral("Invalid MusicXML time signature")); return false; }
    loaded.beats = static_cast<std::uint8_t>(beats); loaded.beatUnit = static_cast<std::uint16_t>(beatUnit);
    loaded.tempoBpm = static_cast<std::uint16_t>(std::clamp(tempoBpm, 20, 300));
    if (!loaded.validTimeSignature()) { setError(error, QStringLiteral("Unsupported MusicXML meter")); return false; }
    double total = 0.0; for (const auto& event : loaded.events) total += std::visit([](const auto& item) { return item.duration.quarterNotes(); }, event);
    if (total > loaded.capacityInQuarterNotes() + 1.0e-8) { setError(error, QStringLiteral("Imported notes exceed the supported single measure")); return false; }
    measure = std::move(loaded); return true;
}

bool ScoreIO::exportMidi(const Measure& measure, const QString& path, int bpm, QString* error) {
    if (bpm < 20 || bpm > 300) { setError(error, QStringLiteral("Tempo must be between 20 and 300 BPM")); return false; }
    struct Event { std::uint64_t tick; std::uint8_t status; std::uint8_t note; std::uint8_t velocity; };
    std::vector<Event> events; std::uint64_t cursor = 0;
    const auto addNote = [&events](std::uint64_t start, std::uint64_t end, std::uint8_t note, std::uint8_t velocity) {
        events.push_back({start, 0x90U, note, velocity}); events.push_back({end, 0x80U, note, 0U});
    };
    for (const MeasureEvent& event : measure.events) {
        const int ticks = std::max(1, durationTicks(std::visit([](const auto& item) { return item.duration; }, event), MidiPpqn));
        const std::uint64_t end = cursor + static_cast<std::uint64_t>(ticks);
        std::visit([&](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, Note>) addNote(cursor, end, item.pitch.midiNote(), item.articulation == Articulation::Sforzando ? 120U : item.articulation == Articulation::Accent ? 110U : 90U);
            else if constexpr (std::is_same_v<T, Chord>) for (const Pitch& pitch : item.pitches) addNote(cursor, end, pitch.midiNote(), item.articulation == Articulation::Sforzando ? 120U : item.articulation == Articulation::Accent ? 110U : 90U);
        }, event);
        cursor = end;
    }
    std::stable_sort(events.begin(), events.end(), [](const Event& a, const Event& b) { return a.tick < b.tick || (a.tick == b.tick && a.status < b.status); });
    QByteArray track; track.append("\x00\xff\x51\x03", 4); const std::uint32_t micros = static_cast<std::uint32_t>(60000000 / bpm);
    track.append(static_cast<char>((micros >> 16U) & 0xffU)); track.append(static_cast<char>((micros >> 8U) & 0xffU)); track.append(static_cast<char>(micros & 0xffU));
    std::uint64_t previous = 0;
    for (const Event& event : events) {
        writeVlq(track, static_cast<std::uint32_t>(event.tick - previous)); previous = event.tick;
        track.append(static_cast<char>(event.status)); track.append(static_cast<char>(event.note)); track.append(static_cast<char>(event.velocity));
    }
    track.append("\x00\xff\x2f\x00", 4);
    QByteArray file; file.append("MThd", 4); writeBe32(file, 6); writeBe16(file, 0); writeBe16(file, 1); writeBe16(file, MidiPpqn);
    file.append("MTrk", 4); writeBe32(file, static_cast<std::uint32_t>(track.size())); file.append(track);
    QSaveFile output(path); if (!output.open(QIODevice::WriteOnly) || output.write(file) != file.size() || !output.commit()) {
        setError(error, output.errorString().isEmpty() ? QStringLiteral("MIDI file write failed") : output.errorString()); return false;
    }
    return true;
}

} // namespace notestudio::core
