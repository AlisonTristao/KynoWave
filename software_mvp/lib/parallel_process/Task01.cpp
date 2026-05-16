#include "Task01.h"
#include "Settings.h"
#include <KynoWave.h>

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask1 = NULL;

// A função real que rodará no núcleo 0
void Task01Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    kynowave->logger.insert_logf(logType::INFO, "Task 1 iniciada no core %u", xPortGetCoreID());

    while (true) {
        // delay de 1 ms para evitar que a task consuma 100% do CPU
        vTaskDelay(pdMS_TO_TICKS(100));

        // send the messagens to ouput channel
        kynowave->logger.send_live_logger();
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