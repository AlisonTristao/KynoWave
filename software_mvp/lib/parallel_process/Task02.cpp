#include "Task01.h"
#include <KynoWave.h>
#include <Settings.h>
#include <ArduinoOTA.h>

// handler para a task que rodará no núcleo 0
TaskHandle_t handleTask0 = NULL;

// A função real que rodará no núcleo
void Task02Code(void * pvParameters) {
    // unpacket the logger pointer from the void parameter
    KynoWave* kynowave = static_cast<KynoWave*>(pvParameters);
    
    // add a log message to indicate that the task is running and the core it is running on
    kynowave->logger.insert_logf(logType::INFO, "Task 2 running on core %u", xPortGetCoreID());
    
    // string to receive the serial commands
    String command;

    while (true) {
        // delay to avoid consuming too much CPU time...
        vTaskDelay(pdMS_TO_TICKS(1)); 

        // check OTA updates
        ArduinoOTA.handle();

        // check MQTT 
        kynowave->MQTT_handle();

        // check for serial input and run the command in the shell if there is any, 
        // this allows us to interact with the shell from the serial monitor while the tasks are running
        if (!KynoWave::readFromSerial(command))
            continue;
            
        // run the command in the shell and get the result code
        kynowave->logger.insert_log(logType::CMDO, command.c_str());
        uint8_t result = kynowave->shell.run_command_line(command.c_str());

        // if return some error, make something...
        if (result != RESULT_OK)
            kynowave->logger.insert_logf(logType::ERRO, "Error: %d", result);
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