#include "Task01.h"
#include "Settings.h"

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask1 = NULL;

// A função real que rodará no núcleo 0
void Task01Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    Logger* logger = static_cast<Logger*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    logger->insert_log("Task 1 iniciada no core " + String(xPortGetCoreID()), logType::INFO);
    
    while (true) {
        
        // delay de 1 ms para evitar que a task consuma 100% do CPU
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void startTask01(void* pvParameters) {
    xTaskCreatePinnedToCore(
        Task01Code, 
        "Task01", 
        TASK_STACK_SIZE, 
        pvParameters, 
        2, 
        &handleTask1, 
        CORE_0  // Fixado no Core 0
    );
}