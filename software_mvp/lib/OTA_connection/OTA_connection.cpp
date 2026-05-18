#include <OTA_connection.h>
#include <Logger.h>
#include <DisplayLCD.h>

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

// ota settings
#ifndef OTA_HOSTNAME
#define OTA_HOSTNAME "KynoWave"
#warning "OTA_HOSTNAME is not defined in platformio.ini, using default: KynoWave"
#endif

#ifndef OTA_PASSWORD
#define OTA_PASSWORD "657585"
#warning "OTA_PASSWORD is not defined in platformio.ini, using default: 657585"
#endif

OTAConnection* OTAConnection::instance_ = nullptr;

OTAConnection::OTAConnection()
    : logger_(nullptr),
      display_(nullptr),
      started_(false),
      last_percentage_(255) {}

void OTAConnection::begin(Logger& logger, DisplayLCD& display) {
    // save the references used by OTA callbacks
    logger_ = &logger;
    display_ = &display;
    instance_ = this;
    last_percentage_ = 255;

    // log the OTA setup start
    logger_->insert_log(logType::INFO, "Setting up OTA...");

    // setup the OTA service
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    // define callbacks for OTA events
    ArduinoOTA.onStart([]() {
        if (instance_) instance_->onStart();
    });
    ArduinoOTA.onEnd([]() {
        if (instance_) instance_->onEnd();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (instance_) instance_->onProgress(progress, total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        if (instance_) instance_->onError(static_cast<uint8_t>(error));
    });

    startIfReady();
}

void OTAConnection::handle() {
    // ensure OTA is started once Wi-Fi is ready
    startIfReady();
    if (started_)
        ArduinoOTA.handle();
}

void OTAConnection::startIfReady() {
    if (started_ || logger_ == nullptr || display_ == nullptr)
        return;

    if (WiFi.status() != WL_CONNECTED)
        return;

    // start OTA only after Wi-Fi is connected
    ArduinoOTA.begin();
    started_ = true;
    logger_->insert_log(logType::INFO, "OTA finished!");
}

void OTAConnection::onStart() {
    if (!logger_ || !display_) return;

    display_->requestTerminalClear();
    logger_->insert_log(logType::INFO, "OTA Update in progress...");
}

void OTAConnection::onEnd() {
    if (!logger_ || !display_) return;

    logger_->insert_log(logType::INFO, "\nUpdate Finished!");
    logger_->insert_log(logType::INFO, "\nKynoWave will restart in 3 seconds!");

    for (int i = 3; i > 0; --i) {
        logger_->insert_logf(logType::INFO, "Restarting in %d seconds...\r", i);
        delay(1000);
    }
    delay(500);
    display_->high(false);
}

void OTAConnection::onProgress(unsigned int progress, unsigned int total) {
    if (!logger_ || total == 0) return;

    uint8_t percentage = (progress * 100) / total;
    if (percentage == last_percentage_)
        return;

    last_percentage_ = percentage;
    if (percentage % 10 == 0)
        logger_->insert_logf(logType::INFO, "Progress: %u%%\r", percentage);
}

void OTAConnection::onError(uint8_t error) {
    if (!logger_) return;

    logger_->insert_logf(logType::INFO, "Erro[%u]: ", error);
    switch (error) {
        case OTA_AUTH_ERROR:    logger_->insert_log(logType::INFO, "Authentication Error"); break;
        case OTA_BEGIN_ERROR:   logger_->insert_log(logType::INFO, "Begin Error"); break;
        case OTA_CONNECT_ERROR: logger_->insert_log(logType::INFO, "Connect Error"); break;
        case OTA_RECEIVE_ERROR: logger_->insert_log(logType::INFO, "Receive Error"); break;
        case OTA_END_ERROR:     logger_->insert_log(logType::INFO, "End Error"); break;
    }
}
