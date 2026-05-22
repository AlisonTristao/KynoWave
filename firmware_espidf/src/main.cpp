#include <stdio.h>
#include <stdbool.h>            
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"       
#include "esp_log.h"

// external libraries
#include <TinyShell.h>      
#include <Logger.h>        

// internal libraries
#include <KynoWave.h>
#include <Task01.h>
#include <Task02.h> 

// header
#include <Settings.h>

// create the instance of the kynowave class, to use the static members
KynoWave kynowave;

extern "C" {
    void app_main();
}

void app_main() {
    // set the send callback of the logger to the function that will print the messages to the serial
    kynowave.begin();

    setupLeds();

    // init the tasks 01 to print the logs 
    startTask01(kynowave.getInstance(), CORE_0); // warning, this core is used to wifi and bluetooth too

    // saves the free memory
    kynowave.logger.insert_logf(logType::DEBG, "Free heap: %0.2f KB",       (float)esp_get_free_heap_size() / 1024);

    // connect to WiFi and MQTT
    kynowave.connectToWiFi();

    // init the tasks 02 to handle the OTA updates, MQTT and the shell commands
    startTask02(kynowave.getInstance(), CORE_1);

    vTaskDelete(NULL);
}