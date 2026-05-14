#include "Task01.h"
#include "Settings.h"

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask0 = NULL;

// A função real que rodará no núcleo
void Task02Code(void * pvParameters) {
    Serial.printf("Task 0 iniciada no core %d\n", xPortGetCoreID());
    
    for(;;) {
        // Exemplo: Lógica de Wi-fi ou processamento pesado
        // Serial.println("Processando no Core 0...");
        
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

void startTask02() {
    xTaskCreatePinnedToCore(
        Task02Code, 
        "Task02", 
        TASK_STACK_SIZE, 
        NULL, 
        2, 
        &handleTask0, 
        CORE_1 
    );
}