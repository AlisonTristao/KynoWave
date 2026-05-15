#include "Task01.h"
#include "Settings.h"

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask0 = NULL;

// A função real que rodará no núcleo
void Task02Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    Logger* logger = static_cast<Logger*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    logger->insert_log("Task 2 iniciada no core " + String(xPortGetCoreID()), logType::INFO);
    
    while(true) {
        logger->send_logger_live(); 

        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

void startTask02(void* pvParameters) {
    xTaskCreatePinnedToCore(
        Task02Code, 
        "Task02", 
        TASK_STACK_SIZE, 
        pvParameters,
        2, 
        &handleTask0, 
        CORE_1 
    );
}