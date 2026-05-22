#ifndef AUDIO_UDP_STREAMER_H
#define AUDIO_UDP_STREAMER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

class AudioUdpStreamer {
public:
    AudioUdpStreamer();

    bool begin(
        uint8_t micPin,
        uint32_t sampleRate,
        uint16_t numSamples,
        const char* remoteIp,
        uint16_t remotePort
    );

    uint8_t routine();

private:
    uint8_t captureBlock();
    bool sendBlock();

    uint8_t micPin_;
    uint32_t sampleRate_;
    uint16_t numSamples_;

    IPAddress remoteIp_;
    uint16_t remotePort_;

    WiFiUDP udp_;
    int16_t* audioBuffer_;

    bool initialized_;
};

#endif