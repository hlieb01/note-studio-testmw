#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include "piano_synth.hpp"

#include <algorithm>
#include <cmath>
#include <new>

namespace notestudio::audio {
namespace {
constexpr double Pi = 3.14159265358979323846;
constexpr float TwoPi = 6.2831853071795864769F;
constexpr float AttackSeconds = 0.004F;
constexpr float DecaySeconds = 1.8F;
constexpr float ReleaseSeconds = 0.35F;
constexpr float Inharmonicity = 0.00012F;
constexpr std::size_t HarmonicCount = 12;
}

PianoSynth::~PianoSynth() { stop(); }

bool PianoSynth::start() noexcept {
    if (device_ != nullptr) {
        return true;
    }
    auto* device = new (std::nothrow) ma_device{};
    if (device == nullptr) {
        return false;
    }
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate = static_cast<ma_uint32>(sampleRate_);
    config.dataCallback = &PianoSynth::dataCallback;
    config.pUserData = this;
    if (ma_device_init(nullptr, &config, device) != MA_SUCCESS) {
        delete device;
        return false;
    }
    if (ma_device_start(device) != MA_SUCCESS) {
        ma_device_uninit(device);
        delete device;
        return false;
    }
    device_ = device;
    sampleRate_ = static_cast<double>(device->sampleRate);
    return true;
}

void PianoSynth::stop() noexcept {
    if (device_ == nullptr) {
        return;
    }
    ma_device_uninit(device_);
    delete device_;
    device_ = nullptr;
}

void PianoSynth::noteOn(std::uint8_t midiNote, float velocity) noexcept {
    const float clampedVelocity = std::clamp(velocity, 0.0F, 1.0F);
    const double frequency = 440.0 * std::pow(2.0, (static_cast<double>(midiNote) - 69.0) / 12.0);
    const std::uint32_t index = nextVoice_.fetch_add(1U, std::memory_order_relaxed) % static_cast<std::uint32_t>(MaxVoices);
    Voice& voice = voices_[index];
    voice.gate.store(0.0F, std::memory_order_release);
    voice.generation.store(0U, std::memory_order_relaxed);
    voice.frequency.store(static_cast<float>(frequency), std::memory_order_relaxed);
    voice.velocity.store(clampedVelocity, std::memory_order_relaxed);
    voice.releaseGain.store(1.0F, std::memory_order_relaxed);
    voice.gate.store(1.0F, std::memory_order_release);
}

void PianoSynth::noteOff(std::uint8_t midiNote) noexcept {
    const double target = 440.0 * std::pow(2.0, (static_cast<double>(midiNote) - 69.0) / 12.0);
    for (Voice& voice : voices_) {
        const float frequency = voice.frequency.load(std::memory_order_relaxed);
        if (std::abs(static_cast<double>(frequency) - target) < 0.5) {
            voice.gate.store(0.0F, std::memory_order_release);
        }
    }
}

void PianoSynth::dataCallback(ma_device* device, void* output, const void*, std::uint32_t frameCount) noexcept {
    if (device == nullptr || output == nullptr || device->pUserData == nullptr) {
        return;
    }
    static_cast<PianoSynth*>(device->pUserData)->render(static_cast<float*>(output), frameCount);
}

void PianoSynth::render(float* output, std::uint32_t frameCount) noexcept {
    const double inverseRate = 1.0 / sampleRate_;
    const bool organMode = organMode_.load(std::memory_order_relaxed);
    for (std::uint32_t frame = 0U; frame < frameCount; ++frame) {
        float sample = 0.0F;
        for (Voice& voice : voices_) {
            const float frequency = voice.frequency.load(std::memory_order_relaxed);
            if (frequency <= 0.0F) {
                continue;
            }
            const float gate = voice.gate.load(std::memory_order_acquire);
            float amplitude = voice.velocity.load(std::memory_order_relaxed);
            const std::uint32_t age = voice.generation.load(std::memory_order_relaxed);
            const double time = static_cast<double>(age) * inverseRate;
            const double envelope = organMode
                ? std::min(1.0, time / 0.025)
                : (time < static_cast<double>(AttackSeconds) ? time / static_cast<double>(AttackSeconds) : std::exp(-time / static_cast<double>(DecaySeconds)));
            float releaseGain = voice.releaseGain.load(std::memory_order_relaxed);
            if (gate <= 0.0F) {
                releaseGain *= static_cast<float>(std::exp(-inverseRate / static_cast<double>(ReleaseSeconds)));
                voice.releaseGain.store(releaseGain, std::memory_order_relaxed);
                if (releaseGain < 0.0001F) {
                    voice.frequency.store(0.0F, std::memory_order_relaxed);
                    continue;
                }
            }
            amplitude *= static_cast<float>(envelope) * releaseGain;
            const double baseFrequency = static_cast<double>(frequency);
            const std::size_t harmonicCount = organMode ? 8U : HarmonicCount;
            for (std::size_t harmonic = 1U; harmonic <= harmonicCount; ++harmonic) {
                const double partial = static_cast<double>(harmonic);
                const double inharmonicPartial = partial * std::sqrt(1.0 + static_cast<double>(Inharmonicity) * partial * partial);
                const float weight = organMode
                    ? (harmonic == 1U ? 1.0F : harmonic == 2U ? 0.55F : harmonic == 3U ? 0.38F : harmonic == 4U ? 0.25F : harmonic == 6U ? 0.16F : 0.10F)
                    : 1.0F / static_cast<float>(harmonic);
                const double phase = std::fmod(static_cast<double>(age) * inverseRate * baseFrequency * inharmonicPartial * static_cast<double>(TwoPi), 2.0 * Pi);
                const float hammer = 1.0F + 0.22F * static_cast<float>(harmonic == 1U ? 1.0 : std::exp(-partial / 3.0));
                sample += amplitude * weight * hammer * static_cast<float>(std::sin(phase));
            }
            voice.generation.fetch_add(1U, std::memory_order_relaxed);
        }
        sample = std::clamp(sample * 0.035F, -0.95F, 0.95F);
        output[static_cast<std::size_t>(frame) * 2U] = sample;
        output[static_cast<std::size_t>(frame) * 2U + 1U] = sample;
    }
}

} // namespace notestudio::audio
