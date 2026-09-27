// The MAX4466 microphone: continuous samples and the sound level.
//
// The ADC runs in continuous (DMA) mode at the chip's top rate, 83 333 Hz on
// the ESP32-S3 (audio -rate lowers it). A task on core 1 takes each DMA frame
// of kBlockSamples conversions and:
//
//   * removes the DC (the MAX4466 sits at mid-supply) and band-passes it:
//     4th-order Butterworth high-pass and low-pass, 400 Hz..3 kHz by default
//     (the voice band; the board picks up noise around 20 kHz). audio -band
//     changes the corners, 0 turns a side off;
//   * hands the block, as signed 12-bit counts, to on_block() -- main streams
//     it to TraceView;
//   * measures the level of the same filtered signal: RMS, dBFS and peak over
//     kPeriodMs of samples (20 levels per second).
//
// The sample rate sent with each block is the MEASURED one (samples counted
// against esp_timer since the last start), so TraceView plays at the right
// pitch even if the ADC clock is a little off its nominal rate.
//
// Only ADC1 has DMA on the S3, which is also why the pin is GPIO1 (board.h).
// Readers (any task) get the last values; nothing blocks.

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <driver/gpio.h>
#include <esp_adc/adc_continuous.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <soc/soc_caps.h>

class Audio {
public:
    /// One DMA frame = one block handed to on_block().
    static constexpr uint16_t kBlockSamples = 1024;
    /// Level period: one level per kPeriodMs of samples.
    static constexpr uint32_t kPeriodMs = 50;
    static constexpr uint32_t kMinRateHz = 8000;
    static constexpr uint32_t kMaxRateHz = SOC_ADC_SAMPLE_FREQ_THRES_HIGH;
    static constexpr uint32_t kDefaultRateHz = kMaxRateHz;
    /// RMS of a full-scale sine (12-bit ADC, 2048 counts peak): 0 dBFS.
    static constexpr float kFullScaleRms = 2048.0f / 1.41421356f;
    static constexpr float kFloorDbfs = -90.0f;
    /// Samples are counts around the DC level: -2048..2047.
    static constexpr int16_t kFullScale = 2048;
    /// Default band: the voice.
    static constexpr uint16_t kDefaultHighPassHz = 400;
    static constexpr uint16_t kDefaultLowPassHz = 3000;

    /// Called on the audio task for every block. True: it was taken (queued
    /// for TraceView).
    using BlockFn = bool (*)(const int16_t* samples, size_t count, uint32_t rate_hz,
                             uint64_t timestamp_us);

    struct Stats {
        uint32_t rate_hz;          // configured
        uint32_t actual_rate_hz;   // measured since the last start
        uint32_t blocks;           // DMA frames read
        uint32_t blocks_streamed;  // taken by on_block()
        uint32_t overflows;        // DMA pool full: samples lost (the task fell behind)
        uint32_t read_errors;
        uint16_t high_pass_hz;     // 0: off
        uint16_t low_pass_hz;      // 0: off
    };

    esp_err_t begin(gpio_num_t pin, int core, BlockFn on_block);

    float rms() const { return rms_.load(std::memory_order_relaxed); }
    float dbfs() const { return dbfs_.load(std::memory_order_relaxed); }
    uint16_t peak() const { return peak_.load(std::memory_order_relaxed); }
    Stats stats() const;

    /// kMinRateHz..kMaxRateHz; false otherwise. The task restarts the ADC.
    bool set_rate(uint32_t hz);
    /// Band-pass corners in Hz, 0 = that side off. False when high_pass is
    /// not below low_pass, or low_pass is not below half the sample rate.
    /// Applied from the next block.
    bool set_band(uint16_t high_pass_hz, uint16_t low_pass_hz);

private:
    // One second-order section (RBJ cookbook), direct form II transposed.
    struct Biquad {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
        float run(float x) {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    static void task_entry(void* self);
    static bool IRAM_ATTR on_pool_overflow(adc_continuous_handle_t, const adc_continuous_evt_data_t*,
                                           void* self);
    void run();
    esp_err_t start_adc(uint32_t rate);
    void process(const adc_continuous_data_t* data, uint32_t count);
    void finish_level();
    void configure_band(uint32_t band, uint32_t rate);

    adc_continuous_handle_t adc_ = nullptr;
    adc_unit_t unit_ = ADC_UNIT_1;
    adc_channel_t channel_ = ADC_CHANNEL_0;
    TaskHandle_t task_ = nullptr;
    BlockFn on_block_ = nullptr;

    // Audio task only.
    uint8_t raw_[kBlockSamples * SOC_ADC_DIGI_RESULT_BYTES] = {};
    adc_continuous_data_t parsed_[kBlockSamples] = {};
    int16_t block_[kBlockSamples] = {};
    uint32_t running_rate_ = 0;
    uint64_t counted_ = 0;        // samples since the ADC started
    int64_t started_us_ = 0;
    float dc_ = 2048.0f;
    float dc_alpha_ = 0.0f;
    Biquad high_pass_[2];
    Biquad low_pass_[2];
    bool high_pass_on_ = false;
    bool low_pass_on_ = false;
    uint32_t running_band_ = 0;
    double level_sum_sq_ = 0.0;
    float level_peak_ = 0.0f;
    uint32_t level_count_ = 0;

    std::atomic<uint32_t> rate_hz_{kDefaultRateHz};
    // high_pass_hz << 16 | low_pass_hz
    std::atomic<uint32_t> band_{(uint32_t{kDefaultHighPassHz} << 16) | kDefaultLowPassHz};
    std::atomic<uint32_t> actual_rate_hz_{0};
    std::atomic<float> rms_{0.0f};
    std::atomic<float> dbfs_{kFloorDbfs};
    std::atomic<uint16_t> peak_{0};
    std::atomic<uint32_t> blocks_{0};
    std::atomic<uint32_t> blocks_streamed_{0};
    std::atomic<uint32_t> overflows_{0};
    std::atomic<uint32_t> read_errors_{0};
};
