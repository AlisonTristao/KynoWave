#include "DisplayLCD.h"

DisplayLCD::DisplayLCD()
    : tft() {
}

DisplayLCD::~DisplayLCD() {
    // destructor
}

void DisplayLCD::begin() {
    pinMode(TFT_BL, OUTPUT);

    tft.init();
    tft.setRotation(rotation_);
    tft.fillScreen(bg_color_);
    tft.setTextColor(text_color_, bg_color_);
    tft.setTextSize(1);
    tft.setTextWrap(false);

    ensureMetrics();
    resetCursor();
    tft.setCursor(cursor_x_, cursor_y_);
    tft.print("TFT_eSPI OK");
    newLine();

    initialized_ = true;
    high();
}

void DisplayLCD::update() {
    if (!initialized_)
        return;

    if (clear_terminal_pending_) {
        clear_terminal_pending_ = false;
        clearTerminal();
    }
}

void DisplayLCD::high(bool state) {
    digitalWrite(TFT_BL, state);
    display_high_ = state;
}

void DisplayLCD::printWrap(const String& text) {
    if (!initialized_)
        return;

    ensureMetrics();
    int16_t max_x = static_cast<int16_t>(tft.width()) - padding_;
    int16_t max_y = static_cast<int16_t>(tft.height()) - padding_;

    for (size_t i = 0; i < text.length(); ++i) {
        char ch = text[i];

        if (ch == '\r')
            continue;

        if (ch == '\n') {
            newLine();
            continue;
        }

        if (cursor_x_ + char_w_ > max_x) {
            newLine();
        }

        if (cursor_y_ + line_h_ > max_y) {
            clearTerminal();
        }

        tft.setCursor(cursor_x_, cursor_y_);
        tft.print(ch);
        cursor_x_ += char_w_;

        if ((i & 0x1F) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}

void DisplayLCD::clear() {
    tft.fillScreen(bg_color_);
    resetCursor();
}

void DisplayLCD::clearTerminal() {
    clear();
}

void DisplayLCD::requestTerminalClear() {
    clear_terminal_pending_ = true;
}

void DisplayLCD::ensureMetrics() {
    if (char_w_ > 0 && line_h_ > 0)
        return;

    tft.setTextSize(1);
    char_w_ = static_cast<int16_t>(tft.textWidth("M"));
    line_h_ = static_cast<int16_t>(tft.fontHeight(1));

    if (char_w_ <= 0) char_w_ = 6;
    if (line_h_ <= 0) line_h_ = 8;
}

void DisplayLCD::newLine() {
    cursor_x_ = padding_;
    cursor_y_ += line_h_;
}

void DisplayLCD::resetCursor() {
    cursor_x_ = padding_;
    cursor_y_ = padding_;
}


