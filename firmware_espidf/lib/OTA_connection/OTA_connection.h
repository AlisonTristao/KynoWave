#ifndef OTA_CONNECTION_H
#define OTA_CONNECTION_H

// external libraries
#include <stdint.h>

class Logger;
class DisplayLCD;

class OTAConnection {
public:
    OTAConnection();

    /*
    * @brief Setup OTA handlers and start when Wi-Fi is ready.
    *
    * @param logger Logger reference used to print OTA status.
    * @param display Display reference used to show OTA progress.
    */
    void begin(Logger& logger, DisplayLCD& display, const char* hostname, const char* password);

    /*
    * @brief Handle OTA state machine (call periodically).
    */
    void handle();

    void applySettings(const char* hostname, const char* password);

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