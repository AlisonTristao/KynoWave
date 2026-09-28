// Who the device is and how it talks to TraceView.
#pragma once
#include <btpdev/config.h>

inline const btpdev::DeviceConfig device_config = {
    .name = "kynowave",
    // The link saved with `btp -link_set <serial|tcp>` (applies at the next
    // boot); TCP while nothing is saved. TCP needs bally.key and wifi.txt in
    // data/ (pio run -t uploadfs); without them the library falls back to the
    // native USB port on its own.
    .link = btpdev::Link::FromSettings,
    .fallback = btpdev::Link::Tcp,
    // Medium: a 16 KB send queue, room for several 2 KB audio blocks.
    .memory = btpdev::Memory::Medium,
    .source_info = {{"board", "Board", "KynoWave prototype"},
                    {"microphone", "Microphone", "MAX4466 (ADC)"}},
};
