#include "Task02.h"
#include <Arduino.h>
#include <KynoWave.h>
#include <Settings.h>
#include <AudioUdpStreamer.h>

TaskHandle_t handleTask0 = NULL;

void Task02Code(void* pvParameters) {
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);

    kynowave->logger.insert_logf(
        logType::INFO,
        "Task 2 running on core %u",
        xPortGetCoreID()
    );

    while (true) {
        kynowave->rms_value_ = kynowave->audioUdp.routine();
        //Serial.println(kynowave->rms_value_);
        //kynowave->setLedsIntensity(led_level);
        taskYIELD();
    }
}

void startTask02(void* pvParameters, uint8_t CORE) {
    xTaskCreatePinnedToCore(
        Task02Code,
        "Task02",
        TASK_STACK_SIZE_02,
        pvParameters,
        2,
        &handleTask0,
        CORE
    );
}