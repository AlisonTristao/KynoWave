#ifndef DISPLAYLCD_H
#define DISPLAYLCD_H

// external libraries
#include <Arduino.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7735.h>
#include <SPI.h>

// header
#include <Settings.h>

class DisplayLCD {
    public:
        DisplayLCD();
        ~DisplayLCD();

        void begin();
        void high(bool state = HIGH);
        void printWrap(const String& text);
        void clear() {
            tft.fillScreen(bg_color);
            tft.setCursor(0, font_offset);
        }
    private:
        Adafruit_ST7735 tft;
        bool display_high           = false;            // flag to track if the display is currently highlighted
        const uint8_t font_size     = 1;                // default font size
        const uint16_t text_color   = ST7735_GREEN;     // colot with matrix estile
        const uint16_t bg_color     = ST7735_BLACK;     // background color
        const uint8_t rotation      = 3;                // horizontal rotation (0-3)
        const uint8_t font_offset   = 12;                // offset in pixels to move the text down, to avoid overlapping with the top of the screen
};


#endif // DISPLAYLCD_H