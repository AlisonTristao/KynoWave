// What TraceView can plot.
#include "app.h"

void app_topics(btpdev::Topics& t) {
    // Every microphone sample, in blocks of one DMA frame (~12 ms at 83 kHz):
    // TraceView's Audio Spectrum widget plays it and shows its FFT. 16 blocks
    // (~200 ms, 33 KB -- in PSRAM) wait here when the link is busy a moment.
    t.stream(kAudioStream, Audio::kBlockSamples, "counts", 16)
        .range(-Audio::kFullScale, Audio::kFullScale - 1);

    // One sample per microphone block (20 per second at most).
    t.topic("audio.level", btpdev::Hz(20))
        .f32("rms", "counts")
        .f32("dbfs", "dBFS", Audio::kFloorDbfs, 0.0)
        .u16("peak", "counts").range(0, 2048)
        .fill([](btpdev::Sample& s) {
            s.put("rms", audio.rms()).put("dbfs", audio.dbfs()).put("peak", audio.peak());
        });
}
