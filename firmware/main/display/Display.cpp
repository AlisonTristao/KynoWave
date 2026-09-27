#include "display/Display.h"

#include <cstdio>
#include <cstring>

#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "board.h"

namespace {

constexpr const char* kTag = "display";
constexpr int kTopBarHeight = 18;
constexpr uint32_t kTickMs = 10;          // LVGL timer handler period
constexpr uint32_t kRefreshMs = 100;      // level and status: 10 Hz
constexpr float kLevelMinDb = -90.0f;

lv_color_t session_color(Display::Session session) {
    switch (session) {
        case Display::Session::Open: return lv_color_hex(0x00FF00);
        case Display::Session::Waiting: return lv_color_hex(0xFFCC00);
        case Display::Session::Failed: return lv_color_hex(0xFF3030);
        default: return lv_color_hex(0xAAAAAA);
    }
}

}  // namespace

esp_err_t Display::begin(int core) {
    lock_ = xSemaphoreCreateMutex();
    if (lock_ == nullptr) return ESP_ERR_NO_MEM;

    gpio_config_t backlight = {};
    backlight.pin_bit_mask = 1ULL << board::kLcdBacklight;
    backlight.mode = GPIO_MODE_OUTPUT;
    gpio_config(&backlight);
    set_backlight(true);

    const size_t buffer_pixels = static_cast<size_t>(St7735::kWidth) * kBufferLines;
    const St7735::Config config = {
        .host = SPI2_HOST,
        .mosi = board::kLcdMosi,
        .sck = board::kLcdSck,
        .cs = board::kLcdCs,
        .dc = board::kLcdDc,
        .reset = board::kLcdReset,
        .pclk_hz = 20 * 1000 * 1000,
        .max_transfer_bytes = static_cast<uint32_t>(buffer_pixels * sizeof(lv_color_t)),
    };
    esp_err_t err = panel_.begin(config, &Display::flush_done, this);
    if (err != ESP_OK) return err;

    // Two DMA-capable buffers: LVGL renders into one while the other goes out.
    auto* buffer1 = static_cast<lv_color_t*>(heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA));
    auto* buffer2 = static_cast<lv_color_t*>(heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_DMA));
    if (buffer1 == nullptr || buffer2 == nullptr) {
        ESP_LOGE(kTag, "no DMA memory for the draw buffers");
        return ESP_ERR_NO_MEM;
    }

    lv_init();
    lv_disp_draw_buf_init(&draw_buf_, buffer1, buffer2, buffer_pixels);
    lv_disp_drv_init(&driver_);
    driver_.hor_res = St7735::kWidth;
    driver_.ver_res = St7735::kHeight;
    driver_.flush_cb = &Display::flush;
    driver_.draw_buf = &draw_buf_;
    driver_.user_data = this;
    lv_disp_drv_register(&driver_);
    build_ui();

    if (xTaskCreatePinnedToCore(&Display::task_entry, "display", 6144, this, 2, &task_, core) != pdPASS) {
        ESP_LOGE(kTag, "could not create the display task");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void Display::set_status(const Status& status) {
    if (xSemaphoreTake(lock_, pdMS_TO_TICKS(5)) != pdTRUE) return;
    status_ = status;
    status_dirty_ = true;
    xSemaphoreGive(lock_);
}

void Display::log(const char* text) {
    if (text == nullptr || lock_ == nullptr) return;
    if (xSemaphoreTake(lock_, pdMS_TO_TICKS(5)) != pdTRUE) return;
    if (line_count_ == kLogLines) {
        std::memmove(lines_[0], lines_[1], sizeof(lines_[0]) * (kLogLines - 1));
        --line_count_;
    }
    std::snprintf(lines_[line_count_++], kLineLength + 1, "%s", text);
    log_dirty_ = true;
    xSemaphoreGive(lock_);
}

void Display::set_backlight(bool on) {
    gpio_set_level(board::kLcdBacklight, on ? 1 : 0);
    backlight_.store(on, std::memory_order_relaxed);
}

// ---- display task --------------------------------------------------------------

void Display::task_entry(void* self) { static_cast<Display*>(self)->run(); }

void Display::run() {
    int64_t last_us = esp_timer_get_time();
    uint32_t since_refresh_ms = kRefreshMs;
    for (;;) {
        const int64_t now_us = esp_timer_get_time();
        const uint32_t elapsed_ms = static_cast<uint32_t>((now_us - last_us) / 1000);
        if (elapsed_ms != 0) {
            lv_tick_inc(elapsed_ms);
            last_us += static_cast<int64_t>(elapsed_ms) * 1000;
            since_refresh_ms += elapsed_ms;
        }
        if (since_refresh_ms >= kRefreshMs) {
            since_refresh_ms = 0;
            refresh_status();
            refresh_level();
            refresh_log();
        }
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(kTickMs));
    }
}

void Display::flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* pixels) {
    auto* self = static_cast<Display*>(driver->user_data);
    if (self->panel_.draw(area->x1, area->y1, area->x2, area->y2, pixels) != ESP_OK) {
        lv_disp_flush_ready(driver);   // drop the frame, never stall LVGL
    }
}

void Display::flush_done(void* context) {
    lv_disp_flush_ready(&static_cast<Display*>(context)->driver_);
}

void Display::build_ui() {
    lv_obj_t* screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);

    // Status bar: Wi-Fi icon and link on the left, uptime on the right.
    lv_obj_t* top = lv_obj_create(screen);
    lv_obj_set_size(top, St7735::kWidth, kTopBarHeight);
    lv_obj_set_pos(top, 0, 0);
    lv_obj_set_style_bg_color(top, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_pad_all(top, 1, 0);
    lv_obj_set_style_radius(top, 0, 0);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    wifi_icon_ = lv_canvas_create(top);
    lv_canvas_set_buffer(wifi_icon_, wifi_canvas_, kWifiIconW, kWifiIconH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(wifi_icon_, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_add_flag(wifi_icon_, LV_OBJ_FLAG_HIDDEN);

    link_label_ = lv_label_create(top);
    lv_obj_set_style_text_font(link_label_, &lv_font_montserrat_12, 0);
    lv_label_set_text(link_label_, "--");
    lv_obj_align(link_label_, LV_ALIGN_LEFT_MID, 2, 0);

    time_label_ = lv_label_create(top);
    lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(time_label_, lv_color_hex(0xEEEEEE), 0);
    lv_label_set_text(time_label_, "00:00:00");
    lv_obj_align(time_label_, LV_ALIGN_RIGHT_MID, -2, 0);

    // Microphone level: dBFS and a bar.
    level_label_ = lv_label_create(screen);
    lv_obj_set_style_text_font(level_label_, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(level_label_, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(level_label_, LV_ALIGN_TOP_MID, 0, kTopBarHeight + 4);

    level_bar_ = lv_bar_create(screen);
    lv_obj_set_size(level_bar_, St7735::kWidth - 8, 10);
    lv_obj_align(level_bar_, LV_ALIGN_TOP_MID, 0, kTopBarHeight + 30);
    lv_bar_set_range(level_bar_, static_cast<int32_t>(kLevelMinDb), 0);
    lv_obj_set_style_bg_color(level_bar_, lv_color_hex(0x00C060), LV_PART_INDICATOR);

    // Log.
    log_label_ = lv_label_create(screen);
    lv_obj_set_width(log_label_, St7735::kWidth - 4);
    lv_obj_set_style_text_font(log_label_, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(log_label_, lv_color_hex(0xCCCCCC), 0);
    lv_label_set_long_mode(log_label_, LV_LABEL_LONG_CLIP);
    lv_label_set_text(log_label_, "");
    lv_obj_align(log_label_, LV_ALIGN_TOP_LEFT, 2, kTopBarHeight + 46);

    lv_scr_load(screen);
}

void Display::refresh_status() {
    Status status;
    bool dirty = false;
    if (xSemaphoreTake(lock_, 0) == pdTRUE) {
        status = status_;
        dirty = status_dirty_;
        status_dirty_ = false;
        xSemaphoreGive(lock_);
    }
    if (dirty) {
        const lv_color_t color = session_color(status.session);
        lv_label_set_text(link_label_, status.link);
        lv_obj_set_style_text_color(link_label_, color, 0);
        if (status.wifi_bars >= 0) {
            draw_wifi_icon(status.wifi_bars, color);
            lv_obj_clear_flag(wifi_icon_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(link_label_, LV_ALIGN_LEFT_MID, kWifiIconW + 6, 0);
        } else {
            lv_obj_add_flag(wifi_icon_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_align(link_label_, LV_ALIGN_LEFT_MID, 2, 0);
        }
    }
    // No clock on this board without Wi-Fi/SNTP: show the uptime.
    const uint32_t seconds = static_cast<uint32_t>(esp_timer_get_time() / 1000000LL);
    char text[16];
    std::snprintf(text, sizeof(text), "%02lu:%02lu:%02lu", static_cast<unsigned long>(seconds / 3600U),
                  static_cast<unsigned long>((seconds / 60U) % 60U), static_cast<unsigned long>(seconds % 60U));
    lv_label_set_text(time_label_, text);
}

void Display::refresh_level() {
    const float dbfs = dbfs_.load(std::memory_order_relaxed);
    const float rounded = static_cast<float>(static_cast<int>(dbfs * 10.0f)) / 10.0f;
    if (rounded == shown_dbfs_) return;
    shown_dbfs_ = rounded;
    char text[16];
    std::snprintf(text, sizeof(text), "%.1f dBFS", static_cast<double>(rounded));
    lv_label_set_text(level_label_, text);
    lv_bar_set_value(level_bar_, static_cast<int32_t>(dbfs), LV_ANIM_OFF);
}

void Display::refresh_log() {
    char text[kLogLines * (kLineLength + 1) + 1];
    size_t used = 0;
    bool dirty = false;
    if (xSemaphoreTake(lock_, 0) == pdTRUE) {
        dirty = log_dirty_;
        log_dirty_ = false;
        for (int i = 0; dirty && i < line_count_; ++i) {
            const int n = std::snprintf(text + used, sizeof(text) - used, i == 0 ? "%s" : "\n%s", lines_[i]);
            if (n > 0) used += static_cast<size_t>(n);
        }
        xSemaphoreGive(lock_);
    }
    if (!dirty) return;
    text[used] = '\0';
    lv_label_set_text(log_label_, text);
}

void Display::draw_wifi_icon(int8_t bars, lv_color_t color) {
    // Four bars of rising height, the lit ones in the session color (from the MVP).
    lv_canvas_fill_bg(wifi_icon_, lv_color_hex(0x111111), LV_OPA_COVER);
    const lv_coord_t bar_w = 4;
    const lv_coord_t bar_gap = 1;
    const lv_coord_t heights[4] = {4, 6, 8, 10};
    lv_draw_rect_dsc_t rect;
    lv_draw_rect_dsc_init(&rect);
    rect.radius = 0;
    rect.border_width = 0;
    rect.bg_opa = LV_OPA_COVER;
    const int lit = bars < 1 ? 1 : (bars > 4 ? 4 : bars);
    for (int i = 0; i < 4; ++i) {
        rect.bg_color = i < lit ? color : lv_color_hex(0x555555);
        lv_canvas_draw_rect(wifi_icon_, i * (bar_w + bar_gap), kWifiIconH - heights[i], bar_w, heights[i], &rect);
    }
}
