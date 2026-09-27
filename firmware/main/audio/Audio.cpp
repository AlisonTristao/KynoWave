#include "audio/Audio.h"

#include <cmath>

#include <esp_log.h>
#include <esp_timer.h>

namespace {

constexpr const char* kTag = "audio";
constexpr float kPi = 3.14159265f;
constexpr float kDcHz = 5.0f;  // DC tracker corner, below anything audible
// A 4th-order Butterworth as two second-order sections.
constexpr float kButterworthQ[2] = {0.54119610f, 1.30656296f};
// DMA frames the driver can hold while the task is busy (~50 ms at 83 kHz).
constexpr uint32_t kPoolFrames = 4;
constexpr uint32_t kReadTimeoutMs = 200;

// One-pole low-pass coefficient for corner `hz` at `rate`.
float one_pole(float hz, uint32_t rate) {
    const float dt = 1.0f / static_cast<float>(rate);
    const float rc = 1.0f / (2.0f * kPi * hz);
    return dt / (rc + dt);
}

}  // namespace

esp_err_t Audio::begin(gpio_num_t pin, int core, BlockFn on_block) {
    on_block_ = on_block;
    esp_err_t err = adc_continuous_io_to_channel(pin, &unit_, &channel_);
    if (err != ESP_OK || unit_ != ADC_UNIT_1) {
        ESP_LOGE(kTag, "GPIO%d is not an ADC1 pin (only ADC1 has DMA)", pin);
        return err != ESP_OK ? err : ESP_ERR_INVALID_ARG;
    }
    adc_continuous_handle_cfg_t handle_config = {};
    handle_config.max_store_buf_size = sizeof(raw_) * kPoolFrames;
    handle_config.conv_frame_size = sizeof(raw_);
    err = adc_continuous_new_handle(&handle_config, &adc_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "ADC continuous mode init failed (%s)", esp_err_to_name(err));
        return err;
    }
    adc_continuous_evt_cbs_t callbacks = {};
    callbacks.on_pool_ovf = &Audio::on_pool_overflow;
    adc_continuous_register_event_callbacks(adc_, &callbacks, this);

    if (xTaskCreatePinnedToCore(&Audio::task_entry, "audio", 4096, this, 5, &task_, core) != pdPASS) {
        ESP_LOGE(kTag, "could not create the audio task");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(kTag, "microphone on GPIO%d (ADC1 channel %d), %lu Hz continuous, %u-sample blocks", pin,
             channel_, static_cast<unsigned long>(rate_hz_.load()), kBlockSamples);
    return ESP_OK;
}

bool Audio::set_rate(uint32_t hz) {
    if (hz < kMinRateHz || hz > kMaxRateHz) return false;
    const uint16_t low_pass = static_cast<uint16_t>(band_.load(std::memory_order_relaxed) & 0xFFFFu);
    if (low_pass != 0 && low_pass * 2U >= hz) return false;  // the band must fit the new rate
    rate_hz_.store(hz, std::memory_order_relaxed);
    return true;
}

bool Audio::set_band(uint16_t high_pass_hz, uint16_t low_pass_hz) {
    if (high_pass_hz != 0 && low_pass_hz != 0 && high_pass_hz >= low_pass_hz) return false;
    if (low_pass_hz != 0 && low_pass_hz * 2U >= rate_hz_.load(std::memory_order_relaxed)) return false;
    band_.store((uint32_t{high_pass_hz} << 16) | low_pass_hz, std::memory_order_relaxed);
    return true;
}

void Audio::configure_band(uint32_t band, uint32_t rate) {
    const float high_pass = static_cast<float>(band >> 16);
    const float low_pass = static_cast<float>(band & 0xFFFFu);
    high_pass_on_ = high_pass > 0.0f;
    low_pass_on_ = low_pass > 0.0f;
    for (int i = 0; i < 2; ++i) {
        if (high_pass_on_) {
            const float w0 = 2.0f * kPi * high_pass / static_cast<float>(rate);
            const float alpha = std::sin(w0) / (2.0f * kButterworthQ[i]);
            const float c = std::cos(w0);
            const float a0 = 1.0f + alpha;
            high_pass_[i] = Biquad{(1.0f + c) / 2.0f / a0, -(1.0f + c) / a0, (1.0f + c) / 2.0f / a0,
                                   -2.0f * c / a0, (1.0f - alpha) / a0};
        }
        if (low_pass_on_) {
            const float w0 = 2.0f * kPi * low_pass / static_cast<float>(rate);
            const float alpha = std::sin(w0) / (2.0f * kButterworthQ[i]);
            const float c = std::cos(w0);
            const float a0 = 1.0f + alpha;
            low_pass_[i] = Biquad{(1.0f - c) / 2.0f / a0, (1.0f - c) / a0, (1.0f - c) / 2.0f / a0,
                                  -2.0f * c / a0, (1.0f - alpha) / a0};
        }
    }
    running_band_ = band;
}

Audio::Stats Audio::stats() const {
    return Stats{rate_hz_.load(std::memory_order_relaxed),
                 actual_rate_hz_.load(std::memory_order_relaxed),
                 blocks_.load(std::memory_order_relaxed),
                 blocks_streamed_.load(std::memory_order_relaxed),
                 overflows_.load(std::memory_order_relaxed),
                 read_errors_.load(std::memory_order_relaxed),
                 static_cast<uint16_t>(band_.load(std::memory_order_relaxed) >> 16),
                 static_cast<uint16_t>(band_.load(std::memory_order_relaxed) & 0xFFFFu)};
}

bool IRAM_ATTR Audio::on_pool_overflow(adc_continuous_handle_t, const adc_continuous_evt_data_t*, void* self) {
    static_cast<Audio*>(self)->overflows_.fetch_add(1, std::memory_order_relaxed);
    return false;
}

void Audio::task_entry(void* self) { static_cast<Audio*>(self)->run(); }

esp_err_t Audio::start_adc(uint32_t rate) {
    if (running_rate_ != 0) adc_continuous_stop(adc_);
    running_rate_ = 0;

    adc_digi_pattern_config_t pattern = {};
    pattern.atten = ADC_ATTEN_DB_12;  // the full 0..3.3 V swing of the MAX4466
    pattern.channel = channel_;
    pattern.unit = unit_;
    pattern.bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    adc_continuous_config_t config = {};
    config.pattern_num = 1;
    config.adc_pattern = &pattern;
    config.sample_freq_hz = rate;
    config.conv_mode = ADC_CONV_SINGLE_UNIT_1;
    esp_err_t err = adc_continuous_config(adc_, &config);
    if (err == ESP_OK) err = adc_continuous_start(adc_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "ADC start at %lu Hz failed (%s)", static_cast<unsigned long>(rate), esp_err_to_name(err));
        return err;
    }
    running_rate_ = rate;
    counted_ = 0;
    started_us_ = esp_timer_get_time();
    actual_rate_hz_.store(0, std::memory_order_relaxed);
    dc_alpha_ = one_pole(kDcHz, rate);
    configure_band(band_.load(std::memory_order_relaxed), rate);
    level_sum_sq_ = 0.0;
    level_peak_ = 0.0f;
    level_count_ = 0;
    return ESP_OK;
}

void Audio::run() {
    for (;;) {
        const uint32_t wanted = rate_hz_.load(std::memory_order_relaxed);
        if (wanted != running_rate_ && start_adc(wanted) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        uint32_t got = 0;
        esp_err_t err = adc_continuous_read(adc_, raw_, sizeof(raw_), &got, kReadTimeoutMs);
        if (err != ESP_OK) {
            read_errors_.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        uint32_t parsed = 0;
        if (adc_continuous_parse_data(adc_, raw_, got, parsed_, &parsed) != ESP_OK || parsed == 0) {
            read_errors_.fetch_add(1, std::memory_order_relaxed);
            continue;
        }
        process(parsed_, parsed);
    }
}

void Audio::process(const adc_continuous_data_t* data, uint32_t count) {
    const int64_t now = esp_timer_get_time();
    counted_ += count;
    const int64_t elapsed = now - started_us_;
    // Measured over at least half a second; the nominal rate until then.
    uint32_t rate = running_rate_;
    if (elapsed > 500000) {
        rate = static_cast<uint32_t>((counted_ * 1000000ULL + static_cast<uint64_t>(elapsed) / 2U) /
                                     static_cast<uint64_t>(elapsed));
        actual_rate_hz_.store(rate, std::memory_order_relaxed);
    }
    const uint32_t level_samples = running_rate_ * kPeriodMs / 1000U;
    const uint32_t band = band_.load(std::memory_order_relaxed);
    if (band != running_band_) configure_band(band, running_rate_);

    size_t n = 0;
    for (uint32_t i = 0; i < count; ++i) {
        if (!data[i].valid || data[i].channel != channel_) continue;
        const float raw = static_cast<float>(data[i].raw_data);
        dc_ += dc_alpha_ * (raw - dc_);
        float x = raw - dc_;
        if (high_pass_on_) x = high_pass_[1].run(high_pass_[0].run(x));
        if (low_pass_on_) x = low_pass_[1].run(low_pass_[0].run(x));

        float s = std::nearbyint(x);
        if (s > kFullScale - 1) s = kFullScale - 1;
        if (s < -kFullScale) s = -kFullScale;
        block_[n++] = static_cast<int16_t>(s);

        level_sum_sq_ += static_cast<double>(x) * x;
        const float magnitude = std::fabs(x);
        if (magnitude > level_peak_) level_peak_ = magnitude;
        if (++level_count_ >= level_samples) finish_level();
    }
    blocks_.fetch_add(1, std::memory_order_relaxed);
    if (n == 0 || on_block_ == nullptr) return;
    // The block's first sample was taken `n` samples before now.
    const uint64_t first_us = static_cast<uint64_t>(now) - static_cast<uint64_t>(n) * 1000000ULL / rate;
    if (on_block_(block_, n, rate, first_us)) blocks_streamed_.fetch_add(1, std::memory_order_relaxed);
}

void Audio::finish_level() {
    const float rms = static_cast<float>(std::sqrt(level_sum_sq_ / level_count_));
    const float dbfs = rms > 0.0f ? 20.0f * std::log10(rms / kFullScaleRms) : kFloorDbfs;
    rms_.store(rms, std::memory_order_relaxed);
    dbfs_.store(dbfs < kFloorDbfs ? kFloorDbfs : dbfs, std::memory_order_relaxed);
    peak_.store(static_cast<uint16_t>(level_peak_ > 2048.0f ? 2048.0f : level_peak_), std::memory_order_relaxed);
    level_sum_sq_ = 0.0;
    level_peak_ = 0.0f;
    level_count_ = 0;
}
