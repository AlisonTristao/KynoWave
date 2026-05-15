#ifndef OUTPUT_H
#define OUTPUT_H

// external libraries
#include <Arduino.h>

// internal libraries
#include <Logger.h>

// callback to print the messages to the serial, this will be set as the send callback of the logger
bool sendToSerial(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) return false;

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
    return true;
}

#endif