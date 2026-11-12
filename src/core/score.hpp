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
    friend bool operator==(const Pitch&, const Pitch&) = default;
};

struct Duration {
    // Denominator: 1=whole, 2=half, 4=quarter, etc.
    std::uint8_t denominator{4};
    std::uint8_t dots{0};
    [[nodiscard]] double quarterNotes() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
};

enum class StemDirection : std::uint8_t { Automatic, Up, Down };

struct Note {
    Pitch pitch{};
    Duration duration{};
    bool accent{false};
};

struct Rest {
    Duration duration{};
};

struct Chord {
    std::vector<Pitch> pitches;
    Duration duration{};
    StemDirection stem{StemDirection::Automatic};
};

using MeasureEvent = std::variant<Note, Rest, Chord>;

enum class RepeatStart : std::uint8_t { None, Begin };
enum class RepeatEnd : std::uint8_t { None, End, Endings };

struct Measure {
    std::uint8_t beats{4};
    std::uint8_t beatUnit{4};
    RepeatStart repeatStart{RepeatStart::None};
    RepeatEnd repeatEnd{RepeatEnd::None};
    std::uint8_t repeatCount{2};
    std::vector<MeasureEvent> events;

    [[nodiscard]] bool validTimeSignature() const noexcept;
    [[nodiscard]] double capacityInQuarterNotes() const noexcept;
};

} // namespace notestudio::core
