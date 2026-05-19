#ifndef KYNOWAVE_H
#define KYNOWAVE_H

// external libraries
#include <Arduino.h>
#include <TinyShell.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <time.h>
#include <string>

// internal libraries
#include <Logger.h>
#include <DisplayLCD.h>
#include <DeviceHub.h>
#include <OTA_connection.h>

// define a baud rate for the serial communication
#ifndef BAUDRATE
#define BAUDRATE 115200 // default baud rate if not defined in platformio.ini
// add a warning to compile time to indicate that the default baud rate is being used
#warning "BAUDRATE is not defined in platformio.ini, using default baud rate: 115200"
#endif

// wifi settings
#ifndef WIFI_SSID
#define WIFI_SSID "your_ssid"
#warning "WIFI_SSID is not defined in platformio.ini, using default: your_ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your_password"
#warning "WIFI_PASSWORD is not defined in platformio.ini, using default: your_password"
#endif

// time settings (NTP)
#ifndef NTP_SERVER_1
#define NTP_SERVER_1 "pool.ntp.org"
#warning "NTP_SERVER_1 is not defined in platformio.ini, using default: pool.ntp.org"
#endif

#ifndef NTP_SERVER_2
#define NTP_SERVER_2 "time.nist.gov"
#warning "NTP_SERVER_2 is not defined in platformio.ini, using default: time.nist.gov"
#endif

#ifndef NTP_SERVER_3
#define NTP_SERVER_3 "time.google.com"
#warning "NTP_SERVER_3 is not defined in platformio.ini, using default: time.google.com"
#endif

#ifndef GMT_OFFSET_SEC
#define GMT_OFFSET_SEC 0
#warning "GMT_OFFSET_SEC is not defined in platformio.ini, using default: 0"
#endif

#ifndef DAYLIGHT_OFFSET_SEC
#define DAYLIGHT_OFFSET_SEC 0
#warning "DAYLIGHT_OFFSET_SEC is not defined in platformio.ini, using default: 0"
#endif

// mqtt settings
#ifndef MQTT_SERVER
#define MQTT_SERVER "192.168.0.100"
#warning "MQTT_SERVER is not defined in platformio.ini, using default: 192.168.0.100"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#warning "MQTT_PORT is not defined in platformio.ini, using default: 1883"
#endif

#ifndef MQTT_TOPIC
#define MQTT_TOPIC "kynowave/logs"
#warning "MQTT_TOPIC is not defined in platformio.ini, using default: kynowave/logs"
#endif

#ifndef MQTT_CMD_TOPIC
#define MQTT_CMD_TOPIC "kynowave/cmd"
#warning "MQTT_CMD_TOPIC is not defined in platformio.ini, using default: kynowave/cmd"
#endif

#include <KynoWaveConfigStore.h>

class KynoWave {
    public:
        KynoWave();

        void begin();

        // members
        Logger logger;
        TinyShell shell;
        DisplayLCD display;
        WiFiClient espClient;
        PubSubClient clientMQTT;

        /*
        * @brief Default output callback, 
        * This function print print the log on serial, 
        * 
        * @param data Pointer to the log message data, which is expected to be a pointer to a message struct.
        * @param len Length of the data in bytes, which should be equal to the size
        */
        static bool sendToSerial(const uint8_t *data, size_t len);
        
        /*
        * @brief default input callback,
        * This function is a placeholder for handling input commands,
        * it can be implemented to read commands from serial or other input methods and pass them to the TinyShell for processing.
        */
        static bool readFromSerial(String& input);

        /*
        * @brief Default output callback for the device hub
        */
        static bool sendToHub(const uint8_t *data, size_t len);

        /**
         * @brief default output callback for MQTT,
         * This function will publish the log messages to a MQTT broker, using the PubSubClient
         * 
         * @param data Pointer to the log message data, which is expected to be a pointer to a message struct.
         * @param len Length of the data in bytes, which should be equal to the size of the message struct.
         * @return true if the message was published successfully, false otherwise (e.g., if
         */
        static bool sendToMQTT(const uint8_t *data, size_t len);

        /**
        * @brief return a void pointer to the instance of the Kyno
        * 
        * @return void pointer to the instance of the KynoWave class, this can be used to access the logger and shell from the tasks
        */
        static void* getInstance() {
            return static_cast<void*>(instante_);
        }

        /**
         * @brief Exceute the Kyno routines to handle the MQTT communication, 
         * this should be called in the main loop or in a task to keep the MQTT connection alive and handle incoming messages.
         * 
         */

        void routine();

        /**
         * @brief Handle MQTT communication
         */
        void MQTT_handle();

        /**
         * @brief Connect to the Wi-Fi network
         */
        void connectToWiFi();

        WifiState wifiState() const { return wifi_state_; }
        uint8_t wifiStrength() const { return wifi_strength_; }
        bool mqttConnected() const { return mqtt_connected_; }
        bool getLocalTimeInfo(struct tm& out) const;

    private:
        // device adapters
        class SerialSink : public IOutputSink {
        public:
            explicit SerialSink(KynoWave& k) : k_(k) {}
            bool send(const uint8_t* data, size_t len) override;
        private:
            KynoWave& k_;
        };

        class DisplaySink : public IOutputSink {
        public:
            explicit DisplaySink(KynoWave& k) : k_(k) {}
            bool send(const uint8_t* data, size_t len) override;
        private:
            KynoWave& k_;
        };

        class MqttSink : public IOutputSink {
        public:
            explicit MqttSink(KynoWave& k) : k_(k) {}
            bool send(const uint8_t* data, size_t len) override;
        private:
            KynoWave& k_;
        };

        class SerialSource : public IInputSource {
        public:
            explicit SerialSource(KynoWave& k) : k_(k) {}
            bool read(String& out) override;
        private:
            KynoWave& k_;
        };

        class MqttSource : public IInputSource {
        public:
            explicit MqttSource(KynoWave& k) : k_(k) {}
            bool read(String& out) override;
        private:
            KynoWave& k_;
        };

        void setup_leds();
        void set_leds_from_flags();

        // static instance of the KynoWave class, this will be used to access the logger and shell from the tasks
        static KynoWave* instante_;

        // wrapper functions to be used in the shell
        static uint8_t wrapper_h();         // explain the available modules and commands

        const String clientId           = "KynoWaveMVP_01";

        KynoWaveConfig config_;
        KynoWaveConfigStore config_store_;

        OTAConnection ota;
        DeviceHub hub;
        SerialSink serial_sink;
        DisplaySink display_sink;
        MqttSink mqtt_sink;
        SerialSource serial_source;
        MqttSource mqtt_source;
        uint8_t leds_pins_array [6] = {LED_LEVEL_1, LED_LEVEL_2, LED_LEVEL_3, LED_LEVEL_4, LED_LEVEL_5, LED_LEVEL_6};

        WifiState wifi_state_ = WifiState::Disconnected;
        uint8_t wifi_strength_ = 0;
        bool mqtt_connected_ = false;
        bool mqtt_cmd_pending_ = false;
        String mqtt_cmd_buffer_;

        static void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
        void onMqttMessage(const char* topic, const uint8_t* payload, unsigned int length);
        bool readFromMqtt(String& input);

        void registerShellModules();
        void applyMqttSettings();
        void applyOtaSettings();
        bool saveConfig();

        uint8_t cmdWifiGetSsid();
        uint8_t cmdWifiSetSsid(const std::string& ssid);
        uint8_t cmdWifiGetPassword();
        uint8_t cmdWifiSetPassword(const std::string& password);

        uint8_t cmdMqttGetServer();
        uint8_t cmdMqttSetServer(const std::string& server);
        uint8_t cmdMqttGetPort();
        uint8_t cmdMqttSetPort(uint16_t port);
        uint8_t cmdMqttGetTopic();
        uint8_t cmdMqttSetTopic(const std::string& topic);
        uint8_t cmdMqttGetCmdTopic();
        uint8_t cmdMqttSetCmdTopic(const std::string& topic);

        uint8_t cmdOtaGetHostname();
        uint8_t cmdOtaSetHostname(const std::string& hostname);
        uint8_t cmdOtaGetPassword();
        uint8_t cmdOtaSetPassword(const std::string& password);

        /**
         * @brief Reconnect to the MQTT broker
         */
        void reconnect();
};  

#endif // KYNOWAVE_H