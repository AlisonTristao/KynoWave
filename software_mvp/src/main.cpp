#include <Arduino.h>
#include <Task01.h>
#include <Task02.h> 
#include <Logger.h>
#include <Settings.h>

// define a baud rate for the serial communication
#ifndef BAUDRATE
    #define BAUDRATE 115200 // default baud rate if not defined in platformio.ini
#endif

// create the logger to add the messagens 
Logger logger;

// callback to print the messages to the serial, this will be set as the send callback of the logger
esp_err_t sendToSerial(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) return ESP_ERR_INVALID_ARG;

    const message* msg = reinterpret_cast<const message*>(data);
    // [timer] [type] [n/total] message
    Serial.printf("[%lu ms] [%s] [%u/%u] %s\n", 
        msg->timer, 
        logTypeToString(msg->type), 
        msg->packet_number, 
        msg->total_packets, 
        msg->msg
    );
    return ESP_OK;
}

void waitForBootButtonPress() {
    const uint8_t bootButtonPin = 0; // GPIO0 is usually the boot button
    pinMode(bootButtonPin, INPUT);
    while (digitalRead(bootButtonPin) == HIGH)
        vTaskDelay(pdMS_TO_TICKS(100));
}

void setup() {
    // wait for the user to press the boot button before starting the main logic
    waitForBootButtonPress();

    // set the send callback of the logger to the function that will print the messages to the serial
    logger.begin();
    logger.set_send_callback(sendToSerial);

    // convert the BAUDRATE macro to an integer and use it as the baud rate for the serial communication
    const uint32_t baud = static_cast<uint32_t>(BAUDRATE);

    // init the serial communication and print a log message to indicate that the serial is initialized
    Serial.begin(baud);
    logger.insert_log("Serial initialized at " + String(baud) + " baud.", logType::INFO);

    // create a void packet to send the logger obj to the tasks
    void* logger_ptr = static_cast<void*>(&logger);

    // init the tasks, passing the logger pointer as a parameter
    startTask01(logger_ptr, CORE_0); // warning, this core is used to wifi and bluetooth too
    startTask02(logger_ptr, CORE_1);
}

void loop() {
    logger.insert_log("stopping the main loop...", logType::DEBG);
    vTaskDelete(NULL); 
}