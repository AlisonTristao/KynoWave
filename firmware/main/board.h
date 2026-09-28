// KynoWave prototype (hardware_v1.0.0): the pins this firmware uses.
#pragma once

#include <driver/gpio.h>

namespace board {

// ST7735 128x160 LCD on the FPC connector (SPI, write only).
constexpr gpio_num_t kLcdCs = GPIO_NUM_4;
constexpr gpio_num_t kLcdReset = GPIO_NUM_5;
constexpr gpio_num_t kLcdDc = GPIO_NUM_6;
constexpr gpio_num_t kLcdMosi = GPIO_NUM_7;
constexpr gpio_num_t kLcdSck = GPIO_NUM_15;
constexpr gpio_num_t kLcdBacklight = GPIO_NUM_16;

// MAX4466 microphone amplifier output, straight into the ADC. On the prototype
// the trace goes to GPIO17 (ADC2, shared with Wi-Fi); a jumper takes it to
// GPIO1 (ADC1 channel 0), which Wi-Fi never touches. GPIO17 is left alone.
constexpr gpio_num_t kMic = GPIO_NUM_1;

// The six front LEDs, left to right (the VU meter). OFF for now: LEDs 5 and
// 6 sit on GPIO35/36, which the chip's octal PSRAM (ESP32-S3R8) uses, and the
// other four alone are not worth a half VU meter. Nothing touches these pins
// while this is false.
constexpr bool kLedsEnabled = false;
constexpr gpio_num_t kLeds[] = {GPIO_NUM_21, GPIO_NUM_47, GPIO_NUM_48,
                                GPIO_NUM_45, GPIO_NUM_35, GPIO_NUM_36};
constexpr int kLedCount = sizeof(kLeds) / sizeof(kLeds[0]);

}  // namespace board
