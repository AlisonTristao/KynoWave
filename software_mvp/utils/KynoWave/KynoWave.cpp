#include "KynoWave.h"

// initialize the static members of the KynoWave class
KynoWave* KynoWave::instante_ = nullptr;

bool KynoWave::sendToSerial(const uint8_t *data, size_t len) {
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
        size_t num_samples = msg->content.size / sizeof(int16_t);
        for (size_t i = 0; i < num_samples; ++i)
            Serial.printf("%d ", msg->content.i2sD[i]);
        Serial.print("\n");
    } else
        Serial.printf("%s\n", msg->content.text);
    return true;
}

// receive a string pointer
bool KynoWave::readFromSerial(String& input) {
    if (!Serial.available())
        return false;

    input = Serial.readStringUntil('\n');
    return true;
}

uint8_t ping(int number) {
    Serial.printf("human -pong %d\n", number);
    return RESULT_OK;  // return 0 to indicate success
}

void print_output(const string& output) {
    Serial.println(output.c_str());
}

void KynoWave::begin() {
    // init the serial communication and print a log message to indicate that the serial is initialized
    Serial.begin(BAUDRATE);
    logger.insert_logf(logType::INFO, "Serial initialized at baud: %u", BAUDRATE);

    // setup the logger
    logger.begin();
    logger.set_send_callback(KynoWave::sendToSerial);

    // setup the shell input callback    
    shell.begin();
    shell.set_output_callback(print_output);

    // setup the shell
    shell.create_module("kyno", "KynoWave main module");
    shell.add(ping, "ping", "Ping the KynoWave to check if it's alive", "kyno");
}