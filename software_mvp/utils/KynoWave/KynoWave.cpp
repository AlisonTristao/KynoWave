#include "KynoWave.h"
#include <PubSubClient.h>
#include <cstring>
#include <time.h>

// initialize the static members of the KynoWave class
KynoWave* KynoWave::instante_ = nullptr;

static uint8_t wifiStrengthFromRssi(int32_t rssi) {
    if (rssi >= -50) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -75) return 2;
    if (rssi >= -85) return 1;
    return 0;
}

KynoWave::KynoWave()
    : hub(),
      serial_sink(*this),
      display_sink(*this),
      mqtt_sink(*this),
      serial_source(*this),
      mqtt_source(*this) {
    if (instante_ == nullptr)
        instante_ = this;

    // initialize the MQTT client with the WiFi client
    clientMQTT.setClient(espClient);
}

bool KynoWave::SerialSink::send(const uint8_t* data, size_t len) {
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
        }
        Serial.print("\n");
    } else {
        Serial.printf("%s\n", msg->content.text);
    }
    return true;
}

    bool KynoWave::DisplaySink::send(const uint8_t* data, size_t len) {
        if (data == nullptr || len == 0) return false;

        const message* msg = reinterpret_cast<const message*>(data);
        if (msg->type == logType::I2SD) {
            // Avoid dumping large binary streams to the display.
            return true;
        }

        String text = String(msg->content.text);
        const size_t max_chars = 120;
        if (text.length() > max_chars) {
            text = text.substring(0, max_chars) + "...";
        }

        k_.display.printWrap(text + "\n");
        return true;
    }

    bool KynoWave::MqttSink::send(const uint8_t* data, size_t len) {
        return KynoWave::sendToMQTT(data, len);
    }

    bool KynoWave::SerialSource::read(String& out) {
        return KynoWave::readFromSerial(out);
    }

    bool KynoWave::MqttSource::read(String& out) {
        return k_.readFromMqtt(out);
    }

    bool KynoWave::sendToSerial(const uint8_t *data, size_t len) {
        if (instante_ == nullptr) return false;
        return instante_->serial_sink.send(data, len);
    }

    bool KynoWave::sendToMQTT(const uint8_t *data, size_t len) {
        if (data == nullptr || len == 0) return false;
        if (instante_ == nullptr) return false;

    //  verify MQTT connection before trying to publish
    if (!instante_->clientMQTT.connected()) 
        return false; 

    // parse the message struct from the raw data
    const message* msg = reinterpret_cast<const message*>(data);
        String topic = String(instante_->mqtt_topic);
    
        if (msg->type == logType::I2SD) topic += "/i2s_data";    
        else                            topic += "/status_logs"; 
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

    // setup the logger
    logger.begin();
    logger.set_send_callback(KynoWave::sendToHub);
    logger.insert_logf(logType::INFO, "Serial initialized at baud: %u", BAUDRATE);

    // setup device hub
    hub.addOutput(&serial_sink, "serial");
    hub.addOutput(&mqtt_sink, "mqtt");
    hub.addOutput(&display_sink, "display");
    hub.addInput(&serial_source, "serial");
    hub.addInput(&mqtt_source, "mqtt");

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

    // setup the MQTT client
    clientMQTT.setServer(mqtt_server, mqtt_port);
    clientMQTT.setBufferSize(256);
    clientMQTT.setCallback(KynoWave::mqttCallback);

    // setup OTA
    ota.begin(logger, display);
}

void KynoWave::MQTT_handle() {
    if (!clientMQTT.connected()) {
        mqtt_connected_ = false;
        reconnect();
    } else {
        mqtt_connected_ = true;
    }
    clientMQTT.loop();
}

void KynoWave::reconnect() {
    // try one time
    if (!clientMQTT.connected()) {
        mqtt_connected_ = false;
        logger.insert_log(logType::INFO, "Trying to connect to MQTT...");
        
        if (clientMQTT.connect(clientId.c_str())) {
            logger.insert_logf(logType::INFO, "Connected to Broker: %s", mqtt_server);
            clientMQTT.subscribe(mqtt_cmd_topic);
            mqtt_connected_ = true;
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
    wifi_state_ = WifiState::Connecting;
    wifi_strength_ = 0;

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
        wifi_state_ = WifiState::Disconnected;
        wifi_strength_ = 0;
    } else {
        logger.insert_log(logType::INFO, "Connected to Wi-Fi!");
        logger.insert_logf(logType::INFO, "IP address: %s", WiFi.localIP().toString().c_str());
        wifi_state_ = WifiState::Connected;
        wifi_strength_ = wifiStrengthFromRssi(WiFi.RSSI());

        // sync system clock via NTP
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    }
}

bool KynoWave::getLocalTimeInfo(struct tm& out) const {
    return ::getLocalTime(&out, 10);
}

void KynoWave::routine() {
    // handle OTA
    ota.handle();

    // check MQTT 
    MQTT_handle();

    // check for serial input and run the command in the shell if there is any, 
    // this allows us to interact with the shell from the serial monitor while the tasks are running
    String command;
    uint8_t source_index = 0;
    if (!hub.readAny(command, &source_index))
        return;
        
    // run the command in the shell and get the result code
    const char* source_name = hub.inputName(source_index);
    if (source_name == nullptr) source_name = "unknown";
    logger.insert_logf(logType::CMDO, "cmd from %s: %s", source_name, command.c_str());
    uint8_t result = shell.run_command_line(command.c_str());

    // if return some error, make something...
    if (result != RESULT_OK)
        logger.insert_logf(logType::ERRO, "Error code: %d", result);
}

bool KynoWave::sendToHub(const uint8_t *data, size_t len) {
    if (instante_ == nullptr) return false;
    return instante_->hub.sendToAll(data, len);
}

void KynoWave::mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    if (instante_ == nullptr) return;
    instante_->onMqttMessage(topic, payload, length);
}

void KynoWave::onMqttMessage(const char* topic, const uint8_t* payload, unsigned int length) {
    if (topic == nullptr || payload == nullptr || length == 0)
        return;

    if (strcmp(topic, mqtt_cmd_topic) != 0)
        return;

    const unsigned int max_len = 255;
    unsigned int copy_len = (length > max_len) ? max_len : length;
    char buffer[max_len + 1];
    memcpy(buffer, payload, copy_len);
    buffer[copy_len] = '\0';

    mqtt_cmd_buffer_ = String(buffer);
    mqtt_cmd_pending_ = true;
}

bool KynoWave::readFromMqtt(String& input) {
    if (!mqtt_cmd_pending_)
        return false;

    input = mqtt_cmd_buffer_;
    mqtt_cmd_buffer_ = "";
    mqtt_cmd_pending_ = false;
    return true;
}
