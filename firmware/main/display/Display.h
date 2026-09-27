// The LCD: LVGL 8 on the ST7735, ported from the MVP's DisplayLCD layout --
// a status bar (Wi-Fi icon, link, uptime), the microphone level and a log.
//
// Only the display task touches LVGL. Other tasks hand it data through the
// setters below (a lock held for a copy, or an atomic); none of them blocks
// for long, none of them draws.
#pragma once

#include <atomic>
#include <cstdint>

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <lvgl.h>

#include "display/St7735.h"

class Display {
public:
    enum class Session : uint8_t { Off, Waiting, Open, Failed };

    struct Status {
        char link[10] = "--";       // "USB", "TCP", "BLE", "ESP-NOW"
        Session session = Session::Off;
        int8_t wifi_bars = -1;      // 0..4 on a Wi-Fi link, -1: no Wi-Fi icon
    };

    esp_err_t begin(int core);

    void set_status(const Status& status);
    void set_level(float dbfs) { dbfs_.store(dbfs, std::memory_order_relaxed); }
    /// A line for the log area (cut to kLineLength). Any task.
    void log(const char* text);
    void set_backlight(bool on);
    bool backlight() const { return backlight_.load(std::memory_order_relaxed); }

private:
    static constexpr int kBufferLines = 16;
    static constexpr int kLogLines = 6;
    static constexpr int kLineLength = 40;

    static void task_entry(void* self);
    static void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* pixels);
    static void flush_done(void* context);
    void run();
    void build_ui();
    void refresh_status();
    void refresh_level();
    void refresh_log();
    void draw_wifi_icon(int8_t bars, lv_color_t color);

    St7735 panel_;
    TaskHandle_t task_ = nullptr;
    SemaphoreHandle_t lock_ = nullptr;   // status_ and the log ring

    Status status_;                      // guarded by lock_
    bool status_dirty_ = true;           // guarded by lock_
    char lines_[kLogLines][kLineLength + 1] = {};   // guarded by lock_
    int line_count_ = 0;                 // guarded by lock_
    bool log_dirty_ = false;             // guarded by lock_

    std::atomic<float> dbfs_{-90.0f};
    std::atomic<bool> backlight_{true};

    // LVGL objects (display task only).
    lv_disp_draw_buf_t draw_buf_{};
    lv_disp_drv_t driver_{};
    lv_obj_t* link_label_ = nullptr;
    lv_obj_t* time_label_ = nullptr;
    lv_obj_t* wifi_icon_ = nullptr;
    lv_obj_t* level_label_ = nullptr;
    lv_obj_t* level_bar_ = nullptr;
    lv_obj_t* log_label_ = nullptr;
    static constexpr int kWifiIconW = 22;
    static constexpr int kWifiIconH = 12;
    lv_color_t wifi_canvas_[kWifiIconW * kWifiIconH] = {};
    float shown_dbfs_ = 1.0f;            // forces the first draw
};
