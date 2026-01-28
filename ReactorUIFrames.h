#pragma once

#include <Arduino.h>
#include "ReactorTypes.h"
#include "ReactorAnimations.h"

namespace ReactorUIFrames {
  void drawCoreStatus(bool warning);
  void drawCoreStatusForce(bool warning);
  void drawCenteredBig(const char* txt, uint8_t size);
  void renderStableUIFrame();
  void renderActiveUIFrame(Mode mode, unsigned long meltdownStartAt);
}
