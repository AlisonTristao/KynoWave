#include "Task01.h"
#include <KynoWave.h>
#include <Settings.h>

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask0 = NULL;

// A função real que rodará no núcleo
void Task02Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    kynowave->logger.insert_logf(logType::INFO, "Task 2 iniciada no core %u", xPortGetCoreID());
    
    // string to receive the serial commands
    String command;

    while (true) {
        if (KynoWave::readFromSerial(command)) {
            // kynowave.logger.insert_logf(logType::ERRO, "Error processing command: %s", command);
            uint8_t result = kynowave->shell.run_command_line(command.c_str());
        }

        vTaskDelay(pdMS_TO_TICKS(1)); 
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