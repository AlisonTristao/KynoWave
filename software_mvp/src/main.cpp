#include <Arduino.h>
#include <Task01.h>
#include <Task02.h> 
#include <Logger.h>
#include <Settings.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

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
    Serial.printf("[%lu ms] [%s] [%u/%u] ", 
        msg->timer, 
        logTypeToString(msg->type), 
        msg->packet_number, 
        msg->total_packets
    );
    if (msg->type == logType::I2SD) {
        int16_t* sound_data = (msg->content.i2sD);
        for (size_t i = 0; i < msg->content.size / sizeof(int16_t); ++i)
            Serial.printf("%d ", sound_data[i]);
        Serial.print("\n");
    } else
        Serial.printf("%s\n", msg->content.text);
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
    logger.insert_log("Serial initialized at baud.", logType::INFO);

    // create a void packet to send the logger obj to the tasks
    void* logger_ptr = static_cast<void*>(&logger);

    // init the tasks, passing the logger pointer as a parameter
    startTask01(logger_ptr, CORE_0); // warning, this core is used to wifi and bluetooth too
    startTask02(logger_ptr, CORE_1);

    const uint32_t free_heap = esp_get_free_heap_size();
    const uint32_t min_free_heap = esp_get_minimum_free_heap_size();
    const uint32_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    const uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    logger.insert_log("Free heap: " + String(free_heap) + " bytes", logType::DEBG);
    logger.insert_log("Min free heap: " + String(min_free_heap) + " bytes", logType::DEBG);
    logger.insert_log("Free 8bit heap: " + String(free_8bit) + " bytes", logType::DEBG);
    logger.insert_log("Free PSRAM: " + String(free_psram) + " bytes", logType::DEBG);
}

void loop() {
    logger.insert_log("stopping the main loop...", logType::DEBG);

    // array de 16 bytes para simular um pacote de dados de som, com valores aleatórios
    int16_t sound_data[16];
    for (int16_t i = -4; i < 12; ++i)
        sound_data[i + 4] = i;
    logger.insert_log(sound_data, 16);

    vTaskDelete(NULL); 
}