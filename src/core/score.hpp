#pragma once

#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace notestudio::core {

enum class Step : std::uint8_t { C, D, E, F, G, A, B };
enum class Accidental : std::int8_t { Flat = -1, Natural = 0, Sharp = 1 };

struct Pitch {
    Step step{Step::C};
    Accidental accidental{Accidental::Natural};
    std::int8_t octave{4};
    [[nodiscard]] std::uint8_t midiNote() const noexcept;
    friend bool operator==(const Pitch&, const Pitch&) = default;
};

struct Duration {
    // Denominator: 1=whole through 512=1/512; dots add successive halves.
    std::uint16_t denominator{4};
    std::uint8_t dots{0};
    [[nodiscard]] double quarterNotes() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
};

enum class StemDirection : std::uint8_t { Automatic, Up, Down };
enum class FermataType : std::uint8_t { Normal, Short, Long, VeryLong };
enum class Articulation : std::uint8_t { None, Staccato, Accent, Sforzando };

struct Fermata {
    FermataType type{FermataType::Normal};
    double lengthFactor{1.5};
};

struct Note {
    Pitch pitch{};
    Duration duration{};
    Articulation articulation{Articulation::None};
    std::optional<Fermata> fermata{};
    std::uint32_t tieGroup{0};
    std::uint32_t slurGroup{0};
};

struct Rest {
    Duration duration{};
    std::optional<Fermata> fermata{};
};

struct Chord {
    std::vector<Pitch> pitches;
    Duration duration{};
    StemDirection stem{StemDirection::Automatic};
    Articulation articulation{Articulation::None};
    std::optional<Fermata> fermata{};
};

using MeasureEvent = std::variant<Note, Rest, Chord>;

enum class RepeatStart : std::uint8_t { None, Begin };
enum class RepeatEnd : std::uint8_t { None, End, Endings };

struct Measure {
    std::uint8_t beats{4};
    std::uint16_t beatUnit{4};
    RepeatStart repeatStart{RepeatStart::None};
    RepeatEnd repeatEnd{RepeatEnd::None};
    std::uint8_t repeatCount{2};
    bool coda{false};
    std::vector<MeasureEvent> events;
    [[nodiscard]] bool validTimeSignature() const noexcept;
    [[nodiscard]] double capacityInQuarterNotes() const noexcept;
};

struct Part {
    enum class Clef : std::uint8_t { Treble, Bass };
    Clef clef{Clef::Treble};
    std::vector<Measure> measures;
};

struct Score {
    std::vector<Part> parts;
};

} // namespace notestudio::core
