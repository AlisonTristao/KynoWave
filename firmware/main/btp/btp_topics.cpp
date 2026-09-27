// What TraceView can plot.
#include "app.h"

void app_topics(btpdev::Topics& t) {
    // One sample per microphone block (20 per second at most).
    t.topic("audio.level", btpdev::Hz(20))
        .f32("rms", "counts")
        .f32("dbfs", "dBFS", Audio::kFloorDbfs, 0.0)
        .u16("peak", "counts").range(0, 2048)
        .fill([](btpdev::Sample& s) {
            s.put("rms", audio.rms()).put("dbfs", audio.dbfs()).put("peak", audio.peak());
        });
}
