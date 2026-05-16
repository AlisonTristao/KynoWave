// external libraries
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <TinyShell.h>

// internal libraries
#include <Task01.h>
#include <Task02.h> 
#include <KynoWave.h>

// header
#include <Settings.h>

// define a baud rate for the serial communication
#ifndef BAUDRATE
    #define BAUDRATE 115200 // default baud rate if not defined in platformio.ini
#endif

// create the instance of the kynowave class, to use the static members
KynoWave kynowave;

void setup() {
    // set the send callback of the logger to the function that will print the messages to the serial
    kynowave.begin();

    // create a void packet to send the logger obj to the tasks
    void* kynowave_ptr = static_cast<void*>(&kynowave);

    // init the tasks, passing the kyno pointer as a parameter
    startTask01(kynowave_ptr, CORE_0); // warning, this core is used to wifi and bluetooth too
    startTask02(kynowave_ptr, CORE_1);
    Serial.println("Tasks started successfully!");

    // saves the free memory
    kynowave.logger.insert_logf(logType::DEBG, "Free heap: %0.2f KB",       (float)esp_get_free_heap_size() / 1024);
    kynowave.logger.insert_logf(logType::DEBG, "Free 8bit heap: %0.2f KB",  (float)heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    kynowave.logger.insert_logf(logType::DEBG, "Free psram: %0.2f KB",      (float)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}

void loop() {
    // stopping the loop to avoid consuming CPU time, since we are using tasks to run the code in parallel
    kynowave.logger.insert_log(logType::DEBG, "Main loop is idle...");
    vTaskDelay(portMAX_DELAY);
}