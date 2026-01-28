#pragma once

#include "ReactorTypes.h"

namespace ReactorStateMachine {
  // State variables (accessible for timing checks in ReactorSystem)
  extern unsigned long stabStartAt;
  extern unsigned long freezeStartAt;
  extern unsigned long startupStartAt;
  extern unsigned long shutdownStartAt;
  extern unsigned long meltdownStartAt;

  // Get current mode
  Mode getMode();

  // State transitions
  void enterStable();
  void enterArming();
  void enterMeltdown();
  void enterStabilizing();
  void enterStartup();
  void enterFreezedown();
  void enterShutdown();
  void enterDark();
  void enterChaos();

  // Transition helpers
  void abortStabilizingToMeltdown();
  void finishStabilizingToStable();
  void finishFreezedownToStable();
}
