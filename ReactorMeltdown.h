#pragma once

#include <Arduino.h>

namespace ReactorMeltdown {

// Initialization
void begin();

// Main update - call during MODE_MELTDOWN
void tick();

// Reset countdown when entering meltdown
void reset();

} // namespace ReactorMeltdown
