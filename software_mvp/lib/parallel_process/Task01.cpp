#include "Task01.h"
#include "Settings.h"

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask1 = NULL;

// A função real que rodará no núcleo
void Task01Code(void * pvParameters) {
    Serial.printf("Task 1 iniciada no core %d\n", xPortGetCoreID());
    
    while (true) {
        
        // delay de 1 ms para evitar que a task consuma 100% do CPU
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void startTask01() {
    xTaskCreatePinnedToCore(
        Task01Code, 
        "Task01", 
        TASK_STACK_SIZE, 
        NULL, 
        2, 
        &handleTask1, 
        CORE_0  // Fixado no Core 0
    );
}