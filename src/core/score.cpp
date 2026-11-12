#include "score.hpp"

#include <bit>

namespace notestudio::core {
namespace {
constexpr int Semitones[] = {0, 2, 4, 5, 7, 9, 11};
}

std::uint8_t Pitch::midiNote() const noexcept {
    const int stepIndex = static_cast<int>(step);
    const int semitone = stepIndex >= 0 && stepIndex < 7 ? Semitones[stepIndex] : 0;
    const int value = (static_cast<int>(octave) + 1) * 12 + semitone + static_cast<int>(accidental);
    return static_cast<std::uint8_t>(value < 0 ? 0 : value > 127 ? 127 : value);
}

double Duration::quarterNotes() const noexcept {
    if (!valid()) {
        return 0.0;
    }
    double length = 4.0 / static_cast<double>(denominator);
    double addition = length / 2.0;
    for (std::uint8_t dot = 0; dot < dots; ++dot) {
        length += addition;
        addition /= 2.0;
    }
    return length;
}

bool Duration::valid() const noexcept {
    return denominator >= 1U && denominator <= 512U && std::has_single_bit(denominator) && dots <= 3U;
}

bool Measure::validTimeSignature() const noexcept {
    return beats > 0U && beatUnit > 0U && beatUnit <= 512U && std::has_single_bit(beatUnit);
}

double Measure::capacityInQuarterNotes() const noexcept {
    return validTimeSignature() ? static_cast<double>(beats) * 4.0 / static_cast<double>(beatUnit) : 0.0;
}

} // namespace notestudio::core
