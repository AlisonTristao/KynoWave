// What TraceView's terminal can call (besides the library's own `btp`).
#include "app.h"
#include "board.h"

static uint8_t audio_stats() {
    const Audio::Stats s = audio.stats();
    btpdev::printf("rate=%lu actual=%lu blocks=%lu streamed=%lu overflows=%lu read_errors=%lu\n",
                   static_cast<unsigned long>(s.rate_hz), static_cast<unsigned long>(s.actual_rate_hz),
                   static_cast<unsigned long>(s.blocks), static_cast<unsigned long>(s.blocks_streamed),
                   static_cast<unsigned long>(s.overflows), static_cast<unsigned long>(s.read_errors));
    btpdev::printf("band=%u..%u Hz\n", static_cast<unsigned>(s.high_pass_hz), static_cast<unsigned>(s.low_pass_hz));
    btpdev::printf("rms=%.1f dbfs=%.1f peak=%u\n", static_cast<double>(audio.rms()),
                   static_cast<double>(audio.dbfs()), static_cast<unsigned>(audio.peak()));
    return RESULT_OK;
}

static uint8_t audio_rate(uint32_t hz) {
    if (!audio.set_rate(hz)) {
        btpdev::printf("rate must be %lu..%lu Hz\n", static_cast<unsigned long>(Audio::kMinRateHz),
                       static_cast<unsigned long>(Audio::kMaxRateHz));
        return RESULT_ERROR;
    }
    btpdev::printf("rate=%lu\n", static_cast<unsigned long>(hz));
    return RESULT_OK;
}

static uint8_t audio_band(uint16_t high_pass_hz, uint16_t low_pass_hz) {
    if (!audio.set_band(high_pass_hz, low_pass_hz)) {
        btpdev::printf("band must be high_pass < low_pass < rate/2 (0 turns a side off)\n");
        return RESULT_ERROR;
    }
    btpdev::printf("band=%u..%u Hz\n", static_cast<unsigned>(high_pass_hz), static_cast<unsigned>(low_pass_hz));
    return RESULT_OK;
}

static uint8_t leds_vu(uint8_t on) {
    leds.set_vu(on != 0U);
    btpdev::printf("vu=%u\n", on != 0U ? 1U : 0U);
    return RESULT_OK;
}

static uint8_t leds_step(uint16_t counts) {
    leds.set_step(counts);
    btpdev::printf("step=%u\n", static_cast<unsigned>(leds.step()));
    return RESULT_OK;
}

static uint8_t leds_set(uint8_t mask) {
    leds.set_mask(mask);
    btpdev::printf("mask=0x%02X (vu off)\n", static_cast<unsigned>(mask));
    return RESULT_OK;
}

static uint8_t display_backlight(uint8_t on) {
    display.set_backlight(on != 0U);
    btpdev::printf("backlight=%u\n", on != 0U ? 1U : 0U);
    return RESULT_OK;
}

void app_commands(btpdev::Commands& c) {
    c.module("audio", "Microphone")
        .add("stats", audio_stats, "audio -stats")
        .add("rate", audio_rate, "audio -rate <8000..83333 Hz>")
        .add("band", audio_band, "audio -band <high-pass Hz> <low-pass Hz>: band-pass, 0 = side off");
    if (board::kLedsEnabled) {
        c.module("leds", "Front LEDs")
            .add("vu", leds_vu, "leds -vu <0|1>: VU meter on/off")
            .add("step", leds_step, "leds -step <counts>: RMS per LED in the VU meter")
            .add("set", leds_set, "leds -set <mask>: fixed pattern, bit i = LED i");
    }
    c.module("display", "LCD")
        .add("backlight", display_backlight, "display -backlight <0|1>");
}
