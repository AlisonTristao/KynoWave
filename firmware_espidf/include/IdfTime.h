#ifndef IDF_TIME_H
#define IDF_TIME_H

#include <stdint.h>
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

inline uint32_t idf_now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

inline void idf_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

#endif // IDF_TIME_H
