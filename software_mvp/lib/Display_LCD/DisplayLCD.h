#ifndef DISPLAYLCD_H
#define DISPLAYLCD_H

// header
#include <Settings.h>

// external libraries
#include <Arduino.h>
#include <TFT_eSPI.h>

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

    private:
        void ensureMetrics();
        void newLine();
        void resetCursor();

        TFT_eSPI tft;
        bool display_high_ = false;
        bool initialized_ = false;
        bool clear_terminal_pending_ = false;
        uint16_t text_color_ = TFT_GREEN;
        uint16_t bg_color_ = TFT_BLACK;
        uint8_t rotation_ = 3;

        int16_t cursor_x_ = 0;
        int16_t cursor_y_ = 0;
        int16_t char_w_ = 0;
        int16_t line_h_ = 0;
        uint8_t padding_ = 1;
};


#endif // DISPLAYLCD_H