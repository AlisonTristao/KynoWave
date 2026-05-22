#include "DisplayLCD.h"
#include <cstdio>
#include <cstring>

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
    tft.setSwapBytes(true);
    tft.fillScreen(bg_color_);

    display_w_ = static_cast<uint16_t>(tft.width());
    display_h_ = static_cast<uint16_t>(tft.height());

    initLvgl();
    initUi();

    initialized_ = true;
    high();
}

void DisplayLCD::update() {
    if (!initialized_ || !lvgl_ready_)
        return;

    if (clear_terminal_pending_) {
        clear_terminal_pending_ = false;
        clearTerminal();
    }

    uint32_t now = millis();
    uint32_t diff = now - last_tick_ms_;
    last_tick_ms_ = now;
    lv_tick_inc(diff);
    lv_timer_handler();
}

void DisplayLCD::high(bool state) {
    digitalWrite(TFT_BL, state);
    display_high_ = state;
}

void DisplayLCD::printWrap(const String& text) {
    if (!initialized_)
        return;

    appendLogText(text);
}

void DisplayLCD::clear() {
    clearTerminal();
}

void DisplayLCD::clearTerminal() {
    log_line_count_ = 0;
    for (uint8_t i = 0; i < kLogLineCount; ++i) {
        log_lines_[i] = "";
    }
    refreshLogLabel();
}

void DisplayLCD::requestTerminalClear() {
    clear_terminal_pending_ = true;
}

void DisplayLCD::updateStatus(const char* time_text,
                              WifiState wifi_state,
                              uint8_t wifi_strength,
                              bool mqtt_connected) {
    if (!initialized_)
        return;

    if (time_text != nullptr) {
        strncpy(time_text_, time_text, sizeof(time_text_) - 1);
        time_text_[sizeof(time_text_) - 1] = '\0';
    }

    wifi_state_ = wifi_state;
    wifi_strength_ = wifi_strength;
    mqtt_connected_ = mqtt_connected;

    updateStatusLabels();
}

void DisplayLCD::flushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    DisplayLCD* self = static_cast<DisplayLCD*>(disp->user_data);
    if (self == nullptr) {
        lv_disp_flush_ready(disp);
        return;
    }

    uint16_t w = static_cast<uint16_t>(area->x2 - area->x1 + 1);
    uint16_t h = static_cast<uint16_t>(area->y2 - area->y1 + 1);

    self->tft.startWrite();
    self->tft.setAddrWindow(area->x1, area->y1, w, h);
    self->tft.pushColors(reinterpret_cast<uint16_t*>(color_p), static_cast<uint32_t>(w) * h, true);
    self->tft.endWrite();

    lv_disp_flush_ready(disp);
}

void DisplayLCD::initLvgl() {
    lv_init();

    lv_disp_draw_buf_init(&draw_buf_, draw_buf1_, nullptr, kBufferPixels);
    lv_disp_drv_init(&disp_drv_);
    disp_drv_.hor_res = display_w_;
    disp_drv_.ver_res = display_h_;
    disp_drv_.flush_cb = DisplayLCD::flushCb;
    disp_drv_.draw_buf = &draw_buf_;
    disp_drv_.user_data = this;
    lv_disp_drv_register(&disp_drv_);

    lvgl_ready_ = true;
    last_tick_ms_ = millis();
}

void DisplayLCD::initUi() {
    screen_ = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen_, lv_color_hex(0x000000), 0);

    top_bar_ = lv_obj_create(screen_);
    lv_obj_set_size(top_bar_, display_w_, kTopBarHeight);
    lv_obj_set_pos(top_bar_, 0, 0);
    lv_obj_set_style_bg_color(top_bar_, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(top_bar_, 0, 0);
    lv_obj_set_style_pad_all(top_bar_, 1, 0);
    lv_obj_set_style_radius(top_bar_, 0, 0);
    lv_obj_clear_flag(top_bar_, LV_OBJ_FLAG_SCROLLABLE);

    time_label_ = lv_label_create(top_bar_);
    lv_label_set_text(time_label_, time_text_);
    lv_obj_align(time_label_, LV_ALIGN_RIGHT_MID, -2, 0);

#if LV_FONT_MONTSERRAT_12
    lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_12, 0);
#endif
    lv_obj_set_style_text_color(time_label_, lv_color_hex(0xeeeeee), 0);

    status_left_ = lv_obj_create(top_bar_);
    lv_obj_set_size(status_left_, 74, kTopBarHeight - 4);
    lv_obj_set_style_bg_opa(status_left_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(status_left_, 0, 0);
    lv_obj_set_style_pad_all(status_left_, 0, 0);
    lv_obj_set_style_radius(status_left_, 0, 0);
    lv_obj_clear_flag(status_left_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(status_left_, LV_ALIGN_LEFT_MID, 2, 0);

    mqtt_label_ = lv_label_create(status_left_);
    lv_label_set_text(mqtt_label_, "MQTT");
    lv_obj_align(mqtt_label_, LV_ALIGN_LEFT_MID, static_cast<lv_coord_t>(kWifiIconW + 6), 0);

#if LV_FONT_MONTSERRAT_12
    lv_obj_set_style_text_font(mqtt_label_, &lv_font_montserrat_12, 0);
#endif
    lv_obj_set_style_text_color(mqtt_label_, lv_color_hex(0xeeeeee), 0);

    wifi_icon_ = lv_canvas_create(status_left_);
    lv_canvas_set_buffer(wifi_icon_, wifi_canvas_buf_, kWifiIconW, kWifiIconH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_set_size(wifi_icon_, kWifiIconW, kWifiIconH);
    lv_obj_set_style_bg_opa(wifi_icon_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wifi_icon_, 0, 0);
    lv_obj_clear_flag(wifi_icon_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(wifi_icon_, LV_ALIGN_LEFT_MID, 0, 0);

    uint16_t content_h = (display_h_ > kTopBarHeight) ? (display_h_ - kTopBarHeight) : 0;
    content_h_ = content_h;

    content_log_ = lv_obj_create(screen_);
    lv_obj_set_size(content_log_, display_w_, content_h);
    lv_obj_set_pos(content_log_, 0, kTopBarHeight);
    lv_obj_set_style_bg_color(content_log_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(content_log_, 0, 0);
    lv_obj_set_style_pad_all(content_log_, 0, 0);
    lv_obj_set_style_radius(content_log_, 0, 0);
    lv_obj_clear_flag(content_log_, LV_OBJ_FLAG_SCROLLABLE);

    log_label_ = lv_label_create(content_log_);
    lv_obj_set_width(log_label_, display_w_);
    lv_label_set_long_mode(log_label_, LV_LABEL_LONG_WRAP);
    lv_label_set_text(log_label_, "");
    lv_obj_align(log_label_, LV_ALIGN_TOP_LEFT, 0, 0);
#if LV_FONT_MONTSERRAT_10
    lv_obj_set_style_text_font(log_label_, &lv_font_montserrat_10, 0);
#endif
    lv_obj_set_style_text_color(log_label_, lv_color_hex(0xcccccc), 0);

    updateStatusLabels();
    lv_scr_load(screen_);
}

void DisplayLCD::appendLogText(const String& text) {
    String cleaned = text;
    cleaned.replace("\r", "");

    int start = 0;
    while (start < cleaned.length()) {
        int end = cleaned.indexOf('\n', start);
        if (end < 0)
            end = cleaned.length();

        String line = cleaned.substring(start, end);
        if (line.length() > 0) {
            pushLogLine(line);
        }

        start = end + 1;
    }
}

void DisplayLCD::pushLogLine(const String& line) {
    if (log_line_count_ < kLogLineCount) {
        log_lines_[log_line_count_++] = line;
    } else {
        for (uint8_t i = 1; i < kLogLineCount; ++i) {
            log_lines_[i - 1] = log_lines_[i];
        }
        log_lines_[kLogLineCount - 1] = line;
    }

    refreshLogLabel();
}

void DisplayLCD::refreshLogLabel() {
    if (log_label_ == nullptr)
        return;

    const lv_font_t* font = lv_obj_get_style_text_font(log_label_, LV_PART_MAIN);
    lv_coord_t max_w = static_cast<lv_coord_t>(display_w_);
    lv_coord_t max_h = static_cast<lv_coord_t>(content_h_);
    lv_coord_t letter_space = lv_obj_get_style_text_letter_space(log_label_, LV_PART_MAIN);
    lv_coord_t line_space = lv_obj_get_style_text_line_space(log_label_, LV_PART_MAIN);

    while (log_line_count_ > 0) {
        String combined;
        for (uint8_t i = 0; i < log_line_count_; ++i) {
            combined += log_lines_[i];
            if (i + 1 < log_line_count_)
                combined += "\n";
        }

        lv_point_t text_size;
        lv_txt_get_size(&text_size,
                        combined.c_str(),
                        font,
                        letter_space,
                        line_space,
                        max_w,
                        LV_TEXT_FLAG_NONE);

        if (text_size.y <= max_h) {
            break;
        }

        for (uint8_t i = 1; i < log_line_count_; ++i) {
            log_lines_[i - 1] = log_lines_[i];
        }
        log_line_count_--;
    }

    String combined;
    for (uint8_t i = 0; i < log_line_count_; ++i) {
        combined += log_lines_[i];
        if (i + 1 < log_line_count_)
            combined += "\n";
    }

    lv_label_set_text(log_label_, combined.c_str());
}

void DisplayLCD::updateStatusLabels() {
    if (time_label_ == nullptr || wifi_icon_ == nullptr || mqtt_label_ == nullptr)
        return;

    lv_label_set_text(time_label_, time_text_);

    lv_color_t wifi_on = lv_color_hex(0x00ff00);
    lv_color_t wifi_warn = lv_color_hex(0xffcc00);
    lv_color_t wifi_off = lv_color_hex(0xff3030);
    lv_color_t wifi_dim = lv_color_hex(0xaaaaaa);

    uint8_t active_bars = 0;
    lv_color_t active_color = wifi_off;
    if (wifi_state_ == WifiState::Connected) {
        active_color = wifi_on;
        uint8_t strength = (wifi_strength_ > 4) ? 4 : wifi_strength_;
        active_bars = (strength == 0) ? 1 : strength;
    } else if (wifi_state_ == WifiState::Connecting) {
        active_color = wifi_warn;
        active_bars = 1;
    } else {
        active_color = wifi_off;
        active_bars = 1;
    }

    lv_obj_clear_flag(wifi_icon_, LV_OBJ_FLAG_HIDDEN);

    lv_canvas_fill_bg(wifi_icon_, lv_color_hex(0x000000), LV_OPA_TRANSP);

    const lv_coord_t bar_w = 4;
    const lv_coord_t bar_gap = 1;
    const lv_coord_t bar_heights[4] = { 4, 6, 8, 10 };
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 0;
    rect_dsc.border_width = 0;
    rect_dsc.bg_opa = LV_OPA_COVER;

    for (uint8_t i = 0; i < 4; ++i) {
        lv_color_t bar_color = (i < active_bars) ? active_color : wifi_dim;
        rect_dsc.bg_color = bar_color;
        lv_coord_t x = static_cast<lv_coord_t>(i * (bar_w + bar_gap));
        lv_coord_t y = static_cast<lv_coord_t>(kWifiIconH - bar_heights[i]);
        lv_canvas_draw_rect(wifi_icon_, x, y, bar_w, bar_heights[i], &rect_dsc);
    }

    lv_label_set_text(mqtt_label_, "MQTT");
    lv_obj_set_style_text_color(mqtt_label_, mqtt_connected_ ? lv_color_hex(0x00ff00) : lv_color_hex(0xff3030), 0);
}


