#pragma once

#include <Arduino.h>

namespace ReactorDark {

// Initialization
void begin();

// Main update - call during MODE_DARK
void tick();

// Initialize dark mode with success display
void enterDarkWithSuccess();

// Reset state when entering dark mode
void reset();

} // namespace ReactorDark
