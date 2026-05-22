#include "KynoWave.h"
#include <PubSubClient.h>
#include <cstring>
#include <time.h>
#include "Serial.printlnh"

// initialize the static members of the KynoWave class
KynoWave* KynoWave::instante_ = nullptr;

static uint8_t wifiStrengthFromRssi(int32_t rssi) {
    if (rssi >= -50) return 4;
    if (rssi >= -65) return 3;
    if (rssi >= -75) return 2;
    if (rssi >= -85) return 1;
    return 0;
}

KynoWave::KynoWave(const String& mqtt_name)
    : clientId(mqtt_name),
      hub(),
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
    if (data == nullptr || len < sizeof(message)) return false;
    const message* msg = reinterpret_cast<const message*>(data);
    char buffer[512];

    if (msg->type == logType::I2SD) {
        size_t num_samples = msg->content.size / sizeof(int16_t);
        size_t offset = 0;

        offset += snprintf(
            buffer + offset,
            sizeof(buffer) - offset,
            "[%lu ms] [%s] [%u/%u] ",
            (unsigned long)msg->timer,
            logTypeToString(msg->type),
            msg->packet_number,
            msg->total_packets
        );

        for (size_t i = 0; i < num_samples && offset < sizeof(buffer); ++i) {
            offset += snprintf(
                buffer + offset,
                sizeof(buffer) - offset,
                "%d ",
                msg->content.i2sD[i]
            );
        }

    } else {
        snprintf(
            buffer,
            sizeof(buffer),
            "[%lu ms] [%s] [%u/%u] %s",
            (unsigned long)msg->timer,
            logTypeToString(msg->type),
            msg->packet_number,
            msg->total_packets,
            msg->content.text
        );
    }

    switch (msg->type) {
        case logType::INFO:
        case logType::CMDO:
            Serial.println("SerialSink", "%s", buffer);
            break;

        case logType::WARN:
            Serial.println("SerialSink", "%s", buffer);
            break;

        case logType::ERRO:
            Serial.println("SerialSink", "%s", buffer);
            break;

        case logType::DEBG:
        case logType::I2SD:
            Serial.println("SerialSink", "%s", buffer);
            break;

        default:
            Serial.println("SerialSink", "%s", buffer);
            break;
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
        String topic = instante_->config_.mqttTopic;

        if (msg->type == logType::I2SD) topic += "/i2s_data";    
        else                            topic += "/status_logs"; 

    if (msg->type == logType::I2SD) {
        bool ok = instante_->clientMQTT.publish(
            topic.c_str(),
            reinterpret_cast<const uint8_t*>(msg->content.i2sD),
            msg->content.size
        );
    } else {
        return instante_->clientMQTT.publish(topic.c_str(), msg->content.text, static_cast<unsigned int>(len));
    }
}

// receive a string pointer
bool KynoWave::readFromSerial(String& input) {
    if (!Serial.available())
        return false;

    input = Serial.readStringUntil('\n');
    input.trim();
    return input.length() > 0;
}

void KynoWave::setLedsIntensity(uint8_t intensity) {
    // convert the value to bits 
    uint8_t state = 0;
    for (uint8_t i = 0; i < NUM_LEDS; ++i)
        if (intensity > i) 
            state |= (1 << i);

    // set the state 
    setLeds(state);
}

void KynoWave::setup_leds() {
    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        pinMode(leds_pins_array[i], OUTPUT);
        digitalWrite(leds_pins_array[i], LOW);
    }
}

void KynoWave::setLeds(uint8_t state) {
    for (uint8_t i = 0; i < NUM_LEDS; ++i){
        uint8_t intensity = (state & (1 << i)) ? 255 : 0;
        if (i == 0 || i == 1) intensity /= 10;
        analogWrite(leds_pins_array[i], intensity);
    }
}

void KynoWave::begin() {
    // init the serial communication and print a log message to indicate that the serial is initialized
    Serial.begin(BAUDRATE);
    Serial.printlnlevel_set("*", LOG_LEVEL);
    WiFi.setSleep(false);

    // setup the logger
    logger.begin();
    logger.set_send_callback(KynoWave::sendToHub);
    logger.insert_logf(logType::INFO, "Serial initialized at baud: %u", BAUDRATE);

    if (!config_store_.load(config_)) {
        logger.insert_log(logType::ERRO, "Config load failed. Using defaults.");
    }

    // setup device hub
    hub.addOutput(&serial_sink, "serial");
    //hub.addOutput(&mqtt_sink, "mqtt");
    hub.addOutput(&display_sink, "display");
    hub.addInput(&serial_source, "serial");
    //hub.addInput(&mqtt_source, "mqtt");

    // setup the shell input callback    
    shell.begin();
    shell.set_output_callback([](const std::string& text) {
        if (text.empty())
            return;
        if (instante_)
            instante_->logger.insert_logf(logType::DEBG, "Shell output: %s", text.c_str());
    });

    // setup the display
    display.begin();

    // setup the kyno basic module
    shell.create_module("kyno", "KynoWave main module");
    shell.add(std::function<uint8_t()>([this](){ 
        instante_->logger.insert_log(logType::INFO, "Pong!");    
        return RESULT_OK; 
    }), "ping", "Ping the KynoWave to check if it's alive", "kyno");
    shell.add(std::function<uint8_t(uint8_t)>([this](uint8_t intensity){ 
        instante_->setLedsIntensity(intensity);
        return RESULT_OK;
    }), "set_leds", "Set the intensity of the LEDs", "kyno");
    registerShellModules();

    // setup the MQTT client
    //applyMqttSettings();
    //clientMQTT.setBufferSize(2048);
    //clientMQTT.setCallback(KynoWave::mqttCallback);

    // setup OTA
    //ota.begin(logger, display, config_.otaHostname.c_str(), config_.otaPassword.c_str());

    // setup the LEDs
    setup_leds();
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
    // verify MQTT connection before trying to reconnect
    if (clientMQTT.connected())
        return;

        // try to reconnect to MQTT broker
        logger.insert_log(logType::INFO, "Trying to connect to MQTT...");

        if (clientMQTT.connect(clientId.c_str())) {
            // once connected, publish an announcement and subscribe to the command topic
            logger.insert_logf(logType::INFO, "Connected to Broker: %s", config_.mqttServer.c_str());
            clientMQTT.subscribe(config_.mqttCmdTopic.c_str());
            return;
        } 

    // if connection failed, log the error code
    logger.insert_logf(logType::INFO, "Failed, rc=%d", clientMQTT.state());
}

void KynoWave::connectToWiFi() {
    // wifi connection
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str());
    wifi_state_ = WifiState::Connecting;
    wifi_strength_ = 0;

    const uint8_t max_attempts = 1; 
    bool success = false;
    
    logger.insert_logf(logType::INFO, "Connecting to Wi-Fi: %s", config_.wifiSsid.c_str());

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
    // handle OTA and MqTT (wifi must be connected)
    //if (wifi_state_ == WifiState::Connected) {
    //    ota.handle();
    //    //MQTT_handle();
    //}

    // set the leds state
    setLedsIntensity(leds_state_);

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

    if (strcmp(topic, config_.mqttCmdTopic.c_str()) != 0)
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
    input.trim();
    Serial.printf("Received MQTT command: %s\n", input.c_str());
    return input.length() > 0;
}

void KynoWave::registerShellModules() {
    shell.create_module("wifi", "Wi-Fi settings");
    shell.add(std::function<uint8_t()>([this]() { return cmdWifiGetSsid(); }), "get_ssid", "Get Wi-Fi SSID", "wifi");
    shell.add(std::function<uint8_t(std::string)>([this](std::string ssid) {
        return cmdWifiSetSsid(ssid);
    }), "set_ssid", "Set Wi-Fi SSID", "wifi");
    shell.add(std::function<uint8_t()>([this]() { return cmdWifiGetPassword(); }), "get_password", "Get Wi-Fi password", "wifi");
    shell.add(std::function<uint8_t(std::string)>([this](std::string password) {
        return cmdWifiSetPassword(password);
    }), "set_password", "Set Wi-Fi password", "wifi");
    shell.add(std::function<uint8_t()>([this]() { return cmdWifiGetState(); }), "get_state", "Get Wi-Fi state", "wifi");
    shell.add(std::function<uint8_t()>([this]() { return cmdWifiReconnect(); }), "reconnect", "Reconnect to Wi-Fi network", "wifi");

    shell.create_module("mqtt", "MQTT settings");
    shell.add(std::function<uint8_t()>([this]() { return cmdMqttGetServer(); }), "get_server", "Get MQTT server", "mqtt");
    shell.add(std::function<uint8_t(std::string)>([this](std::string server) {
        return cmdMqttSetServer(server);
    }), "set_server", "Set MQTT server", "mqtt");
    shell.add(std::function<uint8_t()>([this]() { return cmdMqttGetPort(); }), "get_port", "Get MQTT port", "mqtt");
    shell.add(std::function<uint8_t(uint16_t)>([this](uint16_t port) {
        return cmdMqttSetPort(port);
    }), "set_port", "Set MQTT port", "mqtt");
    shell.add(std::function<uint8_t()>([this]() { return cmdMqttGetTopic(); }), "get_topic", "Get MQTT topic", "mqtt");
    shell.add(std::function<uint8_t(std::string)>([this](std::string topic) {
        return cmdMqttSetTopic(topic);
    }), "set_topic", "Set MQTT topic", "mqtt");
    shell.add(std::function<uint8_t()>([this]() { return cmdMqttGetCmdTopic(); }), "get_cmd_topic", "Get MQTT command topic", "mqtt");
    shell.add(std::function<uint8_t(std::string)>([this](std::string topic) {
        return cmdMqttSetCmdTopic(topic);
    }), "set_cmd_topic", "Set MQTT command topic", "mqtt");
    shell.add(std::function<uint8_t()>([this]() { MQTT_handle(); return RESULT_OK; }), "reconnect", "Reconnect to MQTT broker with current settings", "mqtt");
    // send a message to mqtt 
    shell.add(std::function<uint8_t()>([this]() { 
        message test_msg;
        test_msg.type = logType::CMDO;
        snprintf(test_msg.content.text, sizeof(test_msg.content.text), "Hello MQTT! Time: %lu", (unsigned long)millis());
        test_msg.content.size = strlen(test_msg.content.text);
        return sendToMQTT(reinterpret_cast<const uint8_t*>(&test_msg), sizeof(test_msg)) ? RESULT_OK : RESULT_ERROR; 
    }), "ping", "Ping MQTT broker", "mqtt");

    shell.create_module("ota", "OTA settings");
    shell.add(std::function<uint8_t()>([this]() { return cmdOtaGetHostname(); }), "get_hostname", "Get OTA hostname", "ota");
    shell.add(std::function<uint8_t(std::string)>([this](std::string hostname) {
        return cmdOtaSetHostname(hostname);
    }), "set_hostname", "Set OTA hostname", "ota");
    shell.add(std::function<uint8_t()>([this]() { return cmdOtaGetPassword(); }), "get_password", "Get OTA password", "ota");
    shell.add(std::function<uint8_t(std::string)>([this](std::string password) {
        return cmdOtaSetPassword(password);
    }), "set_password", "Set OTA password", "ota");
}

void KynoWave::applyMqttSettings() {
    clientMQTT.setServer(config_.mqttServer.c_str(), config_.mqttPort);
}

void KynoWave::applyOtaSettings() {
    ota.applySettings(config_.otaHostname.c_str(), config_.otaPassword.c_str());
}

bool KynoWave::saveConfig() {
    if (config_store_.save(config_)) {
        return true;
    }

    logger.insert_log(logType::ERRO, "Failed to save config to SPIFFS.");
    return false;
}

uint8_t KynoWave::cmdWifiGetSsid() {
    logger.insert_logf(logType::INFO, "Wi-Fi SSID: %s", config_.wifiSsid.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdWifiSetSsid(const std::string& ssid) {
    if (ssid.empty()) {
        logger.insert_log(logType::ERRO, "Wi-Fi SSID cannot be empty.");
        return RESULT_ERROR;
    }

    config_.wifiSsid = ssid.c_str();
    if (!saveConfig()) return RESULT_ERROR;
    logger.insert_logf(logType::INFO, "Wi-Fi SSID updated: %s", config_.wifiSsid.c_str());
    connectToWiFi();
    return RESULT_OK;
}

uint8_t KynoWave::cmdWifiGetPassword() {
    logger.insert_logf(logType::INFO, "Wi-Fi password: %s", config_.wifiPassword.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdWifiSetPassword(const std::string& password) {
    if (password.empty()) {
        logger.insert_log(logType::ERRO, "Wi-Fi password cannot be empty.");
        return RESULT_ERROR;
    }

    config_.wifiPassword = password.c_str();
    if (!saveConfig()) return RESULT_ERROR;
    logger.insert_log(logType::INFO, "Wi-Fi password updated.");
    connectToWiFi();
    return RESULT_OK;
}

uint8_t KynoWave::cmdWifiGetState() {
    const char* state_str = "Unknown";
    switch (wifi_state_) {
        case WifiState::Disconnected: state_str = "Disconnected"; break;
        case WifiState::Connecting:   state_str = "Connecting"; break;
        case WifiState::Connected:    state_str = "Connected"; break;
    }
    logger.insert_logf(logType::INFO, "Wi-Fi state: %s", state_str);
    return RESULT_OK;
}

uint8_t KynoWave::cmdWifiReconnect() {
    logger.insert_log(logType::INFO, "Reconnecting to Wi-Fi...");
    connectToWiFi();
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttGetServer() {
    logger.insert_logf(logType::INFO, "MQTT server: %s", config_.mqttServer.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttSetServer(const std::string& server) {
    if (server.empty()) {
        logger.insert_log(logType::ERRO, "MQTT server cannot be empty.");
        return RESULT_ERROR;
    }

    config_.mqttServer = server.c_str();
    if (!saveConfig()) return RESULT_ERROR;
    applyMqttSettings();
    clientMQTT.disconnect();
    reconnect();
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttGetPort() {
    logger.insert_logf(logType::INFO, "MQTT port: %u", static_cast<unsigned int>(config_.mqttPort));
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttSetPort(uint16_t port) {
    if (port == 0) {
        logger.insert_log(logType::ERRO, "MQTT port must be greater than zero.");
        return RESULT_ERROR;
    }

    config_.mqttPort = port;
    if (!saveConfig()) return RESULT_ERROR;
    applyMqttSettings();
    clientMQTT.disconnect();
    reconnect();
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttGetTopic() {
    logger.insert_logf(logType::INFO, "MQTT topic: %s", config_.mqttTopic.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttSetTopic(const std::string& topic) {
    if (topic.empty()) {
        logger.insert_log(logType::ERRO, "MQTT topic cannot be empty.");
        return RESULT_ERROR;
    }

    config_.mqttTopic = topic.c_str();
    if (!saveConfig()) return RESULT_ERROR;
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttGetCmdTopic() {
    logger.insert_logf(logType::INFO, "MQTT command topic: %s", config_.mqttCmdTopic.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdMqttSetCmdTopic(const std::string& topic) {
    if (topic.empty()) {
        logger.insert_log(logType::ERRO, "MQTT command topic cannot be empty.");
        return RESULT_ERROR;
    }

    String old_topic = config_.mqttCmdTopic;
    config_.mqttCmdTopic = topic.c_str();
    if (!saveConfig()) return RESULT_ERROR;

    if (clientMQTT.connected()) {
        clientMQTT.unsubscribe(old_topic.c_str());
        clientMQTT.subscribe(config_.mqttCmdTopic.c_str());
    }
    return RESULT_OK;
}

uint8_t KynoWave::cmdOtaGetHostname() {
    logger.insert_logf(logType::INFO, "OTA hostname: %s", config_.otaHostname.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdOtaSetHostname(const std::string& hostname) {
    if (hostname.empty()) {
        logger.insert_log(logType::ERRO, "OTA hostname cannot be empty.");
        return RESULT_ERROR;
    }

    config_.otaHostname = hostname.c_str();
    if (!saveConfig()) return RESULT_ERROR;
    applyOtaSettings();
    return RESULT_OK;
}

uint8_t KynoWave::cmdOtaGetPassword() {
    logger.insert_logf(logType::INFO, "OTA password: %s", config_.otaPassword.c_str());
    return RESULT_OK;
}

uint8_t KynoWave::cmdOtaSetPassword(const std::string& password) {
    if (password.empty()) {
        logger.insert_log(logType::ERRO, "OTA password cannot be empty.");
        return RESULT_ERROR;
    }

    config_.otaPassword = password.c_str();
    if (!saveConfig()) return RESULT_ERROR;
    applyOtaSettings();
    return RESULT_OK;
}
