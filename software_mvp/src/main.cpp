// external libraries
#include <Arduino.h>
#include <esp_heap_caps.h>  // for checking the free heap memory in the setup function
#include <esp_system.h>     // for checking the free heap memory in the setup function
#include <TinyShell.h>      // for the shell
#include <Logger.h>         // for the logger

// internal libraries7
#include <KynoWave.h>
#include <Task01.h>
#include <Task02.h> 

// header
#include <Settings.h>

// create the instance of the kynowave class, to use the static members
KynoWave kynowave;

void setup() {
    // set the send callback of the logger to the function that will print the messages to the serial
    kynowave.begin();

    // init the tasks 01 to print the logs 
    startTask01(kynowave.getInstance(), CORE_0); // warning, this core is used to wifi and bluetooth too

    // saves the free memory
    kynowave.logger.insert_logf(logType::DEBG, "Free heap: %0.2f KB",       (float)esp_get_free_heap_size() / 1024);

    // connect to WiFi and MQTT
    kynowave.connectToWiFi();

    // init the tasks 02 to handle the OTA updates, MQTT and the shell commands
    startTask02(kynowave.getInstance(), CORE_1);
}

void loop() {
    // stopping the loop to avoid consuming CPU time, since we are using tasks to run the code in parallel
    kynowave.logger.insert_log(logType::DEBG, "Stopping the void loop...");
    vTaskDelete(NULL); // delete the loop task to stop it from running
}