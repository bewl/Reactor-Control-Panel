#pragma once

#include <Arduino.h>

// Shared reactor modes used across modules
enum Mode : uint8_t {
  MODE_STABLE,
  MODE_ARMING,
  MODE_CRITICAL,
  MODE_MELTDOWN,
  MODE_STABILIZING,
  MODE_STARTUP,
  MODE_FREEZEDOWN,
  MODE_SHUTDOWN,
  MODE_DARK,
  MODE_CHAOS
};
