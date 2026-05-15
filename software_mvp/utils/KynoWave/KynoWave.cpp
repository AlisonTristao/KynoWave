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
        int16_t* sound_data = (msg->content.i2sD);
        for (size_t i = 0; i < msg->content.size / sizeof(int16_t); ++i)
            Serial.printf("%d ", sound_data[i]);
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

uint8_t ping() {
    Serial.println("pong...");
    return RESULT_OK;  // return 0 to indicate success
}

// wrapper functions to be used in the shell
uint8_t KynoWave::wrapper_h() {
    instante_->logger.insert_log(logType::INFO, KynoWave::instante_->shell.get_help("").c_str());
    return RESULT_OK;  // return 0 to indicate success
}

void KynoWave::begin() {
    // init the serial communication and print a log message to indicate that the serial is initialized
    Serial.begin(BAUDRATE);
    logger.insert_logf(logType::INFO, "Serial initialized at baud: %u", BAUDRATE);

    // setup the logger
    logger.set_send_callback(KynoWave::sendToSerial);
    logger.begin();

    // setup the shell
    shell.create_module("kyno", "KynoWave main module");
    shell.create_module("help", "ajuda e informacoes");

    shell.add(ping, "ping", "Ping the KynoWave to check if it's alive", "kyno");

    // add the wrapper functions to the help module
    shell.add(KynoWave::wrapper_h, "h", "Lista os modulos", "help");
}