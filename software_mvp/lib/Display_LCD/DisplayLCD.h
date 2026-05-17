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
    private:
        Adafruit_ST7735 tft;
        bool display_high           = false;            // flag to track if the display is currently highlighted
        const uint8_t font_size     = 1;                // default font size
        const uint16_t text_color   = ST7735_GREEN;     // colot with matrix estile
        const uint16_t bg_color     = ST7735_BLACK;     // background color
        const uint8_t rotation      = 3;                // horizontal rotation (0-3)
};


#endif // DISPLAYLCD_H