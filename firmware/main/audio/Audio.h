// Sound level from the MAX4466 microphone.
//
// A task on core 1 reads blocks with adc_oneshot: kBlockSamples samples
// paced at the sample rate (16 ms at 16 kHz), then sleeps until the next
// period (kPeriodMs). Each block gives one level -- 20 per second -- and the
// core stays free about two thirds of the time.
//
// Per sample: DC removal (slow average) and a 3-stage low-pass at 3 kHz (the
// voice band), as the MVP firmware did; per block: RMS and peak.
//
// A failed read is counted (stats().read_errors) and the sample skipped. On
// ADC1 (board::kMic) that should never happen; on ADC2 it does whenever Wi-Fi
// is on. The ADC1 also has a DMA (continuous) mode that could replace the
// paced reads -- not needed for a level.
//
// Readers (any task) get the last block's values; nothing blocks.

#pragma once

#include <atomic>
#include <cstdint>

#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class Audio {
public:
    static constexpr uint16_t kBlockSamples = 256;
    static constexpr uint32_t kPeriodMs = 50;
    static constexpr uint32_t kDefaultRateHz = 16000;
    static constexpr uint32_t kMinRateHz = 2000;
    static constexpr uint32_t kMaxRateHz = 16000;
    /// RMS of a full-scale sine (12-bit ADC, 2048 counts peak): 0 dBFS.
    static constexpr float kFullScaleRms = 2048.0f / 1.41421356f;
    static constexpr float kFloorDbfs = -90.0f;

    struct Stats {
        uint32_t rate_hz;        // configured
        uint32_t actual_rate_hz; // measured over the last block
        uint32_t blocks;
        uint32_t read_errors;
        uint32_t block_us;       // time to read one block
    };

    esp_err_t begin(gpio_num_t pin, int core);

    float rms() const { return rms_.load(std::memory_order_relaxed); }
    float dbfs() const { return dbfs_.load(std::memory_order_relaxed); }
    uint16_t peak() const { return peak_.load(std::memory_order_relaxed); }
    Stats stats() const;

    /// kMinRateHz..kMaxRateHz; false otherwise. Takes effect on the next block.
    bool set_rate(uint32_t hz);

private:
    static void task_entry(void* self);
    void run();
    void read_block();

    adc_oneshot_unit_handle_t unit_ = nullptr;
    adc_channel_t channel_ = ADC_CHANNEL_0;
    TaskHandle_t task_ = nullptr;

    // Filter state (audio task only).
    float dc_ = 2048.0f;
    float lp1_ = 0.0f;
    float lp2_ = 0.0f;
    float lp3_ = 0.0f;

    std::atomic<uint32_t> rate_hz_{kDefaultRateHz};
    std::atomic<float> rms_{0.0f};
    std::atomic<float> dbfs_{kFloorDbfs};
    std::atomic<uint16_t> peak_{0};
    std::atomic<uint32_t> actual_rate_hz_{0};
    std::atomic<uint32_t> blocks_{0};
    std::atomic<uint32_t> read_errors_{0};
    std::atomic<uint32_t> block_us_{0};
};
