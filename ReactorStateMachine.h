#pragma once

#include "ReactorTypes.h"

namespace ReactorStateMachine {
  // State variables (accessible for timing checks in ReactorSystem)
  extern unsigned long armingStartAt;      // Arming countdown timer (5 seconds)
  extern unsigned long criticalStartAt;    // Critical warning timer (3 seconds)
  extern unsigned long stabStartAt;
  extern unsigned long freezeStartAt;
  extern unsigned long startupStartAt;
  extern unsigned long shutdownStartAt;
  extern unsigned long meltdownStartAt;    // Meltdown countdown timer (10 seconds)

  // Get current mode
  Mode getMode();

  // State transitions
  void enterStable();
  void enterArming();
  void enterCritical();
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
