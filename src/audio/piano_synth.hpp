#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

struct ma_device;

namespace notestudio::audio {

class PianoSynth final {
public:
    static constexpr std::size_t MaxVoices = 64;

    PianoSynth() noexcept = default;
    ~PianoSynth();
    PianoSynth(const PianoSynth&) = delete;
    PianoSynth& operator=(const PianoSynth&) = delete;

    [[nodiscard]] bool start() noexcept;
    void stop() noexcept;
    void noteOn(std::uint8_t midiNote, float velocity = 0.8F) noexcept;
    void noteOff(std::uint8_t midiNote) noexcept;
    void setOrganMode(bool enabled) noexcept { organMode_.store(enabled, std::memory_order_relaxed); }

private:
    struct Voice {
        std::atomic<float> frequency{0.0F};
        std::atomic<float> velocity{0.0F};
        std::atomic<float> releaseGain{1.0F};
        std::atomic<float> gate{0.0F};
        std::atomic<std::uint32_t> generation{0U};
    };

    static void dataCallback(ma_device* device, void* output, const void* input, std::uint32_t frameCount) noexcept;
    void render(float* output, std::uint32_t frameCount) noexcept;

    std::array<Voice, MaxVoices> voices_{};
    std::atomic<std::uint32_t> nextVoice_{0U};
    std::atomic<bool> organMode_{false};
    ma_device* device_{nullptr};
    double sampleRate_{48000.0};
};

} // namespace notestudio::audio
