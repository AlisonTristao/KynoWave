#include "Task01.h"
#include "Settings.h"
#include <KynoWave.h>
#include <time.h>
#include <cstring>

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask1 = NULL;

// A função real que rodará no núcleo 0
void Task01Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    kynowave->logger.insert_logf(logType::INFO, "Task 1 running on core %u", xPortGetCoreID());

    constexpr uint32_t kUiTickMs = 10;
    constexpr uint32_t kStatusIntervalMs = 1000;
    constexpr uint32_t kLogIntervalMs = 100;

    uint32_t last_status_ms = 0;
    uint32_t last_log_ms = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(kUiTickMs));
        kynowave->display.update();

        uint32_t now = millis();

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

        if (now - last_log_ms >= kLogIntervalMs) {
            kynowave->logger.send_live_logger();
            last_log_ms = now;
        }
    }
}

void startTask01(void* pvParameters, uint8_t CORE) {
    xTaskCreatePinnedToCore(
        Task01Code, 
        "Task01", 
        TASK_STACK_SIZE_01, 
        pvParameters, 
        2, 
        &handleTask1, 
        CORE  // Fixado no Core 0
    );
}