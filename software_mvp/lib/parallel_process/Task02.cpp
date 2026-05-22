#include "Task02.h"
#include <Arduino.h>
#include <KynoWave.h>
#include <Settings.h>
#include <AudioUdpStreamer.h>

TaskHandle_t handleTask0 = NULL;

#define MIC_PIN 17
#define SAMPLE_RATE 16000
#define NUM_SAMPLES 512

#define PC_IP "10.253.53.156"
#define PC_PORT 5005

AudioUdpStreamer audioUdp;

void Task02Code(void* pvParameters) {
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);

    kynowave->logger.insert_logf(
        logType::INFO,
        "Task 2 running on core %u",
        xPortGetCoreID()
    );

    bool audioOk = audioUdp.begin(
        MIC_PIN,
        SAMPLE_RATE,
        NUM_SAMPLES,
        PC_IP,
        PC_PORT
    );

    while (true) {
        kynowave->leds_state_ = audioUdp.routine();
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