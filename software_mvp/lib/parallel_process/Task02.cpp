#include "Task02.h"
#include <KynoWave.h>
#include <Settings.h>

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask0 = NULL;

// A função real que rodará no núcleo
void Task02Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    kynowave->logger.insert_logf(logType::INFO, "Task 2 running on core %u", xPortGetCoreID());

    while (true) {
        // delay to avoid consuming too much CPU time...
        vTaskDelay(pdMS_TO_TICKS(1)); 

        // kyno routine
        kynowave->routine();
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