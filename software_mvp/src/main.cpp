// external libraries
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <TinyShell.h>

// internal libraries
#include <Task01.h>
#include <Task02.h> 
#include <Logger.h>

// header
#include <Output.h>
#include <Settings.h>

// define a baud rate for the serial communication
#ifndef BAUDRATE
    #define BAUDRATE 115200 // default baud rate if not defined in platformio.ini
#endif

// create the logger to add the messagens 
Logger logger;

// create a shell to run terminal commands
TinyShell shell;

void setup() {
    // set the send callback of the logger to the function that will print the messages to the serial
    logger.begin();
    logger.set_send_callback(sendToSerial);

    // init the serial communication and print a log message to indicate that the serial is initialized
    Serial.begin(BAUDRATE);
    logger.insert_log(logType::INFO, "Serial initialized at baud: %u", BAUDRATE);

    // create a void packet to send the logger obj to the tasks
    void* logger_ptr = static_cast<void*>(&logger);

    // init the tasks, passing the logger pointer as a parameter
    startTask01(logger_ptr, CORE_0); // warning, this core is used to wifi and bluetooth too
    startTask02(logger_ptr, CORE_1);

    // saves the free memory
    logger.insert_log(logType::DEBG, "Free heap: %0.2f KB", (float)esp_get_free_heap_size() / 1024);
    logger.insert_log(logType::DEBG, "Free 8bit heap: %0.2f KB", (float)heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    logger.insert_log(logType::DEBG, "Free psram: %0.2f KB", (float)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
}

void loop() {
    Serial.println("Main loop running...");
    logger.insert_log(logType::DEBG, "stopping the main loop...");
    vTaskDelete(NULL); 
}