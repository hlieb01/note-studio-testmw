#pragma once

#include "../audio/piano_synth.hpp"
#include "../core/score.hpp"

#include <QObject>
#include <QTimer>
#include <QVariantList>

#include <cstddef>
#include <vector>

namespace notestudio::ui {

class ScoreController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool hexMode READ hexMode WRITE setHexMode NOTIFY hexModeChanged)
    Q_PROPERTY(int notationMode READ notationMode NOTIFY notationModeChanged)
    Q_PROPERTY(int noteCount READ noteCount NOTIFY scoreChanged)
    Q_PROPERTY(int beats READ beats NOTIFY meterChanged)
    Q_PROPERTY(int beatUnit READ beatUnit NOTIFY meterChanged)
    Q_PROPERTY(bool coda READ coda NOTIFY scoreChanged)
    Q_PROPERTY(bool segno READ segno NOTIFY scoreChanged)
    Q_PROPERTY(int tempoBpm READ tempoBpm NOTIFY tempoChanged)
    Q_PROPERTY(QString tempoName READ tempoName NOTIFY tempoChanged)
    Q_PROPERTY(int dynamicMark READ dynamicMark NOTIFY scoreChanged)
    Q_PROPERTY(int dynamicWedge READ dynamicWedge NOTIFY scoreChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QVariantList noteData READ noteData NOTIFY scoreChanged)
public:
    explicit ScoreController(QObject* parent = nullptr);
    [[nodiscard]] bool playing() const noexcept { return playing_; }
    [[nodiscard]] bool hexMode() const noexcept { return hexMode_; }
    [[nodiscard]] int notationMode() const noexcept { return notationMode_; }
    [[nodiscard]] int noteCount() const noexcept;
    [[nodiscard]] int beats() const noexcept { return measure_.beats; }
    [[nodiscard]] int beatUnit() const noexcept { return static_cast<int>(measure_.beatUnit); }
    [[nodiscard]] bool coda() const noexcept { return measure_.coda; }
    [[nodiscard]] bool segno() const noexcept { return measure_.segno; }
    [[nodiscard]] int tempoBpm() const noexcept { return static_cast<int>(measure_.tempoBpm); }
    [[nodiscard]] QString tempoName() const;
    [[nodiscard]] int dynamicMark() const noexcept { return static_cast<int>(measure_.dynamic); }
    [[nodiscard]] int dynamicWedge() const noexcept { return static_cast<int>(measure_.wedge); }
    [[nodiscard]] QString message() const { return message_; }
    [[nodiscard]] QVariantList noteData() const;

    bool initializeAudio();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void startPlayback();
    Q_INVOKABLE void stopPlayback();
    Q_INVOKABLE void addNote(int midiNote, int denominator, int dots);
    Q_INVOKABLE void addChord(int midiRoot, int denominator, int dots);
    Q_INVOKABLE void addRest(int denominator, int dots);
    Q_INVOKABLE void clearMeasure();
    Q_INVOKABLE void setTimeSignature(int numerator, int denominator);
    Q_INVOKABLE void addFermata(int type);
    Q_INVOKABLE void tieLastTwo();
    Q_INVOKABLE void slurLastTwo();
    Q_INVOKABLE void toggleRepeat();
    Q_INVOKABLE void toggleCoda();
    Q_INVOKABLE void toggleSegno();
    Q_INVOKABLE void setTempoBpm(int bpm);
    Q_INVOKABLE void setDynamics(int mark, int wedge);
    Q_INVOKABLE void setLastArticulation(const QString& name);
    Q_INVOKABLE void saveProjectJson(const QString& path);
    Q_INVOKABLE void loadProjectJson(const QString& path);
    Q_INVOKABLE void exportMusicXml(const QString& path);
    Q_INVOKABLE void importMusicXml(const QString& path);
    Q_INVOKABLE void exportMidi(const QString& path);
    void setHexMode(bool enabled);
    Q_INVOKABLE void setNotationMode(int mode);

signals:
    void playingChanged();
    void hexModeChanged();
    void notationModeChanged();
    void scoreChanged();
    void meterChanged();
    void messageChanged();
    void tempoChanged();

private slots:
    void playNextEvent();

private:
    void setMessage(const QString& value);
    void scheduleNoteOff(int midiNote, int milliseconds);

    audio::PianoSynth synth_;
    core::Measure measure_{};
    QTimer playbackTimer_;
    std::vector<int> playbackNotes_;
    std::size_t playbackIndex_{0};
    std::uint8_t repeatPass_{1U};
    bool playing_{false};
    bool hexMode_{false};
    int notationMode_{0};
    double bpm_{120.0};
    std::uint32_t nextGroupId_{1U};
    QString message_{QStringLiteral("Ready")};
};

} // namespace notestudio::ui
