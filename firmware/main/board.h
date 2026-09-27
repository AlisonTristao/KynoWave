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

// MAX4466 microphone amplifier output, straight into the ADC (ADC2 channel 6:
// shared with Wi-Fi, see audio/Audio.h).
constexpr gpio_num_t kMic = GPIO_NUM_17;

// The six front LEDs, left to right (the VU meter).
constexpr gpio_num_t kLeds[] = {GPIO_NUM_21, GPIO_NUM_47, GPIO_NUM_48,
                                GPIO_NUM_45, GPIO_NUM_35, GPIO_NUM_36};
constexpr int kLedCount = sizeof(kLeds) / sizeof(kLeds[0]);

}  // namespace board
