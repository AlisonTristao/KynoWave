#ifndef KYNOWAVE_CONFIG_STORE_H
#define KYNOWAVE_CONFIG_STORE_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>

#ifndef WIFI_SSID
#define WIFI_SSID "your_ssid"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "your_password"
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER "192.168.0.100"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_TOPIC
#define MQTT_TOPIC "kynowave/logs"
#endif

#ifndef MQTT_CMD_TOPIC
#define MQTT_CMD_TOPIC "kynowave/cmd"
#endif

#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "KynoWave"
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD "124578"
#endif

struct KynoWaveConfig {
    String wifiSsid;
    String wifiPassword;
    String mqttServer;
    uint16_t mqttPort = MQTT_PORT;
    String mqttTopic;
    String mqttCmdTopic;
    String otaHostname;
    String otaPassword;
};

class KynoWaveConfigStore {
public:
    KynoWaveConfigStore() = default;

    bool begin() {
        return SPIFFS.begin(true);
    }

    bool load(KynoWaveConfig& out) {
        applyDefaults(out);

        if (!SPIFFS.begin(true)) {
            return false;
        }

        if (!SPIFFS.exists(filePath_)) {
            return save(out);
        }

        File file = SPIFFS.open(filePath_, "r");
        if (!file) {
            return false;
        }

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#")) {
                continue;
            }

            int sep = line.indexOf('=');
            if (sep <= 0) {
                continue;
            }

            String key = line.substring(0, sep);
            String value = line.substring(sep + 1);
            key.trim();
            value.trim();

            if (key == "wifi.ssid") {
                out.wifiSsid = value;
            } else if (key == "wifi.password") {
                out.wifiPassword = value;
            } else if (key == "mqtt.server") {
                out.mqttServer = value;
            } else if (key == "mqtt.port") {
                out.mqttPort = static_cast<uint16_t>(value.toInt());
            } else if (key == "mqtt.topic") {
                out.mqttTopic = value;
            } else if (key == "mqtt.cmd_topic") {
                out.mqttCmdTopic = value;
            } else if (key == "ota.hostname") {
                out.otaHostname = value;
            } else if (key == "ota.password") {
                out.otaPassword = value;
            }
        }

        file.close();
        sanitize(out);
        return true;
    }

    bool save(const KynoWaveConfig& cfg) {
        if (!SPIFFS.begin(true)) {
            return false;
        }

        File file = SPIFFS.open(filePath_, "w");
        if (!file) {
            return false;
        }

        file.print("wifi.ssid=");
        file.println(cfg.wifiSsid);
        file.print("wifi.password=");
        file.println(cfg.wifiPassword);
        file.print("mqtt.server=");
        file.println(cfg.mqttServer);
        file.print("mqtt.port=");
        file.println(static_cast<unsigned int>(cfg.mqttPort));
        file.print("mqtt.topic=");
        file.println(cfg.mqttTopic);
        file.print("mqtt.cmd_topic=");
        file.println(cfg.mqttCmdTopic);
        file.print("ota.hostname=");
        file.println(cfg.otaHostname);
        file.print("ota.password=");
        file.println(cfg.otaPassword);

        file.close();
        return true;
    }

private:
    const char* filePath_ = "/kynowave.cfg";

    void applyDefaults(KynoWaveConfig& out) const {
        out.wifiSsid = WIFI_SSID;
        out.wifiPassword = WIFI_PASSWORD;
        out.mqttServer = MQTT_SERVER;
        out.mqttPort = MQTT_PORT;
        out.mqttTopic = MQTT_TOPIC;
        out.mqttCmdTopic = MQTT_CMD_TOPIC;
        out.otaHostname = OTA_HOSTNAME;
        out.otaPassword = OTA_PASSWORD;
    }

    void sanitize(KynoWaveConfig& cfg) const {
        if (cfg.wifiSsid.length() == 0) cfg.wifiSsid = WIFI_SSID;
        if (cfg.wifiPassword.length() == 0) cfg.wifiPassword = WIFI_PASSWORD;
        if (cfg.mqttServer.length() == 0) cfg.mqttServer = MQTT_SERVER;
        if (cfg.mqttPort == 0) cfg.mqttPort = MQTT_PORT;
        if (cfg.mqttTopic.length() == 0) cfg.mqttTopic = MQTT_TOPIC;
        if (cfg.mqttCmdTopic.length() == 0) cfg.mqttCmdTopic = MQTT_CMD_TOPIC;
        if (cfg.otaHostname.length() == 0) cfg.otaHostname = OTA_HOSTNAME;
        if (cfg.otaPassword.length() == 0) cfg.otaPassword = OTA_PASSWORD;
    }
};

#endif // KYNOWAVE_CONFIG_STORE_H
