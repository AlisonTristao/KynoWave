#ifndef LV_CONF_H
#define LV_CONF_H

// Basic LVGL configuration for the ST7735 128x160 display.
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

// Use the maximum resolution to handle rotation changes.
#define LV_HOR_RES_MAX 160
#define LV_VER_RES_MAX 128

// Tick is provided by the application (see DisplayLCD::update).
#define LV_TICK_CUSTOM 0

// Keep LVGL logging disabled for embedded builds.
#define LV_USE_LOG 0

// Enable smaller fonts for tight layouts.
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1

#endif // LV_CONF_H
