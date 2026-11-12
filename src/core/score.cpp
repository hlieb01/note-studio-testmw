#include "score.hpp"

#include <bit>
#include <cmath>

namespace notestudio::core {

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
    return denominator != 0U && dots <= 3U && std::has_single_bit(denominator);
}

bool Measure::validTimeSignature() const noexcept {
    return beats > 0U && beatUnit > 0U && std::has_single_bit(beatUnit);
}

double Measure::capacityInQuarterNotes() const noexcept {
    if (!validTimeSignature()) {
        return 0.0;
    }
    return static_cast<double>(beats) * 4.0 / static_cast<double>(beatUnit);
}

} // namespace notestudio::core
