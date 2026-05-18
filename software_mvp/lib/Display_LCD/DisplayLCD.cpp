#include "DisplayLCD.h"
#include <Fonts/TomThumb.h> // Importa a micro-fonte

DisplayLCD::DisplayLCD() : tft(TFT_CS, TFT_DC, TFT_SDA, TFT_SCK, TFT_RST) {
    // constructor
}

DisplayLCD::~DisplayLCD() {
    // destructor
}

void DisplayLCD::begin() {
    // turn on the backlight
    pinMode(TFT_BL, OUTPUT);
    
    // initialize the display and clear the screen with black
    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(ST7735_BLACK);

    // rotate the display if needed (0-3)
    tft.setRotation(rotation);              
    tft.setTextColor(text_color, bg_color);       
    tft.setTextSize(font_size);                     
    tft.setTextWrap(true);                  // turn on text wrapping to avoid text going off the screen
    tft.setCursor(0, 0);                    // set the cursor to the top-left corner of the screen
    tft.setFont(&TomThumb);
    
    // turn on the backlight
    high();
}

void DisplayLCD::high(bool state) {
    // set the backlight pin to the state (HIGH or LOW)
    digitalWrite(TFT_BL, state);
    display_high = state;
}

void DisplayLCD::printWrap(const String& text) {
    // get the current Y position of the cursor
    int16_t posicaoY = tft.getCursorY();
    
    // if the Y position is greater than or equal to 120, clear the screen and reset the cursor to (0,0)
    if (posicaoY >= TFT_HEIGHT - font_offset) { 
        tft.fillScreen(bg_color); 
        tft.setCursor(0, font_offset); // reset the cursor to the top-left corner with an offset for the font height
    }
    
    // print the text on the display, it will automatically wrap to the next line if it exceeds the width of the screen
    tft.print(text);
}


