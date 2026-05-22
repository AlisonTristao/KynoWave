#include "Task01.h"
#include "Settings.h"
#include <KynoWave.h>
#include <time.h>
#include <cstring>

TaskHandle_t handleTask1 = NULL;

void Task01Code(void * pvParameters) {
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);

    kynowave->logger.insert_logf(
        logType::INFO,
        "Task 1 running on core %u",
        xPortGetCoreID()
    );

    constexpr uint32_t kRoutineDelayMs    = 1;     // evita watchdog
    constexpr uint32_t kDisplayIntervalMs = 250;   // 4 Hz
    constexpr uint32_t kStatusIntervalMs  = 1000;  // 1 Hz
    constexpr uint32_t kLoggerIntervalMs  = 250;   // 4 Hz

    uint32_t last_display_ms = 0;
    uint32_t last_status_ms  = 0;
    uint32_t last_logger_ms  = 0;

    while (true) {
        uint32_t now = millis();

        kynowave->routine();

        if (now - last_display_ms >= kDisplayIntervalMs) {
            kynowave->display.update();
            last_display_ms = now;
        }

        if (now - last_status_ms >= kStatusIntervalMs) {
            char time_text[24];
            struct tm timeinfo;

            if (kynowave->getLocalTimeInfo(timeinfo)) {
                strftime(time_text, sizeof(time_text), "%d/%m %H:%M", &timeinfo);
            } else {
                strncpy(time_text, "--/-- --:--", sizeof(time_text));
                time_text[sizeof(time_text) - 1] = '\0';
            }

            kynowave->display.updateStatus(
                time_text,
                kynowave->wifiState(),
                kynowave->wifiStrength(),
                kynowave->mqttConnected()
            );

            last_status_ms = now;
        }

        if (now - last_logger_ms >= kLoggerIntervalMs) {
            kynowave->logger.send_live_logger();
            last_logger_ms = now;
        }

        vTaskDelay(pdMS_TO_TICKS(kRoutineDelayMs));
    }
}
void startTask01(void* pvParameters, uint8_t CORE) {
    xTaskCreatePinnedToCore(
        Task01Code,
        "Task01",
        TASK_STACK_SIZE_01,
        pvParameters,
        3,
        &handleTask1,
        CORE
    );
}