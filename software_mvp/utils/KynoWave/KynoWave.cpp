#include "KynoWave.h"
#include <PubSubClient.h>

// initialize the static members of the KynoWave class
KynoWave* KynoWave::instante_ = nullptr;

bool KynoWave::sendToSerial(const uint8_t *data, size_t len) {
    KynoWave::sendToMQTT(data, len); // also send the log to MQTT
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
        for (size_t i = 0; i < num_samples; ++i) {
            Serial.printf("%d ", msg->content.i2sD[i]);
            instante_->display.printWrap(String(msg->content.i2sD[i]) + " ");
        }
        Serial.print("\n");
        instante_->display.printWrap("\n");
    } else {
        Serial.printf("%s\n", msg->content.text);
        instante_->display.printWrap(String(msg->content.text) + "\n");
    }
    return true;
}

bool KynoWave::sendToMQTT(const uint8_t *data, size_t len) {
    if (data == nullptr || len == 0) return false;

    //  verify MQTT connection before trying to publish
    if (!instante_->clientMQTT.connected()) 
        return false; 

    // parse the message struct from the raw data
    const message* msg = reinterpret_cast<const message*>(data);
    String topic = "kynowave/device_01";
    
    if (msg->type == logType::I2SD) topic += "i2s_data";    
    else                            topic += "status_logs"; 
    return instante_->clientMQTT.publish(topic.c_str(), data, static_cast<unsigned int>(len));
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

void KynoWave::begin() {
    // init the serial communication and print a log message to indicate that the serial is initialized
    Serial.begin(BAUDRATE);
    logger.insert_logf(logType::INFO, "Serial initialized at baud: %u", BAUDRATE);

    // setup the logger
    logger.begin();
    logger.set_send_callback(KynoWave::sendToSerial);

    // setup the shell input callback    
    shell.begin();
    shell.set_output_callback([](const std::string& text) {
        instante_->logger.insert_logf(logType::DEBG, "Shell output: %s", text.c_str());
    });

    // setup the display
    display.begin();

    // setup the shell
    shell.create_module("kyno", "KynoWave main module");
    shell.add(ping, "ping", "Ping the KynoWave to check if it's alive", "kyno");
}

void KynoWave::MQTT_handle() {
    if (!clientMQTT.connected())
        reconnect();
    clientMQTT.loop();
}

void KynoWave::setup_MQTT() {
    clientMQTT.setServer(mqtt_server, mqtt_port);
    clientMQTT.setBufferSize(256);
    reconnect();
}

void KynoWave::reconnect() {
    // try one time
    if (!clientMQTT.connected()) {
        logger.insert_log(logType::INFO, "Trying to connect to MQTT...");
        
        if (clientMQTT.connect(clientId.c_str())) {
            logger.insert_logf(logType::INFO, "Connected to Broker: %s", mqtt_server);
        } else {
            logger.insert_logf(logType::INFO, "Failed, rc=%d", clientMQTT.state());
            logger.insert_log(logType::INFO, ". Trying again in 5 seconds...");
            delay(5000);
        }
    }
}

void KynoWave::connectToWiFi() {
    // wifi connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    const uint8_t max_attempts = 5; 
    bool success = false;
    
    logger.insert_logf(logType::INFO, "Connecting to Wi-Fi: %s", ssid);

    for (uint8_t attempts = 0; attempts < max_attempts; ++attempts) {
        if (WiFi.waitForConnectResult() == WL_CONNECTED) {
            success = true;
            break; 
        }
        logger.insert_log(logType::INFO, "Failed to connect to Wi-Fi! Trying again...");
        delay(1000);
    }

    if (!success) {
        logger.insert_log(logType::INFO, "Failed to connect to Wi-Fi!");
        return; 
    } else {
        logger.insert_log(logType::INFO, "Connected to Wi-Fi!");
        logger.insert_logf(logType::INFO, "IP address: %s", WiFi.localIP().toString().c_str());
    }
}
