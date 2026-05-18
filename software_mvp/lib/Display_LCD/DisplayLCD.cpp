#include "DisplayLCD.h"
#include <Fonts/TomThumb.h>

DisplayWidget::DisplayWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span)
    : col_(col),
      row_(row),
      col_span_(col_span),
      row_span_(row_span) {}

WifiWidget::WifiWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span)
    : DisplayWidget(col, row, col_span, row_span) {}

bool WifiWidget::setState(WifiState state, uint8_t strength) {
    bool changed = (state_ != state) || (strength_ != strength);
    state_ = state;
    strength_ = strength;
    return changed;
}

void WifiWidget::draw(DisplayLCD& display) {
    PixelRect rect = display.widgetRect(*this);
    Adafruit_ST7735& tft = display.driver();

    tft.fillRect(rect.x, rect.y, rect.w, rect.h, display.bgColor());

    uint16_t active_color = (state_ == WifiState::Connected) ? ST7735_GREEN : ST7735_YELLOW;
    uint16_t outline_color = (state_ == WifiState::Disconnected) ? ST7735_RED : ST7735_YELLOW;

    const uint8_t bar_count = 4;
    int16_t bar_w = rect.w / (bar_count * 2);
    if (bar_w < 2) bar_w = 2;
    int16_t gap = bar_w;
    int16_t total_w = bar_w * bar_count + gap * (bar_count - 1);
    int16_t start_x = rect.x + (rect.w - total_w) / 2;
    int16_t base_y = rect.y + rect.h - 2;
    int16_t max_bar_h = rect.h - 6;
    if (max_bar_h < 6) max_bar_h = 6;

    for (uint8_t i = 0; i < bar_count; ++i) {
        int16_t bar_h = (max_bar_h * (i + 1)) / bar_count;
        int16_t x = start_x + i * (bar_w + gap);
        int16_t y = base_y - bar_h;

        if (state_ == WifiState::Connected && i < strength_) {
            tft.fillRect(x, y, bar_w, bar_h, active_color);
        } else if (state_ == WifiState::Connecting && i == 0) {
            tft.fillRect(x, y, bar_w, bar_h, active_color);
            tft.drawRect(x, y, bar_w, bar_h, outline_color);
        } else {
            tft.drawRect(x, y, bar_w, bar_h, outline_color);
        }
    }

    if (state_ == WifiState::Disconnected) {
        tft.drawLine(rect.x + 2, rect.y + 2, rect.x + rect.w - 3, rect.y + rect.h - 3, ST7735_RED);
        tft.drawLine(rect.x + 2, rect.y + rect.h - 3, rect.x + rect.w - 3, rect.y + 2, ST7735_RED);
    }
}

MqttWidget::MqttWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span)
    : DisplayWidget(col, row, col_span, row_span) {}

bool MqttWidget::setConnected(bool connected) {
    bool changed = (connected_ != connected);
    connected_ = connected;
    return changed;
}

void MqttWidget::draw(DisplayLCD& display) {
    PixelRect rect = display.widgetRect(*this);
    Adafruit_ST7735& tft = display.driver();

    tft.fillRect(rect.x, rect.y, rect.w, rect.h, display.bgColor());
    uint16_t accent_color = connected_ ? ST7735_GREEN : ST7735_RED;

    int16_t pad = 2;
    int16_t box_x = rect.x + pad;
    int16_t box_y = rect.y + pad;
    int16_t box_w = rect.w - pad * 2;
    int16_t box_h = rect.h - pad * 2;
    tft.drawRoundRect(box_x, box_y, box_w, box_h, 3, accent_color);

    tft.setFont(nullptr);
    tft.setTextSize(1);
    tft.setTextColor(display.textColor(), display.bgColor());

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    tft.getTextBounds("MQTT", 0, 0, &x1, &y1, &w, &h);
    int16_t text_x = rect.x + (rect.w - static_cast<int16_t>(w)) / 2;
    int16_t text_y = rect.y + 7 - y1;
    tft.setCursor(text_x, text_y);
    tft.print("MQTT");

    int16_t line_y = rect.y + rect.h - 8;
    int16_t line_x1 = rect.x + 6;
    int16_t line_x2 = rect.x + rect.w - 7;
    tft.drawLine(line_x1, line_y, line_x2, line_y, accent_color);

    int16_t badge_x = rect.x + rect.w - 10;
    int16_t badge_y = rect.y + rect.h - 10;
    tft.fillCircle(badge_x, badge_y, 3, accent_color);
}

DateTimeWidget::DateTimeWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span)
    : DisplayWidget(col, row, col_span, row_span) {}

bool DateTimeWidget::setDateTime(const String& time_text, const String& date_text) {
    bool changed = (time_text_ != time_text) || (date_text_ != date_text);
    time_text_ = time_text;
    date_text_ = date_text;
    return changed;
}

void DateTimeWidget::draw(DisplayLCD& display) {
    PixelRect rect = display.widgetRect(*this);
    Adafruit_ST7735& tft = display.driver();

    tft.fillRect(rect.x, rect.y, rect.w, rect.h, display.bgColor());
    tft.setFont(nullptr);
    tft.setTextColor(display.textColor(), display.bgColor());

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;

    tft.setTextSize(2);
    tft.getTextBounds(time_text_.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t time_x = rect.x + (rect.w - static_cast<int16_t>(w)) / 2;
    int16_t time_y = rect.y + 4 - y1;
    int16_t time_h = static_cast<int16_t>(h);
    tft.setCursor(time_x, time_y);
    tft.print(time_text_);

    tft.setTextSize(1);
    tft.getTextBounds(date_text_.c_str(), 0, 0, &x1, &y1, &w, &h);
    int16_t date_x = rect.x + (rect.w - static_cast<int16_t>(w)) / 2;
    int16_t date_y = time_y + time_h + 4 - y1;
    tft.setCursor(date_x, date_y);
    tft.print(date_text_);
}

TerminalWidget::TerminalWidget(uint8_t col, uint8_t row, uint8_t col_span, uint8_t row_span)
    : DisplayWidget(col, row, col_span, row_span) {}

void TerminalWidget::ensureMetrics(DisplayLCD& display) {
    if (initialized_ && char_w_ > 0 && line_h_ > 0)
        return;

    Adafruit_ST7735& tft = display.driver();
    tft.setFont(&TomThumb);
    tft.setTextSize(1);

    int16_t x1 = 0;
    int16_t y1 = 0;
    uint16_t w = 0;
    uint16_t h = 0;
    tft.getTextBounds("M", 0, 0, &x1, &y1, &w, &h);

    char_w_ = (w == 0) ? 4 : static_cast<int16_t>(w);
    line_h_ = (h == 0) ? 6 : static_cast<int16_t>(h + 1);
    baseline_offset_ = (y1 < 0) ? static_cast<int16_t>(-y1) : 0;
    initialized_ = true;
}

void TerminalWidget::init(DisplayLCD& display) {
    ensureMetrics(display);
    clear(display);
    resetCursor(display);
}

void TerminalWidget::draw(DisplayLCD& display) {
    clear(display);
    resetCursor(display);
}

void TerminalWidget::clear(DisplayLCD& display) {
    PixelRect rect = display.widgetRect(*this);
    display.driver().fillRect(rect.x, rect.y, rect.w, rect.h, display.bgColor());
}

void TerminalWidget::resetCursor(DisplayLCD& display) {
    PixelRect rect = display.widgetRect(*this);
    cursor_x_ = rect.x + padding_;
    cursor_y_ = rect.y + padding_ + baseline_offset_;
}

void TerminalWidget::newLine(DisplayLCD& display) {
    PixelRect rect = display.widgetRect(*this);
    cursor_x_ = rect.x + padding_;
    cursor_y_ += line_h_;

    if (cursor_y_ + line_h_ > rect.y + rect.h - padding_) {
        clear(display);
        cursor_y_ = rect.y + padding_ + baseline_offset_;
    }
}

void TerminalWidget::appendText(DisplayLCD& display, const String& text) {
    ensureMetrics(display);

    Adafruit_ST7735& tft = display.driver();
    tft.setFont(&TomThumb);
    tft.setTextSize(1);
    tft.setTextColor(display.textColor(), display.bgColor());
    tft.setTextWrap(false);

    PixelRect rect = display.widgetRect(*this);
    int16_t max_x = rect.x + rect.w - padding_;

    for (size_t i = 0; i < text.length(); ++i) {
        char ch = text[i];

        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            newLine(display);
            continue;
        }

        if (cursor_x_ + char_w_ > max_x) {
            newLine(display);
        }

        tft.setCursor(cursor_x_, cursor_y_);
        tft.print(ch);
        cursor_x_ += char_w_;
    }
}

DisplayLCD::DisplayLCD(uint8_t grid_cols, uint8_t grid_rows, uint16_t cell_width, uint16_t cell_height)
        : tft(TFT_CS, TFT_DC, TFT_SDA, TFT_SCK, TFT_RST),
            grid_cols_(grid_cols),
            grid_rows_(grid_rows),
            cell_width_(cell_width),
            cell_height_(cell_height),
            wifi_widget_(0, 0, 1, 1),
            mqtt_widget_(1, 0, 1, 1),
            datetime_widget_(2, 0, 2, 1),
            terminal_widget_(0, 1, 3, 3) {
    addWidget(&wifi_widget_);
    addWidget(&mqtt_widget_);
    addWidget(&datetime_widget_);
    addWidget(&terminal_widget_);
}

DisplayLCD::~DisplayLCD() {
    // destructor
}

void DisplayLCD::begin() {
    pinMode(TFT_BL, OUTPUT);

    tft.initR(INITR_BLACKTAB);
    tft.fillScreen(bg_color);

    tft.setRotation(rotation);
    tft.setTextColor(text_color, bg_color);
    tft.setTextSize(font_size);
    tft.setTextWrap(false);
    tft.setCursor(0, 0);
    tft.setFont(&TomThumb);

    syncGridToDisplay();

    initWidgets();
    drawWidgets();

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

    for (uint8_t i = 0; i < widget_count_; ++i) {
        if (widgets_[i] != nullptr)
            widgets_[i]->update(*this);
    }
}

void DisplayLCD::high(bool state) {
    digitalWrite(TFT_BL, state);
    display_high = state;
}

void DisplayLCD::printWrap(const String& text) {
    if (!initialized_)
        return;

    terminal_widget_.appendText(*this, text);
}

void DisplayLCD::clear() {
    tft.fillScreen(bg_color);
    terminal_widget_.resetCursor(*this);
}

void DisplayLCD::clearTerminal() {
    terminal_widget_.clear(*this);
    terminal_widget_.resetCursor(*this);
}

void DisplayLCD::requestTerminalClear() {
    clear_terminal_pending_ = true;
}

PixelRect DisplayLCD::widgetRect(const DisplayWidget& widget) const {
    PixelRect rect;
    rect.x = static_cast<int16_t>(border_px_ + widget.col() * cell_width_);
    rect.y = static_cast<int16_t>(border_px_ + widget.row() * cell_height_);
    rect.w = static_cast<int16_t>(widget.colSpan() * cell_width_);
    rect.h = static_cast<int16_t>(widget.rowSpan() * cell_height_);
    return rect;
}

bool DisplayLCD::setWiFiState(WifiState state, uint8_t strength) {
    bool changed = wifi_widget_.setState(state, strength);
    if (initialized_ && changed)
        wifi_widget_.draw(*this);
    return changed;
}

bool DisplayLCD::setMqttConnected(bool connected) {
    bool changed = mqtt_widget_.setConnected(connected);
    if (initialized_ && changed)
        mqtt_widget_.draw(*this);
    return changed;
}

bool DisplayLCD::setDateTime(const String& time_text, const String& date_text) {
    bool changed = datetime_widget_.setDateTime(time_text, date_text);
    if (initialized_ && changed)
        datetime_widget_.draw(*this);
    return changed;
}

void DisplayLCD::initWidgets() {
    for (uint8_t i = 0; i < widget_count_; ++i) {
        if (widgets_[i] != nullptr)
            widgets_[i]->init(*this);
    }
}

void DisplayLCD::drawWidgets() {
    for (uint8_t i = 0; i < widget_count_; ++i) {
        if (widgets_[i] != nullptr)
            widgets_[i]->draw(*this);
    }
}

void DisplayLCD::syncGridToDisplay() {
    if (grid_cols_ == 0 || grid_rows_ == 0)
        return;

    uint16_t display_w = static_cast<uint16_t>(tft.width());
    uint16_t display_h = static_cast<uint16_t>(tft.height());
    uint16_t border = static_cast<uint16_t>(border_px_ * 2);
    uint16_t inner_w = (display_w > border) ? static_cast<uint16_t>(display_w - border) : display_w;
    uint16_t inner_h = (display_h > border) ? static_cast<uint16_t>(display_h - border) : display_h;
    uint16_t expected_w = static_cast<uint16_t>(grid_cols_ * cell_width_);
    uint16_t expected_h = static_cast<uint16_t>(grid_rows_ * cell_height_);

    if (expected_w != inner_w || expected_h != inner_h) {
        cell_width_ = static_cast<uint16_t>(inner_w / grid_cols_);
        cell_height_ = static_cast<uint16_t>(inner_h / grid_rows_);
    }
}

bool DisplayLCD::addWidget(DisplayWidget* widget) {
    if (widget == nullptr || widget_count_ >= kMaxWidgets)
        return false;

    widgets_[widget_count_++] = widget;
    return true;
}


