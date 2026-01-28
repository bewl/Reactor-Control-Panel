#pragma once

#include <Arduino.h>

namespace ReactorAudio {

void begin(uint8_t buzzerPin);
void toneHz(unsigned int hz);
void off();
void muteFor(unsigned long ms);
bool isMuted();
void tickMute();

} // namespace ReactorAudio
