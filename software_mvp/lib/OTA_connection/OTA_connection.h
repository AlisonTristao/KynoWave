#ifndef OTA_CONNECTION_H
#define OTA_CONNECTION_H

// external libraries
#include <stdint.h>

class Logger;
class DisplayLCD;

class OTAConnection {
public:
    OTAConnection();

    void begin(Logger& logger, DisplayLCD& display);
    void handle();

private:
    static OTAConnection* instance_;
    Logger* logger_;
    DisplayLCD* display_;
    bool started_;
    uint8_t last_percentage_;

    void startIfReady();
    void onStart();
    void onEnd();
    void onProgress(unsigned int progress, unsigned int total);
    void onError(uint8_t error);
};

#endif // OTA_CONNECTION_H