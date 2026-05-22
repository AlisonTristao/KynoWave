#include "AudioUdpStreamer.h"

AudioUdpStreamer::AudioUdpStreamer()
    : micPin_(0),
      sampleRate_(16000),
      numSamples_(256),
      remotePort_(5005),
      audioBuffer_(nullptr),
      initialized_(false) {}

bool AudioUdpStreamer::begin(
    uint8_t micPin,
    uint32_t sampleRate,
    uint16_t numSamples,
    const char* remoteIp,
    uint16_t remotePort
) {
    if (remoteIp == nullptr || numSamples == 0 || sampleRate == 0) {
        return false;
    }

    micPin_ = micPin;
    sampleRate_ = sampleRate;
    numSamples_ = numSamples;
    remotePort_ = remotePort;

    if (!remoteIp_.fromString(remoteIp)) {
        return false;
    }

    audioBuffer_ = new int16_t[numSamples_];

    if (audioBuffer_ == nullptr) {
        return false;
    }

    pinMode(micPin_, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(micPin_, ADC_11db);

    udp_.begin(0);

    initialized_ = true;
    return true;
}

uint8_t AudioUdpStreamer::captureBlock() {
    if (!initialized_ || audioBuffer_ == nullptr) {
        return 0;
    }

    static float dc = 2048.0f;

    static float lp1 = 0.0f;
    static float lp2 = 0.0f;
    static float lp3 = 0.0f;

    uint64_t sumSq = 0;

    const float cutoffHz = 3000.0f;
    const float dt = 1.0f / (float)sampleRate_;
    const float rc = 1.0f / (2.0f * PI * cutoffHz);
    const float alpha = dt / (rc + dt);

    const float dcAlpha = 0.001f;

    const uint32_t intervalUs = 1000000UL / sampleRate_;
    uint32_t nextSampleTime = micros();

    for (uint16_t i = 0; i < numSamples_; i++) {
        while ((int32_t)(micros() - nextSampleTime) < 0) {
            // espera próxima amostra
        }

        nextSampleTime += intervalUs;

        int adc = analogRead(micPin_);

        dc += dcAlpha * ((float)adc - dc);

        float x = (float)adc - dc;

        lp1 += alpha * (x - lp1);
        lp2 += alpha * (lp1 - lp2);
        lp3 += alpha * (lp2 - lp3);

        sumSq += (int64_t)(lp3 * lp3);

        int32_t sample = (int32_t)(lp3 * 16.0f);

        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;

        audioBuffer_[i] = (int16_t)sample;
    }

    float rms = sqrt((float)sumSq / numSamples_);

    if (rms < 5)   return 0;
    if (rms < 15)  return 1;
    if (rms < 30)  return 2;
    if (rms < 60)  return 3;
    if (rms < 120) return 4;
    if (rms < 250) return 5;

    return 6;
}
bool AudioUdpStreamer::sendBlock() {
    if (!initialized_ || audioBuffer_ == nullptr) {
        return false;
    }

    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    udp_.beginPacket(remoteIp_, remotePort_);
    udp_.write(
        reinterpret_cast<const uint8_t*>(audioBuffer_),
        numSamples_ * sizeof(int16_t)
    );

    return udp_.endPacket() == 1;
}

uint8_t AudioUdpStreamer::routine() {
    uint8_t level = captureBlock();
    sendBlock();
    return level;
}