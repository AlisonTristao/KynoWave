#ifndef SETTINGS_H
#define SETTINGS_H

// core and task settings
#define CORE_0 0
#define CORE_1 1
#define TASK_STACK_SIZE_01 2*4096
#define TASK_STACK_SIZE_02 2*4096

// display pins
#define TFT_CS      4
#define TFT_RST     5
#define TFT_DC      6   // AO
#define TFT_SDA     7   // MOSI
#define TFT_SCK     15  
#define TFT_BL      16
#define TFT_WIDTH   128
#define TFT_HEIGHT  160

// TFT_eSPI setup (kept in Settings.h by request)
#ifndef CONFIG_IDF_TARGET_ESP32S3
#define CONFIG_IDF_TARGET_ESP32S3 1
#endif

#define ST7735_DRIVER
#define ST7735_BLACKTAB

#define TFT_MOSI TFT_SDA
#define TFT_SCLK TFT_SCK
#define TFT_MISO -1

#define TFT_BACKLIGHT_ON HIGH
#define SPI_FREQUENCY 20000000
#define SPI_READ_FREQUENCY 20000000

#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
// Free fonts and smooth font rendering use extra RAM/flash.
// Keep disabled for stability while validating the display.

// LEDS 
#define LED_1       21
#define LED_2       47
#define LED_3       48
#define LED_4       45
#define LED_5       35
#define LED_6       36
#define NUM_LEDS     6
#define LEDS_ARR {LED_1, LED_2, LED_3, LED_4, LED_5, LED_6}

// Audio settings
#define MIC_PIN 17
#define SAMPLE_RATE 16000
#define NUM_SAMPLES 256

#endif