#ifndef DISPLAYLCD_H
#define DISPLAYLCD_H

// header
#include <Settings.h>

// external libraries
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

enum class WifiState : uint8_t {
    Disconnected = 0,
    Connecting,
    Connected
};

class DisplayLCD {
    public:
        DisplayLCD();
        ~DisplayLCD();

        void begin();
        void update();
        void high(bool state = HIGH);
        void printWrap(const String& text);
        void clear();
        void clearTerminal();
        void requestTerminalClear();

        void updateStatus(const char* time_text,
                  WifiState wifi_state,
                  uint8_t wifi_strength,
                  bool mqtt_connected);

    private:
        static void flushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);

        void initLvgl();
        void initUi();
        void appendLogText(const String& text);
        void pushLogLine(const String& line);
        void refreshLogLabel();
        void updateStatusLabels();

        TFT_eSPI tft;
        bool display_high_ = false;
        bool initialized_ = false;
        bool lvgl_ready_ = false;
        bool clear_terminal_pending_ = false;
        uint16_t text_color_ = TFT_GREEN;
        uint16_t bg_color_ = TFT_BLACK;
        uint8_t rotation_ = 3;

        uint16_t display_w_ = 0;
        uint16_t display_h_ = 0;
        uint32_t last_tick_ms_ = 0;

        static constexpr uint8_t kTopBarHeight = 18;
        static constexpr uint8_t kLogLineCount = 16;
        static constexpr uint8_t kWifiIconW = 22;
        static constexpr uint8_t kWifiIconH = 12;
        static constexpr uint8_t kBufferLines = 20;
        static constexpr uint16_t kMaxWidth = (TFT_WIDTH > TFT_HEIGHT) ? TFT_WIDTH : TFT_HEIGHT;
        static constexpr size_t kBufferPixels = static_cast<size_t>(kMaxWidth) * kBufferLines;

        lv_disp_draw_buf_t draw_buf_;
        lv_disp_drv_t disp_drv_;
        lv_color_t draw_buf1_[kBufferPixels];

        lv_obj_t* screen_ = nullptr;
        lv_obj_t* top_bar_ = nullptr;
        lv_obj_t* status_left_ = nullptr;
        lv_obj_t* time_label_ = nullptr;
        lv_obj_t* wifi_icon_ = nullptr;
        lv_obj_t* mqtt_label_ = nullptr;
        lv_obj_t* content_log_ = nullptr;
        lv_obj_t* log_label_ = nullptr;

        String log_lines_[kLogLineCount];
        uint8_t log_line_count_ = 0;

        lv_color_t wifi_canvas_buf_[static_cast<size_t>(kWifiIconW) * kWifiIconH];

        char time_text_[24] = "--/-- --:--";
        WifiState wifi_state_ = WifiState::Disconnected;
        uint8_t wifi_strength_ = 0;
        bool mqtt_connected_ = false;
};


#endif // DISPLAYLCD_H