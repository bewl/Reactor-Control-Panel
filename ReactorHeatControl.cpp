#include "ReactorHeatControl.h"
#include "ReactorHeat.h"
#include "ReactorSequences.h"

namespace ReactorHeatControl {

static void updateTargetForMode(Mode mode) {
  float heatTarget = ReactorHeat::getLevel();
  switch (mode) {
    case MODE_STABLE:
      heatTarget = 4.0f; // safe-ish idle
      break;

    case MODE_ARMING: {
      uint8_t step = ReactorSequences::getStep(MODE_ARMING);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_ARMING);
      float phase = (total > 0) ? (float)step / (float)total : 0.0f;
      if (phase < 0) phase = 0; if (phase > 1) phase = 1;
      heatTarget = 6.0f + 4.0f * phase; // 6..10
      } break;

    case MODE_STARTUP: {
      uint8_t step = ReactorSequences::getStep(MODE_STARTUP);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_STARTUP);
      float t = (total > 0) ? (float)step / (float)total : 0.0f;
      if (t < 0) t = 0; if (t > 1) t = 1;
      heatTarget = 3.0f + 6.0f * t; // 3..9 over startup
      } break;

    case MODE_STABILIZING: {
      uint8_t step = ReactorSequences::getStep(MODE_STABILIZING);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_STABILIZING);
      float t = (total > 1) ? (float)step / (float)(total - 1) : 0.0f;
      heatTarget = 9.0f - (9.0f - 3.0f) * t;
      } break;

    case MODE_FREEZEDOWN:
      heatTarget = 1.0f; // near freezing
      break;

    case MODE_SHUTDOWN: {
      // Gradually decrease heat during shutdown
      uint8_t step = ReactorSequences::getStep(MODE_SHUTDOWN);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_SHUTDOWN);
      float t = (total > 0) ? (float)step / (float)total : 0.0f;
      if (t < 0) t = 0; if (t > 1) t = 1;
      heatTarget = 6.0f - 3.0f * t; // 6..3 over shutdown
      } break;

    case MODE_MELTDOWN:
      heatTarget = 11.5f; // drive near max during meltdown
      break;

    default:
      break;
  }

  ReactorHeat::setTarget(heatTarget);
}

void tick(Mode mode) {
  updateTargetForMode(mode);
  ReactorHeat::tick(mode);
}

} // namespace ReactorHeatControl
