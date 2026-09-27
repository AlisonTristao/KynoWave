#include "audio/Audio.h"

#include <cmath>
#include <cstdlib>

#include <esp_log.h>
#include <esp_timer.h>

namespace {

constexpr const char* kTag = "audio";
constexpr float kPi = 3.14159265f;
constexpr float kLowPassHz = 3000.0f;
constexpr float kDcAlpha = 0.001f;   // DC tracker: ~1000 samples

}  // namespace

esp_err_t Audio::begin(gpio_num_t pin, int core) {
    adc_unit_t unit_id = ADC_UNIT_1;
    esp_err_t err = adc_oneshot_io_to_channel(pin, &unit_id, &channel_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "GPIO%d is not an ADC pin", pin);
        return err;
    }
    adc_oneshot_unit_init_cfg_t unit_config = {};
    unit_config.unit_id = unit_id;
    err = adc_oneshot_new_unit(&unit_config, &unit_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "ADC%d init failed (%s)", unit_id + 1, esp_err_to_name(err));
        return err;
    }
    adc_oneshot_chan_cfg_t channel_config = {};
    channel_config.atten = ADC_ATTEN_DB_12;       // full 0..3.3 V swing of the MAX4466
    channel_config.bitwidth = ADC_BITWIDTH_12;
    err = adc_oneshot_config_channel(unit_, channel_, &channel_config);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "ADC channel config failed (%s)", esp_err_to_name(err));
        return err;
    }
    if (xTaskCreatePinnedToCore(&Audio::task_entry, "audio", 4096, this, 5, &task_, core) != pdPASS) {
        ESP_LOGE(kTag, "could not create the audio task");
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(kTag, "microphone on GPIO%d (ADC%d channel %d), %lu Hz, %u-sample blocks every %lu ms",
             pin, unit_id + 1, channel_, static_cast<unsigned long>(kDefaultRateHz), kBlockSamples,
             static_cast<unsigned long>(kPeriodMs));
    return ESP_OK;
}

bool Audio::set_rate(uint32_t hz) {
    if (hz < kMinRateHz || hz > kMaxRateHz) return false;
    rate_hz_.store(hz, std::memory_order_relaxed);
    return true;
}

Audio::Stats Audio::stats() const {
    return Stats{rate_hz_.load(std::memory_order_relaxed), actual_rate_hz_.load(std::memory_order_relaxed),
                 blocks_.load(std::memory_order_relaxed), read_errors_.load(std::memory_order_relaxed),
                 block_us_.load(std::memory_order_relaxed)};
}

void Audio::task_entry(void* self) { static_cast<Audio*>(self)->run(); }

void Audio::run() {
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        read_block();
        // Sleep the rest of the period: the core is free until the next block.
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(kPeriodMs));
    }
}

void Audio::read_block() {
    const uint32_t rate = rate_hz_.load(std::memory_order_relaxed);
    const float dt = 1.0f / static_cast<float>(rate);
    const float rc = 1.0f / (2.0f * kPi * kLowPassHz);
    const float alpha = dt / (rc + dt);

    double sum_sq = 0.0;
    float peak = 0.0f;
    uint16_t good = 0;
    const int64_t start = esp_timer_get_time();

    for (uint32_t i = 0; i < kBlockSamples; ++i) {
        // Pace each sample at its own instant (no drift across the block).
        const int64_t due = start + static_cast<int64_t>(i) * 1000000LL / rate;
        while (esp_timer_get_time() < due) {
        }
        int raw = 0;
        if (adc_oneshot_read(unit_, channel_, &raw) != ESP_OK) {
            read_errors_.fetch_add(1, std::memory_order_relaxed);   // ADC2 taken by Wi-Fi
            continue;
        }
        dc_ += kDcAlpha * (static_cast<float>(raw) - dc_);
        const float x = static_cast<float>(raw) - dc_;
        lp1_ += alpha * (x - lp1_);
        lp2_ += alpha * (lp1_ - lp2_);
        lp3_ += alpha * (lp2_ - lp3_);
        sum_sq += static_cast<double>(lp3_) * lp3_;
        const float magnitude = std::fabs(lp3_);
        if (magnitude > peak) peak = magnitude;
        ++good;
    }

    const int64_t elapsed = esp_timer_get_time() - start;
    block_us_.store(static_cast<uint32_t>(elapsed), std::memory_order_relaxed);
    if (elapsed > 0) {
        actual_rate_hz_.store(static_cast<uint32_t>(kBlockSamples * 1000000LL / elapsed),
                              std::memory_order_relaxed);
    }
    if (good == 0) return;   // keep the last level

    const float rms = static_cast<float>(std::sqrt(sum_sq / good));
    const float dbfs = rms > 0.0f ? 20.0f * std::log10(rms / kFullScaleRms) : kFloorDbfs;
    rms_.store(rms, std::memory_order_relaxed);
    dbfs_.store(dbfs < kFloorDbfs ? kFloorDbfs : dbfs, std::memory_order_relaxed);
    peak_.store(static_cast<uint16_t>(peak > 2048.0f ? 2048.0f : peak), std::memory_order_relaxed);
    blocks_.fetch_add(1, std::memory_order_relaxed);
}
