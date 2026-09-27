// The firmware's modules, shared by main.cpp and the BTPDevice files in btp/.
#pragma once

#include <BtpDevice.h>

#include "audio/Audio.h"
#include "display/Display.h"
#include "leds/Leds.h"

/// The stream topic with the microphone's samples (btp_topics.cpp).
inline constexpr const char* kAudioStream = "audio.pcm";

extern Audio audio;
extern Leds leds;
extern Display display;
extern BtpDevice device;
