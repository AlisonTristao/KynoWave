#ifndef KYNOWAVE_H
#define KYNOWAVE_H

// external libraries
#include <Arduino.h>
#include <TinyShell.h>
#include <PubSubClient.h>
#include <WiFi.h>

// internal libraries
#include <Logger.h>
#include <DisplayLCD.h>

// define a baud rate for the serial communication
#ifndef BAUDRATE
#define BAUDRATE 115200 // default baud rate if not defined in platformio.ini
// add a warning to compile time to indicate that the default baud rate is being used
#warning "BAUDRATE is not defined in platformio.ini, using default baud rate: 115200"
#endif

class KynoWave {
    public:
        KynoWave() {
            if (instante_ == nullptr)
                instante_ = this;
            
            // initialize the MQTT client with the WiFi client
            clientMQTT.setClient(espClient);
        };

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
         * @brief Handle MQTT communication
         */
        void MQTT_handle();
        
        /**
         * @brief Setup the MQTT connection, this will connect to the WiFi and then to the MQTT broker, and set the callback for incoming messages
         */
        void setup_MQTT();

        /**
         * @brief Connect to the Wi-Fi network
         */
        void connectToWiFi();

    private:
        // static instance of the KynoWave class, this will be used to access the logger and shell from the tasks
        static KynoWave* instante_;

        // wrapper functions to be used in the shell
        static uint8_t wrapper_h();         // explain the available modules and commands

        const char* mqtt_server = "192.168.0.100";
        const int mqtt_port = 1883;
        const char* mqtt_topic = "kynowave/logs";
        const char* ssid = "ole telecom.129246";
        const char* password = "474726264h";
        const String clientId = "KynoWaveMVP_01";

        /**
         * @brief Reconnect to the MQTT broker
         */
        void reconnect();
};  

#endif // KYNOWAVE_H