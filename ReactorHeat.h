#pragma once

#include <Arduino.h>
#include "ReactorTypes.h"

namespace ReactorHeat {

void begin();
void setTarget(float level);       // desired level 0..12
void setLevel(float level);        // force level instantly
float getLevel();                  // current level (0..12)
uint8_t percent();                 // 0..100 for UI
void tick(Mode mode);              // apply slew + special blinks
void allOff();
void chaosFlicker();

} // namespace ReactorHeat
