// external libraries
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

// internal libraries
#include <KynoWave.h>

const char* ssid = "ole telecom.129246";
const char* password = "474726264h";

// global pointer to the kynowave instance, to use it in the OTA callbacks
KynoWave* ptr_kynowave = nullptr;

void onOtaStart() {
    if (!ptr_kynowave) return;
    
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
    } else { // U_SPIFFS
        type = "filesystem";
    }
    ptr_kynowave->display.clear();
    ptr_kynowave->logger.insert_log(logType::INFO, "OTA Update in progress...");
}

void onOtaEnd() {
    if (!ptr_kynowave) return;
    ptr_kynowave->logger.insert_log(logType::INFO, "\nUpdate Finished!");
    ptr_kynowave->logger.insert_log(logType::INFO, "\nKynoWave will restart in 3 seconds!");

    for (int i = 3; i > 0; --i) {
        ptr_kynowave->logger.insert_logf(logType::INFO, "Restarting in %d seconds...\r", i);
        delay(1000);
    }
    delay(500);
    ptr_kynowave->display.high(false);
}

void onOtaProgress(unsigned int progress, unsigned int total) {
    if (total == 0) return; 
    uint8_t percentage = (progress * 100) / total;
    static uint8_t last_percentage = 255; 

    // only log and update the display if the percentage has changed to avoid flooding the logs and the display with too many updates
    if (percentage == last_percentage)
        return;
        
    // update the percentage
    last_percentage = percentage; 
    if (percentage % 10 == 0)   ptr_kynowave->logger.insert_logf(logType::INFO, "Progress: %u%%\r", percentage);
}

void onOtaError(ota_error_t error) {
    ptr_kynowave->logger.insert_logf(logType::INFO, "Erro[%u]: ", error);
    if (!ptr_kynowave) return;
    switch (error) {
        case OTA_AUTH_ERROR:    ptr_kynowave->logger.insert_log(logType::INFO, "Authentication Error"); break;
        case OTA_BEGIN_ERROR:   ptr_kynowave->logger.insert_log(logType::INFO, "Begin Error"); break;
        case OTA_CONNECT_ERROR: ptr_kynowave->logger.insert_log(logType::INFO, "Connect Error"); break;
        case OTA_RECEIVE_ERROR: ptr_kynowave->logger.insert_log(logType::INFO, "Receive Error"); break;
        case OTA_END_ERROR:     ptr_kynowave->logger.insert_log(logType::INFO, "End Error"); break;
    }
}

void setup_ota(KynoWave& kynowave) {
    // save the global pointer to the kynowave instance, to use it in the OTA callbacks
    ptr_kynowave = &kynowave;

    // wifi connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    const uint8_t max_attempts = 5; 
    bool success = false;
    
    kynowave.logger.insert_logf(logType::INFO, "Connecting to Wi-Fi: %s", ssid);

    for (uint8_t attempts = 0; attempts < max_attempts; ++attempts) {
        if (WiFi.waitForConnectResult() == WL_CONNECTED) {
            success = true;
            break; 
        }
        kynowave.logger.insert_log(logType::INFO, "Failed to connect to Wi-Fi! Trying again...");
        delay(1000);
    }

    if (!success) {
        kynowave.logger.insert_log(logType::INFO, "Failed to connect to Wi-Fi!");
        return; 
    }

    // setup the OTA service
    ArduinoOTA.setHostname("KynoWave"); 
    ArduinoOTA.setPassword("657585");

    // define os callbacks para os eventos de OTA
    ArduinoOTA.onStart(onOtaStart);
    ArduinoOTA.onEnd(onOtaEnd);
    ArduinoOTA.onProgress(onOtaProgress);
    ArduinoOTA.onError(onOtaError);

    // start the OTA service
    ArduinoOTA.begin();

    kynowave.logger.insert_log(logType::INFO, "OTA finished!");
    kynowave.logger.insert_logf(logType::INFO, "IP Address: %s", WiFi.localIP().toString().c_str());
}