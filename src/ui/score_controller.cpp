#include "score_controller.hpp"
#include "../core/score_io.hpp"

#include <QMetaObject>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <utility>
#include <variant>

namespace notestudio::ui {

ScoreController::ScoreController(QObject* parent) : QObject(parent) {
    playbackTimer_.setSingleShot(true);
    connect(&playbackTimer_, &QTimer::timeout, this, &ScoreController::playNextEvent);
}

int ScoreController::noteCount() const noexcept {
    return static_cast<int>(measure_.events.size());
}

QString ScoreController::tempoName() const {
    if (bpm_ < 76.0) return QStringLiteral("Adagio");
    if (bpm_ < 109.0) return QStringLiteral("Andante");
    if (bpm_ < 169.0) return QStringLiteral("Allegro");
    return QStringLiteral("Presto");
}

QVariantList ScoreController::noteData() const {
    QVariantList result;
    for (const auto& event : measure_.events) {
        QVariantMap item;
        std::visit([&item](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            item.insert(QStringLiteral("denominator"), static_cast<int>(value.duration.denominator));
            item.insert(QStringLiteral("dots"), static_cast<int>(value.duration.dots));
            item.insert(QStringLiteral("isRest"), std::is_same_v<T, core::Rest>);
            item.insert(QStringLiteral("fermata"), value.fermata.has_value());
            if constexpr (std::is_same_v<T, core::Note>) {
                item.insert(QStringLiteral("midi"), static_cast<int>(value.pitch.midiNote()));
                item.insert(QStringLiteral("accidental"), static_cast<int>(value.pitch.accidental));
                item.insert(QStringLiteral("step"), static_cast<int>(value.pitch.step));
                item.insert(QStringLiteral("articulation"), static_cast<int>(value.articulation));
                item.insert(QStringLiteral("tie"), static_cast<qulonglong>(value.tieGroup));
                item.insert(QStringLiteral("slur"), static_cast<qulonglong>(value.slurGroup));
            } else if constexpr (std::is_same_v<T, core::Chord>) {
                item.insert(QStringLiteral("midi"), value.pitches.empty() ? 60 : static_cast<int>(value.pitches.front().midiNote()));
                item.insert(QStringLiteral("accidental"), value.pitches.empty() ? 0 : static_cast<int>(value.pitches.front().accidental));
                item.insert(QStringLiteral("articulation"), static_cast<int>(value.articulation));
            } else {
                item.insert(QStringLiteral("midi"), 60);
                item.insert(QStringLiteral("articulation"), 0);
            }
            item.insert(QStringLiteral("fermataType"), value.fermata ? static_cast<int>(value.fermata->type) : 0);
        }, event);
        result.push_back(item);
    }
    return result;
}

bool ScoreController::initializeAudio() {
    if (!synth_.start()) {
        setMessage(QStringLiteral("Audio device unavailable; playback will retry when requested"));
        return false;
    }
    setMessage(QStringLiteral("Audio ready"));
    return true;
}

void ScoreController::togglePlayback() {
    playing_ ? stopPlayback() : startPlayback();
}

void ScoreController::startPlayback() {
    if (playing_) return;
    if (measure_.events.empty()) {
        setMessage(QStringLiteral("Add notes to the measure first"));
        return;
    }
    if (!synth_.start()) {
        setMessage(QStringLiteral("Audio device unavailable"));
        return;
    }
    playbackIndex_ = 0U;
    repeatPass_ = 1U;
    playing_ = true;
    emit playingChanged();
    setMessage(QStringLiteral("Playing · %1 BPM").arg(bpm_));
    playNextEvent();
}

void ScoreController::stopPlayback() {
    if (!playing_) return;
    playbackTimer_.stop();
    for (const int midiNote : playbackNotes_) {
        synth_.noteOff(static_cast<std::uint8_t>(midiNote));
    }
    playbackNotes_.clear();
    playing_ = false;
    emit playingChanged();
    setMessage(QStringLiteral("Stopped"));
}

void ScoreController::playNextEvent() {
    if (!playing_) return;
    if (playbackIndex_ >= measure_.events.size()) {
        const bool repeat = measure_.repeatStart == core::RepeatStart::Begin && measure_.repeatEnd == core::RepeatEnd::End;
        if (repeat && repeatPass_ < std::max<std::uint8_t>(2U, measure_.repeatCount)) {
            ++repeatPass_;
            playbackIndex_ = 0U;
            playNextEvent();
            return;
        }
        if (playing_) {
            playing_ = false;
            emit playingChanged();
            setMessage(QStringLiteral("Playback complete"));
        }
        return;
    }

    const core::MeasureEvent& event = measure_.events[playbackIndex_++];
    core::Duration duration{};
    std::vector<int> pitches;
    core::Articulation articulation{core::Articulation::None};
    double fermataFactor = 1.0;
    std::uint32_t tieGroup = 0U;

    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        duration = item.duration;
        if constexpr (std::is_same_v<T, core::Note>) {
            pitches.push_back(static_cast<int>(item.pitch.midiNote()));
            articulation = item.articulation;
            tieGroup = item.tieGroup;
            if (item.fermata) fermataFactor = item.fermata->lengthFactor;
        } else if constexpr (std::is_same_v<T, core::Chord>) {
            for (const core::Pitch& pitch : item.pitches) pitches.push_back(static_cast<int>(pitch.midiNote()));
            articulation = item.articulation;
            if (item.fermata) fermataFactor = item.fermata->lengthFactor;
        } else if (item.fermata) {
            fermataFactor = item.fermata->lengthFactor;
        }
    }, event);

    const double beatMs = 60000.0 / bpm_;
    const int durationMs = std::max(1, static_cast<int>(std::lround(duration.quarterNotes() * beatMs * fermataFactor)));
    double gate = articulation == core::Articulation::Staccato ? 0.42 : 0.92;
    if (articulation == core::Articulation::Accent || articulation == core::Articulation::Sforzando) gate = 0.96;
    bool tiedFromPrevious = false;
    bool tieContinues = false;
    if (tieGroup != 0U && playbackIndex_ >= 2U) {
        const auto* previous = std::get_if<core::Note>(&measure_.events[playbackIndex_ - 2U]);
        const auto* current = std::get_if<core::Note>(&event);
        tiedFromPrevious = previous != nullptr && current != nullptr && previous->tieGroup == tieGroup && previous->pitch == current->pitch;
    }
    if (tieGroup != 0U && playbackIndex_ < measure_.events.size()) {
        const auto* current = std::get_if<core::Note>(&event);
        const auto* next = std::get_if<core::Note>(&measure_.events[playbackIndex_]);
        tieContinues = current != nullptr && next != nullptr && next->tieGroup == tieGroup && next->pitch == current->pitch;
    }
    static constexpr float dynamicVelocity[] = {0.20F, 0.28F, 0.38F, 0.52F, 0.66F, 0.78F, 0.89F, 1.0F, 0.0F};
    float volume = dynamicVelocity[static_cast<std::size_t>(measure_.dynamic)];
    const double progress = measure_.events.size() <= 1U ? 1.0 : static_cast<double>(playbackIndex_ - 1U) / static_cast<double>(measure_.events.size() - 1U);
    if (measure_.wedge == core::DynamicWedge::Crescendo || measure_.wedge == core::DynamicWedge::CrescendoNiente) volume *= static_cast<float>(progress);
    else if (measure_.wedge == core::DynamicWedge::Diminuendo || measure_.wedge == core::DynamicWedge::DiminuendoNiente) volume *= static_cast<float>(1.0 - progress);
    for (const int midiNote : pitches) {
        const float articulationVelocity = articulation == core::Articulation::Sforzando ? 1.0F : articulation == core::Articulation::Accent ? 0.92F : 0.78F;
        const float velocity = std::clamp(articulationVelocity * volume, 0.0F, 1.0F);
        if (!tiedFromPrevious) {
            synth_.noteOn(static_cast<std::uint8_t>(midiNote), velocity);
            playbackNotes_.push_back(midiNote);
        }
        if (!tieContinues) scheduleNoteOff(midiNote, std::max(1, static_cast<int>(static_cast<double>(durationMs) * gate)));
    }
    playbackTimer_.start(durationMs);
}

void ScoreController::scheduleNoteOff(int midiNote, int milliseconds) {
    QTimer::singleShot(milliseconds, this, [this, midiNote]() {
        synth_.noteOff(static_cast<std::uint8_t>(midiNote));
        const auto found = std::find(playbackNotes_.begin(), playbackNotes_.end(), midiNote);
        if (found != playbackNotes_.end()) playbackNotes_.erase(found);
    });
}

void ScoreController::addNote(int midiNote, int denominator, int dots) {
    if (midiNote < 0 || midiNote > 127 || denominator < 1 || denominator > 1024 || dots < 0 || dots > 3) return;
    const auto validDenominator = static_cast<std::uint16_t>(denominator);
    core::Duration duration{validDenominator, static_cast<std::uint8_t>(dots)};
    if (!duration.valid()) return;
    const double used = [&]() {
        double sum = 0.0;
        for (const auto& event : measure_.events) {
            sum += std::visit([](const auto& item) { return item.duration.quarterNotes(); }, event);
        }
        return sum;
    }();
    if (used + duration.quarterNotes() > measure_.capacityInQuarterNotes() + 1.0e-9) {
        setMessage(QStringLiteral("Measure is full"));
        return;
    }
    const int semitone = midiNote % 12;
    static constexpr core::Step steps[] = {core::Step::C, core::Step::C, core::Step::D, core::Step::D, core::Step::E, core::Step::F, core::Step::F, core::Step::G, core::Step::G, core::Step::A, core::Step::A, core::Step::B};
    static constexpr core::Accidental accidentals[] = {core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural};
    core::Pitch pitch{steps[semitone], accidentals[semitone], static_cast<std::int8_t>(midiNote / 12 - 1)};
    measure_.events.emplace_back(core::Note{pitch, duration});
    emit scoreChanged();
    setMessage(QStringLiteral("Added note · 1/%1").arg(denominator));
}

void ScoreController::addChord(int midiRoot, int denominator, int dots) {
    if (midiRoot < 0 || midiRoot > 115 || denominator < 1 || denominator > 1024 || dots < 0 || dots > 3) return;
    const core::Duration duration{static_cast<std::uint16_t>(denominator), static_cast<std::uint8_t>(dots)};
    if (!duration.valid()) return;
    double used = 0.0;
    for (const auto& event : measure_.events) {
        used += std::visit([](const auto& item) { return item.duration.quarterNotes(); }, event);
    }
    if (used + duration.quarterNotes() > measure_.capacityInQuarterNotes() + 1.0e-9) {
        setMessage(QStringLiteral("Measure is full"));
        return;
    }
    const auto makePitch = [](int midi) {
        const int semitone = midi % 12;
        static constexpr core::Step steps[] = {core::Step::C, core::Step::C, core::Step::D, core::Step::D, core::Step::E, core::Step::F, core::Step::F, core::Step::G, core::Step::G, core::Step::A, core::Step::A, core::Step::B};
        static constexpr core::Accidental accidentals[] = {core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural, core::Accidental::Sharp, core::Accidental::Natural};
        return core::Pitch{steps[semitone], accidentals[semitone], static_cast<std::int8_t>(midi / 12 - 1)};
    };
    measure_.events.emplace_back(core::Chord{{makePitch(midiRoot), makePitch(midiRoot + 4), makePitch(midiRoot + 7)}, duration});
    emit scoreChanged();
    setMessage(QStringLiteral("Added triad chord"));
}

void ScoreController::addRest(int denominator, int dots) {
    if (denominator < 1 || denominator > 1024 || dots < 0 || dots > 3) return;
    core::Duration duration{static_cast<std::uint16_t>(denominator), static_cast<std::uint8_t>(dots)};
    if (!duration.valid()) return;
    double used = 0.0;
    for (const auto& event : measure_.events) {
        used += std::visit([](const auto& item) { return item.duration.quarterNotes(); }, event);
    }
    if (used + duration.quarterNotes() > measure_.capacityInQuarterNotes() + 1.0e-9) {
        setMessage(QStringLiteral("Measure is full"));
        return;
    }
    measure_.events.emplace_back(core::Rest{duration});
    emit scoreChanged();
    setMessage(QStringLiteral("Added rest"));
}

void ScoreController::clearMeasure() {
    stopPlayback();
    measure_.events.clear();
    emit scoreChanged();
    setMessage(QStringLiteral("Measure cleared"));
}

void ScoreController::setTimeSignature(int numerator, int denominator) {
    if (numerator < 1 || numerator > 32 || denominator < 1 || denominator > 512) return;
    core::Measure candidate = measure_;
    candidate.beats = static_cast<std::uint8_t>(numerator);
    candidate.beatUnit = static_cast<std::uint16_t>(denominator);
    if (!candidate.validTimeSignature()) return;
    double used = 0.0;
    for (const auto& event : candidate.events) {
        used += std::visit([](const auto& item) { return item.duration.quarterNotes(); }, event);
    }
    if (used > candidate.capacityInQuarterNotes() + 1.0e-9) {
        candidate.events.clear();
        setMessage(QStringLiteral("Measure reset for new time signature"));
    }
    measure_ = std::move(candidate);
    emit meterChanged();
    emit scoreChanged();
    setMessage(QStringLiteral("Time signature · %1/%2").arg(numerator).arg(denominator));
}

void ScoreController::addFermata(int type) {
    if (measure_.events.empty()) return;
    if (type < 0 || type > 5) return;
    static constexpr double factors[] = {1.5, 1.2, 2.0, 3.0, 1.75, 1.5};
    const core::Fermata fermata{static_cast<core::FermataType>(type), factors[type]};
    std::visit([fermata](auto& item) { item.fermata = fermata; }, measure_.events.back());
    emit scoreChanged();
    setMessage(QStringLiteral("Fermata variant applied"));
}

void ScoreController::tieLastTwo() {
    if (measure_.events.size() < 2U) return;
    auto* first = std::get_if<core::Note>(&measure_.events[measure_.events.size() - 2U]);
    auto* second = std::get_if<core::Note>(&measure_.events.back());
    if (first == nullptr || second == nullptr || first->pitch != second->pitch) {
        setMessage(QStringLiteral("Tie requires matching notes"));
        return;
    }
    const std::uint32_t group = nextGroupId_++;
    first->tieGroup = group;
    second->tieGroup = group;
    emit scoreChanged();
    setMessage(QStringLiteral("Tie added"));
}

void ScoreController::slurLastTwo() {
    if (measure_.events.size() < 2U) return;
    auto* first = std::get_if<core::Note>(&measure_.events[measure_.events.size() - 2U]);
    auto* second = std::get_if<core::Note>(&measure_.events.back());
    if (first == nullptr || second == nullptr) {
        setMessage(QStringLiteral("Slur requires two notes"));
        return;
    }
    const std::uint32_t group = nextGroupId_++;
    first->slurGroup = group;
    second->slurGroup = group;
    emit scoreChanged();
    setMessage(QStringLiteral("Slur added"));
}

void ScoreController::toggleRepeat() {
    const bool enable = measure_.repeatStart == core::RepeatStart::None;
    measure_.repeatStart = enable ? core::RepeatStart::Begin : core::RepeatStart::None;
    measure_.repeatEnd = enable ? core::RepeatEnd::End : core::RepeatEnd::None;
    emit scoreChanged();
    setMessage(enable ? QStringLiteral("Repeat enabled") : QStringLiteral("Repeat disabled"));
}

void ScoreController::toggleCoda() {
    measure_.coda = !measure_.coda;
    emit scoreChanged();
    setMessage(measure_.coda ? QStringLiteral("Coda marker added") : QStringLiteral("Coda marker removed"));
}

void ScoreController::toggleSegno() {
    measure_.segno = !measure_.segno;
    emit scoreChanged();
    setMessage(measure_.segno ? QStringLiteral("Segno marker added") : QStringLiteral("Segno marker removed"));
}

void ScoreController::setTempoBpm(int bpm) {
    if (bpm < 20 || bpm > 300 || measure_.tempoBpm == static_cast<std::uint16_t>(bpm)) return;
    measure_.tempoBpm = static_cast<std::uint16_t>(bpm);
    bpm_ = static_cast<double>(bpm);
    emit tempoChanged();
    emit scoreChanged();
    setMessage(QStringLiteral("%1 · %2 BPM").arg(tempoName()).arg(bpm));
}

void ScoreController::setDynamics(int mark, int wedge) {
    if (mark < 0 || mark > static_cast<int>(core::DynamicMark::Niente) ||
        wedge < 0 || wedge > static_cast<int>(core::DynamicWedge::DiminuendoNiente)) return;
    measure_.dynamic = static_cast<core::DynamicMark>(mark);
    measure_.wedge = static_cast<core::DynamicWedge>(wedge);
    emit scoreChanged();
    setMessage(QStringLiteral("Dynamics updated"));
}

void ScoreController::setLastArticulation(const QString& name) {
    if (measure_.events.empty()) return;
    core::Articulation articulation = core::Articulation::None;
    if (name == QStringLiteral("staccato")) articulation = core::Articulation::Staccato;
    else if (name == QStringLiteral("accent")) articulation = core::Articulation::Accent;
    else if (name == QStringLiteral("sforzando")) articulation = core::Articulation::Sforzando;
    std::visit([articulation](auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, core::Note> || std::is_same_v<T, core::Chord>) item.articulation = articulation;
    }, measure_.events.back());
    emit scoreChanged();
    setMessage(QStringLiteral("Articulation updated"));
}

void ScoreController::saveProjectJson(const QString& path) {
    QString error;
    if (core::ScoreIO::saveJson(measure_, path, &error)) setMessage(QStringLiteral("Project saved: %1").arg(path));
    else setMessage(QStringLiteral("Save failed: %1").arg(error));
}

void ScoreController::loadProjectJson(const QString& path) {
    core::Measure loaded;
    QString error;
    if (!core::ScoreIO::loadJson(loaded, path, &error)) { setMessage(QStringLiteral("Open failed: %1").arg(error)); return; }
    stopPlayback();
    measure_ = std::move(loaded);
    bpm_ = static_cast<double>(measure_.tempoBpm);
    emit meterChanged();
    emit scoreChanged();
    emit tempoChanged();
    setMessage(QStringLiteral("Project opened: %1").arg(path));
}

void ScoreController::exportMusicXml(const QString& path) {
    QString error;
    if (core::ScoreIO::exportMusicXml(measure_, path, &error)) setMessage(QStringLiteral("MusicXML exported"));
    else setMessage(QStringLiteral("MusicXML export failed: %1").arg(error));
}

void ScoreController::importMusicXml(const QString& path) {
    core::Measure loaded;
    QString error;
    if (!core::ScoreIO::importMusicXml(loaded, path, &error)) { setMessage(QStringLiteral("MusicXML import failed: %1").arg(error)); return; }
    stopPlayback();
    measure_ = std::move(loaded);
    bpm_ = static_cast<double>(measure_.tempoBpm);
    emit meterChanged();
    emit scoreChanged();
    emit tempoChanged();
    setMessage(QStringLiteral("MusicXML imported"));
}

void ScoreController::exportMidi(const QString& path) {
    QString error;
    if (core::ScoreIO::exportMidi(measure_, path, static_cast<int>(bpm_), &error)) setMessage(QStringLiteral("MIDI exported"));
    else setMessage(QStringLiteral("MIDI export failed: %1").arg(error));
}

void ScoreController::setHexMode(bool enabled) {
    setNotationMode(enabled ? 1 : 0);
}

void ScoreController::setNotationMode(int mode) {
    const int bounded = std::clamp(mode, 0, 3);
    if (notationMode_ == bounded) return;
    const bool wasHex = hexMode_;
    notationMode_ = bounded;
    hexMode_ = bounded == 1;
    emit notationModeChanged();
    if (wasHex != hexMode_) emit hexModeChanged();
}

void ScoreController::setMessage(const QString& value) {
    if (message_ == value) return;
    message_ = value;
    emit messageChanged();
}

} // namespace notestudio::ui
