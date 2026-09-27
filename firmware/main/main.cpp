// KynoWave -- the microphone to TraceView through BTPDevice: the live audio
// (stream "audio.pcm") and the sound level ("audio.level").
//
//   1. pio run -t upload, plug the board's native USB port into the PC.
//   2. In TraceView, add "kynowave" (Serial, automatic).
//   3. Add an Audio Spectrum widget on "audio.pcm" (plays it and shows the
//      FFT), plot "audio.level"; terminal: `audio -stats`, `btp -topics`.

#include <cstdio>

#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app.h"
#include "board.h"
#include "btp/btp_device.h"
#include "btp/btp_storage.h"

void app_commands(btpdev::Commands& c);
void app_topics(btpdev::Topics& t);

Audio audio;
Leds leds;
Display display;
BtpDevice device(device_config, storage_config);

namespace {

constexpr const char* kTag = "kynowave";
constexpr int kAppCore = 1;   // core 0 is BTPDevice's and the radio's

// The audio task's blocks go straight into the library's stream queue.
bool stream_block(const int16_t* samples, size_t count, uint32_t rate_hz, uint64_t timestamp_us) {
    return device.write_stream(kAudioStream, samples, count, rate_hz, timestamp_us);
}

const char* link_label(btpdev::Link link) {
    switch (link) {
        case btpdev::Link::Tcp: return "TCP";
        case btpdev::Link::Ble: return "BLE";
        case btpdev::Link::EspNow: return "ESP-NOW";
        default: return "USB";
    }
}

Display::Session session_of(btpdev::State state) {
    switch (state) {
        case btpdev::State::Connected: return Display::Session::Open;
        case btpdev::State::Starting:
        case btpdev::State::Ready: return Display::Session::Waiting;
        case btpdev::State::Failed: return Display::Session::Failed;
        default: return Display::Session::Off;
    }
}

int8_t wifi_bars() {
    wifi_ap_record_t ap = {};
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return 0;
    if (ap.rssi >= -55) return 4;
    if (ap.rssi >= -65) return 3;
    if (ap.rssi >= -75) return 2;
    return 1;
}

// The status bar follows BTPDevice: link, session and (on TCP) Wi-Fi bars.
void show_link(btpdev::State state) {
    Display::Status status;
    const btpdev::Link link = device.active_link();
    std::snprintf(status.link, sizeof(status.link), "%s", link_label(link));
    status.session = session_of(state);
    status.wifi_bars = link == btpdev::Link::Tcp ? wifi_bars() : -1;
    display.set_status(status);
}

}  // namespace

extern "C" void app_main() {
    if (leds.begin() != ESP_OK) ESP_LOGW(kTag, "no LEDs");
    if (display.begin(kAppCore) != ESP_OK) ESP_LOGW(kTag, "no display");
    display.log("KynoWave");
    if (audio.begin(board::kMic, kAppCore, stream_block) != ESP_OK) display.log("microfone: erro no ADC");

    if (device.begin(app_commands, app_topics) != ESP_OK) {
        // The cause is in the log and in last_error(); the level still shows here.
        display.log(device.last_error().message);
    } else {
        char line[48];
        std::snprintf(line, sizeof(line), "BTP: %s, source %08lx", link_label(device.active_link()),
                      static_cast<unsigned long>(device.source_id()));
        display.log(line);
    }

    btpdev::State shown = btpdev::State::Stopped;
    for (uint32_t tick = 0;; ++tick) {
        leds.show_level(audio.rms());
        display.set_level(audio.dbfs());

        const btpdev::State state = device.state();
        if (state != shown || tick % 20U == 0U) show_link(state);   // Wi-Fi bars once a second
        if (state != shown) {
            if (state == btpdev::State::Connected) {
                display.log("TraceView conectado");
                btpdev::log(btpdev::LogLevel::Info, "KynoWave pronto, microfone a %lu Hz",
                            static_cast<unsigned long>(audio.stats().rate_hz));
            } else if (shown == btpdev::State::Connected) {
                display.log("TraceView desconectado");
            }
            shown = state;
        }
        vTaskDelay(pdMS_TO_TICKS(Audio::kPeriodMs));
    }
}
