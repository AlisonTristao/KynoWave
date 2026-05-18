#ifndef DISPLAYLCD_H
#define DISPLAYLCD_H

// external libraries
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// header
#include <Settings.h>

class DisplayLCD;

struct PixelRect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};

class DisplayWidget {
    public:
        DisplayWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span);
        virtual ~DisplayWidget() = default;

        virtual void init(DisplayLCD& display) {}
        virtual void update(DisplayLCD& display) {}
        virtual void draw(DisplayLCD& display) = 0;

        uint8_t col() const { return col_; }
        uint8_t row() const { return row_; }
        uint8_t colSpan() const { return col_span_; }
        uint8_t rowSpan() const { return row_span_; }

    private:
        uint8_t col_;
        uint8_t row_;
        uint8_t col_span_;
        uint8_t row_span_;
};

enum class WifiState : uint8_t {
    Disconnected = 0,
    Connecting,
    Connected
};

class WifiWidget : public DisplayWidget {
    public:
        WifiWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span);
        bool setState(WifiState state, uint8_t strength);
        void draw(DisplayLCD& display) override;

    private:
        WifiState state_ = WifiState::Disconnected;
        uint8_t strength_ = 0;
};

class MqttWidget : public DisplayWidget {
    public:
        MqttWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span);
        bool setConnected(bool connected);
        void draw(DisplayLCD& display) override;

    private:
        bool connected_ = false;
};

class DateTimeWidget : public DisplayWidget {
    public:
        DateTimeWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span);
        bool setDateTime(const String& time_text, const String& date_text);
        void draw(DisplayLCD& display) override;

    private:
        String time_text_ = "--:--";
        String date_text_ = "--/--/----";
};


class TerminalWidget : public DisplayWidget {
    public:
        TerminalWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span);
        void init(DisplayLCD& display) override;
        void draw(DisplayLCD& display) override;
        void appendText(DisplayLCD& display, const String& text);
        void clear(DisplayLCD& display);
        void resetCursor(DisplayLCD& display);

    private:
        void newLine(DisplayLCD& display);
        void ensureMetrics(DisplayLCD& display);

        int16_t cursor_x_ = 0;
        int16_t cursor_y_ = 0;
        int16_t char_w_ = 0;
        int16_t line_h_ = 0;
        int16_t baseline_offset_ = 0;
        uint8_t padding_ = 1;
        bool initialized_ = false;
};

class DisplayLCD {
    public:
        DisplayLCD(uint8_t grid_cols = 4, uint8_t grid_rows = 4, uint16_t cell_width = 32, uint16_t cell_height = 40);
        ~DisplayLCD();

        void begin();
        void update();
        void high(bool state = HIGH);
        void printWrap(const String& text);
        void clear();
        void clearTerminal();
        void requestTerminalClear();

        PixelRect widgetRect(const DisplayWidget& widget) const;
        Adafruit_ST7735& driver() { return tft; }
        uint16_t textColor() const { return text_color; }
        uint16_t bgColor() const { return bg_color; }

        bool setWiFiState(WifiState state, uint8_t strength);
        bool setMqttConnected(bool connected);
        bool setDateTime(const String& time_text, const String& date_text);

    private:
        void initWidgets();
        void drawWidgets();
        bool addWidget(DisplayWidget* widget);
        void syncGridToDisplay();

        Adafruit_ST7735 tft;
        bool display_high           = false;
        bool initialized_           = false;
        bool clear_terminal_pending_ = false;
        const uint8_t font_size     = 1;
        const uint16_t text_color   = ST7735_GREEN;
        const uint16_t bg_color     = ST7735_BLACK;
        const uint8_t rotation      = 3;
        uint8_t border_px_          = 2;

        uint8_t grid_cols_          = 4;
        uint8_t grid_rows_          = 4;
        uint16_t cell_width_        = 32;
        uint16_t cell_height_       = 40;

        static constexpr uint8_t kMaxWidgets = 8;
        DisplayWidget* widgets_[kMaxWidgets] = {nullptr};
        uint8_t widget_count_ = 0;

        WifiWidget wifi_widget_;
        MqttWidget mqtt_widget_;
        DateTimeWidget datetime_widget_;
        TerminalWidget terminal_widget_;
};


#endif // DISPLAYLCD_H