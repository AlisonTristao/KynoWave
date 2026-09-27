// Who the device is and how it talks to TraceView.
#pragma once
#include <btpdev/config.h>

inline const btpdev::DeviceConfig device_config = {
    .name = "kynowave",
    // The native USB port: no key, no Wi-Fi. TCP and BLE come later
    // (docs/PLANO_BTPDEVICE.md, etapa 4).
    .link = btpdev::Link::Serial,
    .memory = btpdev::Memory::Small,
    .source_info = {{"board", "Board", "KynoWave prototype"},
                    {"microphone", "Microphone", "MAX4466 (ADC)"}},
};
