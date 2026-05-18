#ifndef PINS_SETTINGS_H
#define PINS_SETTINGS_H

// core and task settings
#define CORE_0 0
#define CORE_1 1
#define TASK_STACK_SIZE_01 2040
#define TASK_STACK_SIZE_02 32*1024

// display pins
#define TFT_CS      4
#define TFT_RST     5
#define TFT_DC      6   // AO
#define TFT_SDA     7   // MOSI
#define TFT_SCK     15  
#define TFT_BL      16
#define TFT_WIDTH   160
#define TFT_HEIGHT  128

#endif