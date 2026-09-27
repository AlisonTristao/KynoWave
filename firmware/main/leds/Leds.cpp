#include "leds/Leds.h"

#include <driver/ledc.h>
#include <esp_log.h>

#include "board.h"

namespace {

constexpr const char* kTag = "leds";
constexpr ledc_mode_t kMode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t kTimer = LEDC_TIMER_0;
constexpr uint32_t kFullDuty = 255;
// The first two LEDs are much brighter than the rest: dimmed to match.
constexpr uint32_t kDimDuty = kFullDuty / 10;

}  // namespace

esp_err_t Leds::begin() {
    ledc_timer_config_t timer = {};
    timer.speed_mode = kMode;
    timer.timer_num = kTimer;
    timer.duty_resolution = LEDC_TIMER_8_BIT;
    timer.freq_hz = 5000;
    timer.clk_cfg = LEDC_AUTO_CLK;
    esp_err_t err = ledc_timer_config(&timer);
    for (int i = 0; err == ESP_OK && i < board::kLedCount; ++i) {
        ledc_channel_config_t channel = {};
        channel.gpio_num = board::kLeds[i];
        channel.speed_mode = kMode;
        channel.channel = static_cast<ledc_channel_t>(i);
        channel.timer_sel = kTimer;
        channel.duty = 0;
        err = ledc_channel_config(&channel);
    }
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "LEDC setup failed (%s)", esp_err_to_name(err));
        return err;
    }
    ready_ = true;
    apply(0);
    return ESP_OK;
}

void Leds::show_level(float rms) {
    if (!vu()) {
        apply(mask_.load(std::memory_order_relaxed));
        return;
    }
    const float step = static_cast<float>(this->step());
    uint8_t mask = 0;
    for (int i = 0; i < board::kLedCount; ++i) {
        if (rms > i * step) mask |= static_cast<uint8_t>(1U << i);
    }
    apply(mask);
}

void Leds::set_mask(uint8_t mask) {
    mask_.store(mask, std::memory_order_relaxed);
    set_vu(false);
}

void Leds::apply(uint8_t mask) {
    if (!ready_ || mask == shown_) return;
    for (int i = 0; i < board::kLedCount; ++i) {
        const auto channel = static_cast<ledc_channel_t>(i);
        const uint32_t on = i < 2 ? kDimDuty : kFullDuty;
        ledc_set_duty(kMode, channel, (mask & (1U << i)) != 0U ? on : 0U);
        ledc_update_duty(kMode, channel);
    }
    shown_ = mask;
}
