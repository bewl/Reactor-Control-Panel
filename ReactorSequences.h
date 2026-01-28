#pragma once

#include <Arduino.h>
#include "ReactorTypes.h"

// Forward declaration for transition callbacks
namespace ReactorSystem {
  void finishStabilizingToStable();
  void finishFreezedownToStable();
  void enterStabilizing();
}

namespace ReactorSequences {

// Initialization
void begin();

// Main update - call once per tick, passes current mode for state tracking
void tick(Mode mode);

// Reset all sequence timers (call on mode transitions)
void reset();

// Query current step for a mode
uint8_t getStep(Mode mode);
uint8_t getTotalSteps(Mode mode);

// Get message for current step of a mode
const char* getStepMessage(Mode mode);

// Sequence completion helpers
void finishStabilizingToStable();
void finishFreezedownToStable();

// Drawing helpers
void drawArmingNumber(uint8_t n);
void drawStabilizingStep();
void drawStartupStep();
void drawFreezedownStep();
void drawShutdownStep();
void drawMeltdownCountdown();

} // namespace ReactorSequences
