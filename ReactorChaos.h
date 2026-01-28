#pragma once

#include <Arduino.h>

namespace ReactorChaos {

// Initialization
void begin();

// Main update - call during MODE_CHAOS
void tick();

// Reset state when entering chaos
void reset();

} // namespace ReactorChaos
