// The six front LEDs (LEDC PWM): a VU meter for the microphone, or a fixed
// pattern set from the terminal.
#pragma once

#include <atomic>
#include <cstdint>

#include <esp_err.h>

class Leds {
public:
    static constexpr uint16_t kDefaultStep = 9;   // RMS counts per LED (as in the MVP)
    static constexpr int kReleaseCalls = 3;         // 150 ms per LED going out

    esp_err_t begin();

    /// VU mode: LED i lights when rms > i * step. Call at the level's rate
    /// (every 50 ms). A LED lights at once but goes out only one at a time,
    /// every kReleaseCalls calls: a level sitting on a threshold no longer
    /// switches LEDs 20 times a second (each switch is a current step the
    /// LCD backlight and the microphone felt).
    void show_level(float rms);

    /// Fixed pattern (bit i = LED i); leaves VU mode.
    void set_mask(uint8_t mask);
    void set_vu(bool on) { vu_.store(on, std::memory_order_relaxed); }
    bool vu() const { return vu_.load(std::memory_order_relaxed); }
    void set_step(uint16_t counts) { step_.store(counts == 0 ? 1 : counts, std::memory_order_relaxed); }
    uint16_t step() const { return step_.load(std::memory_order_relaxed); }

private:
    void apply(uint8_t mask);

    bool ready_ = false;
    uint8_t shown_ = 0xFF;   // mask on the pins now (0xFF: never written)
    int lit_ = 0;            // VU: LEDs lit now
    int release_calls_ = 0;
    std::atomic<bool> vu_{true};
    std::atomic<uint16_t> step_{kDefaultStep};
    std::atomic<uint8_t> mask_{0};
};
