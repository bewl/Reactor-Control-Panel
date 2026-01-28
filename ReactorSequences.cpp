#include "ReactorSequences.h"
#include "ReactorTypes.h"
#include "ReactorAudio.h"
#include "ReactorUI.h"
#include "ReactorHeat.h"
#include <math.h>

namespace ReactorSequences {

// Forward declarations of internal tick functions
void tickArming(unsigned long now);
void tickStabilizing(unsigned long now);
void tickFreezedown(unsigned long now);
void tickStartup(unsigned long now);
void tickShutdown(unsigned long now);

// Forward declarations of internal drawing functions
void drawArmingNumber(uint8_t n);
void drawStabilizingStep();
void drawFreezedownStep();
void drawStartupStep();
void drawShutdownStep();

// ======================= Timing Constants =======================
// Arming (3-2-1)
const unsigned long ARM_STEP_MS   = 500;
const uint8_t       ARM_BLINKS    = 5;
const unsigned int  ARM_CHIRP_HZ  = 1600;

// Freezedown Sequence
const unsigned long FREEZE_STEP_MS        = 1200;
const uint8_t       FREEZE_TOTAL_STEPS    = 5;
const unsigned long FREEZE_LED_PERIOD_MS  = 1200;

// Freezedown alarm (slow, cooling "wah-wah")
const unsigned long FREEZE_ALARM_PERIOD_MS = 500;
const int           FREEZE_ALARM_HIGH_HZ   = 650;
const int           FREEZE_ALARM_LOW_HZ    = 350;

// Stabilization sequence
const unsigned long STAB_STEP_MS       = 1000;
const uint8_t       STAB_TOTAL_STEPS   = 5;
const unsigned long STAB_LED_PERIOD_MS = 1000;

// Stabilization alarm sound (alternating siren)
const unsigned long STAB_ALARM_PERIOD_MS = 400;
const int           STAB_ALARM_LOW_HZ    = 800;
const int           STAB_ALARM_HIGH_HZ   = 1000;

// Startup sequence (multi-step + rising pitch, then auto -> Stabilizing)
const unsigned long STARTUP_STEP_MS       = 2000;
const uint8_t       STARTUP_TOTAL_STEPS   = 5;
const unsigned long STARTUP_LED_PERIOD_MS = 400;
const int           STARTUP_F0_HZ         = 300;
const int           STARTUP_F1_HZ         = 1600;

// Shutdown sequence (multi-step + falling pitch, then auto -> Stable)
const unsigned long SHUTDOWN_STEP_MS       = 2000;
const uint8_t       SHUTDOWN_TOTAL_STEPS   = 5;
const unsigned long SHUTDOWN_LED_PERIOD_MS = 800;
const int           SHUTDOWN_F0_HZ         = 1400;
const int           SHUTDOWN_F1_HZ         = 200;

// ======================= Message Arrays =======================
const char* const STAB_MSGS[STAB_TOTAL_STEPS] = {
  "Inserting control rods",
  "Coolant flow increasing",
  "Pressure equalizing",
  "Containment securing",
  "Calibrating sensors"
};

const char* const STARTUP_MSGS[STARTUP_TOTAL_STEPS] = {
  "Evacuate chamber",
  "Seal access hatches",
  "Charge pre-heaters",
  "Spin aux pumps",
  "Diagnostics ready"
};

const char* const FREEZE_MSGS[FREEZE_TOTAL_STEPS] = {
  "Cryo coolant engaged",
  "Thermal siphons active",
  "Lattice contraction",
  "Containment frost check",
  "Core hibernation"
};

const char* const SHUTDOWN_MSGS[SHUTDOWN_TOTAL_STEPS] = {
  "Divert plasma flow",
  "Drain coolant system",
  "Retract control rods",
  "Vent reactor chamber",
  "Power systems offline"
};

// ======================= State Variables =======================
// Arming state
uint8_t       armStep = 0;
unsigned long armTickAt = 0;

// Stabilizing state
uint8_t       stabStep = 0;
unsigned long stabStepAt = 0;
unsigned long stabLedAt  = 0;
bool          stabLedOn  = false;
int           lastShownStabStep = -1;

// Stabilization alarm state
unsigned long stabAlarmAt = 0;
bool          stabAlarmHigh = false;

// Freezedown state
uint8_t       freezeStep    = 0;
unsigned long freezeStepAt  = 0;
unsigned long freezeLedAt   = 0;
bool          freezeLedOn   = false;
int           lastShownFreezeStep = -1;

// Freezedown alarm state
unsigned long freezeAlarmAt = 0;
bool          freezeAlarmHigh = false;

// Startup state
uint8_t       startupStep     = 0;
unsigned long startupStepAt   = 0;
unsigned long startupStart    = 0;
unsigned long startupBlinkAt  = 0;
bool          startupLedOn    = false;
int           lastShownStartupStep = -1;

// Shutdown state
uint8_t       shutdownStep     = 0;
unsigned long shutdownStepAt   = 0;
unsigned long shutdownStart    = 0;
int           lastShownShutdownStep = -1;

// Pin configuration (from ReactorSystem)
const uint8_t PIN_LED_MELTDOWN      = 13;
const uint8_t PIN_LED_STABLE        = 12;
const uint8_t PIN_LED_STARTUP       = 11;
const uint8_t PIN_LED_FREEZEDOWN    = 9;

// ======================= Helpers =======================
inline bool isMuted() { return ReactorAudio::isMuted(); }
inline void buzzerOff() { ReactorAudio::off(); }
inline void buzzerTone(unsigned int hz) { ReactorAudio::toneHz(hz); }

float currentHeatPercent() {
  uint8_t level = ReactorHeat::getLevel();
  return (float)level / 12.0f * 100.0f;
}

// ======================= API =======================
void begin() {
  pinMode(PIN_LED_MELTDOWN,     OUTPUT);
  pinMode(PIN_LED_STABLE,       OUTPUT);
  pinMode(PIN_LED_STARTUP,      OUTPUT);
  pinMode(PIN_LED_FREEZEDOWN,   OUTPUT);
  reset();
}

void reset() {
  armStep = 0;
  armTickAt = 0;
  
  stabStep = 0;
  stabStepAt = 0;
  stabLedAt = 0;
  stabLedOn = false;
  lastShownStabStep = -1;
  stabAlarmAt = 0;
  stabAlarmHigh = false;
  
  freezeStep = 0;
  freezeStepAt = 0;
  freezeLedAt = 0;
  freezeLedOn = false;
  lastShownFreezeStep = -1;
  freezeAlarmAt = 0;
  freezeAlarmHigh = false;
  
  startupStep = 0;
  startupStepAt = millis();
  startupStart = startupStepAt;
  startupBlinkAt = startupStepAt;
  startupLedOn = false;
  lastShownStartupStep = -1;
  
  shutdownStep = 0;
  shutdownStepAt = millis();
  shutdownStart = shutdownStepAt;
  lastShownShutdownStep = -1;
}

uint8_t getStep(Mode mode) {
  switch (mode) {
    case MODE_ARMING: return armStep;
    case MODE_STABILIZING: return stabStep;
    case MODE_FREEZEDOWN: return freezeStep;
    case MODE_STARTUP: return startupStep;
    case MODE_SHUTDOWN: return shutdownStep;
    default: return 0;
  }
}

uint8_t getTotalSteps(Mode mode) {
  switch (mode) {
    case MODE_ARMING: return ARM_BLINKS * 2;
    case MODE_STABILIZING: return STAB_TOTAL_STEPS;
    case MODE_FREEZEDOWN: return FREEZE_TOTAL_STEPS;
    case MODE_STARTUP: return STARTUP_TOTAL_STEPS;
    case MODE_SHUTDOWN: return SHUTDOWN_TOTAL_STEPS;
    default: return 0;
  }
}

const char* getStepMessage(Mode mode) {
  uint8_t step = getStep(mode);
  if (step >= getTotalSteps(mode)) step = getTotalSteps(mode) - 1;
  
  switch (mode) {
    case MODE_STABILIZING: return STAB_MSGS[step];
    case MODE_STARTUP: return STARTUP_MSGS[step];
    case MODE_FREEZEDOWN: return FREEZE_MSGS[step];
    case MODE_SHUTDOWN: return SHUTDOWN_MSGS[step];
    default: return "";
  }
}

void tick(Mode mode) {
  unsigned long now = millis();
  
  switch (mode) {
    case MODE_ARMING:
      tickArming(now);
      break;
    case MODE_STABILIZING:
      tickStabilizing(now);
      break;
    case MODE_FREEZEDOWN:
      tickFreezedown(now);
      break;
    case MODE_STARTUP:
      tickStartup(now);
      break;
    case MODE_SHUTDOWN:
      tickShutdown(now);
      break;
    default:
      break;
  }
}

void finishStabilizingToStable() {
  // This will be called from ReactorSystem to transition
  // The actual state management stays in ReactorSystem
}

void finishFreezedownToStable() {
  // This will be called from ReactorSystem to transition
}

} // namespace ReactorSequences

// ======================= Internal Tick Functions (file scope) =======================
void ReactorSequences::tickArming(unsigned long now) {
  if (now - armTickAt < ARM_STEP_MS) return;
  armTickAt = now;

  ++armStep;
  bool onPhase = (armStep % 2) == 1;
  digitalWrite(PIN_LED_MELTDOWN, onPhase ? HIGH : LOW);
  if (onPhase) buzzerTone(ARM_CHIRP_HZ);
  else         buzzerOff();

  uint8_t totalToggles = ARM_BLINKS * 2;
  if (armStep <= totalToggles) {
    uint8_t num = ARM_BLINKS - ((armStep - 1) / 2);
    if (num >= 1) drawArmingNumber(num);
  }
}

void ReactorSequences::tickStabilizing(unsigned long now) {
  // Blink stable LED with a 1s period (toggle every 500ms)
  if (now - stabLedAt >= (STAB_LED_PERIOD_MS / 2)) {
    stabLedAt = now;
    stabLedOn = !stabLedOn;
    digitalWrite(PIN_LED_STABLE, stabLedOn ? HIGH : LOW);
  }

  // Alternate the alarm tone while stabilizing (urgent "waah-waah")
  if (now - stabAlarmAt >= STAB_ALARM_PERIOD_MS) {
    stabAlarmAt = now;
    stabAlarmHigh = !stabAlarmHigh;
    buzzerTone(stabAlarmHigh ? STAB_ALARM_HIGH_HZ : STAB_ALARM_LOW_HZ);
  }

  // Advance progress steps
  if (now - stabStepAt >= STAB_STEP_MS) {
    stabStepAt = now;
    if (stabStep < STAB_TOTAL_STEPS - 1) {
      ++stabStep;
      drawStabilizingStep();
    }
    // Completion check handled by ReactorSystem via stabilization timer
  }
}

void ReactorSequences::tickStartup(unsigned long now) {
  unsigned long elapsedSeq = now - startupStart;
  unsigned long totalStartupMs = (unsigned long)STARTUP_TOTAL_STEPS * STARTUP_STEP_MS;
  
  if (elapsedSeq < totalStartupMs) {
    float t = (float)elapsedSeq / (float)totalStartupMs;
    float ratio = (float)STARTUP_F1_HZ / (float)STARTUP_F0_HZ;
    float f = (float)STARTUP_F0_HZ * powf(ratio, t);
    buzzerTone((unsigned int)f);
  } else {
    buzzerOff();
  }

  if (now - startupBlinkAt >= STARTUP_LED_PERIOD_MS) {
    startupBlinkAt = now;
    startupLedOn = !startupLedOn;
    digitalWrite(PIN_LED_STARTUP, startupLedOn ? HIGH : LOW);
  }

  if (now - startupStepAt >= STARTUP_STEP_MS) {
    startupStepAt = now;
    if (startupStep < STARTUP_TOTAL_STEPS - 1) {
      ++startupStep;
      drawStartupStep();
    }
  }
}

void ReactorSequences::tickFreezedown(unsigned long now) {
  // Pulse the FREEZEDOWN LED slowly
  if (now - freezeLedAt >= (FREEZE_LED_PERIOD_MS / 2)) {
    freezeLedAt = now;
    freezeLedOn = !freezeLedOn;
    digitalWrite(PIN_LED_FREEZEDOWN, freezeLedOn ? HIGH : LOW);
  }

  // Alternate a low cooling alarm
  if (now - freezeAlarmAt >= FREEZE_ALARM_PERIOD_MS) {
    freezeAlarmAt = now;
    freezeAlarmHigh = !freezeAlarmHigh;
    buzzerTone(freezeAlarmHigh ? FREEZE_ALARM_HIGH_HZ : FREEZE_ALARM_LOW_HZ);
  }

  // Advance progress steps
  if (now - freezeStepAt >= FREEZE_STEP_MS) {
    freezeStepAt = now;
    if (freezeStep < FREEZE_TOTAL_STEPS - 1) {
      ++freezeStep;
      drawFreezedownStep();
    }
  }
}

void ReactorSequences::tickShutdown(unsigned long now) {
  unsigned long elapsedSeq = now - shutdownStart;
  unsigned long totalShutdownMs = (unsigned long)SHUTDOWN_TOTAL_STEPS * SHUTDOWN_STEP_MS;
  
  // Play falling pitch sweep during shutdown
  if (elapsedSeq < totalShutdownMs) {
    float t = (float)elapsedSeq / (float)totalShutdownMs;
    float ratio = (float)SHUTDOWN_F1_HZ / (float)SHUTDOWN_F0_HZ;
    float f = (float)SHUTDOWN_F0_HZ * powf(ratio, t);
    buzzerTone((unsigned int)f);
  } else {
    buzzerOff();
  }

  // Advance progress steps
  if (now - shutdownStepAt >= SHUTDOWN_STEP_MS) {
    shutdownStepAt = now;
    if (shutdownStep < SHUTDOWN_TOTAL_STEPS - 1) {
      ++shutdownStep;
      drawShutdownStep();
    }
  }
}

// ======================= Drawing Functions =======================
void ReactorSequences::drawArmingNumber(uint8_t n) {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.warning = false;
  m.progress = n;
  ReactorUI::ui.render(MODE_ARMING, m, isMuted());
}

void ReactorSequences::drawStabilizingStep() {
  if (stabStep == lastShownStabStep) return;
  lastShownStabStep = stabStep;
  uint8_t step = stabStep;
  if (step >= STAB_TOTAL_STEPS) step = STAB_TOTAL_STEPS - 1;

  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / STAB_TOTAL_STEPS);
  ReactorUI::ui.render(MODE_STABILIZING, m, isMuted());
}

void ReactorSequences::drawFreezedownStep() {
  if (freezeStep == lastShownFreezeStep) return;
  lastShownFreezeStep = freezeStep;
  uint8_t step = freezeStep;
  if (step >= FREEZE_TOTAL_STEPS) step = FREEZE_TOTAL_STEPS - 1;

  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / FREEZE_TOTAL_STEPS);
  m.freezing = true;
  ReactorUI::ui.render(MODE_FREEZEDOWN, m, isMuted());
}

void ReactorSequences::drawStartupStep() {
  if (startupStep == lastShownStartupStep) return;
  lastShownStartupStep = startupStep;
  uint8_t step = startupStep;
  if (step >= STARTUP_TOTAL_STEPS) step = STARTUP_TOTAL_STEPS - 1;

  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / STARTUP_TOTAL_STEPS);
  ReactorUI::ui.render(MODE_STARTUP, m, isMuted());
}

void ReactorSequences::drawShutdownStep() {
  if (shutdownStep == lastShownShutdownStep) return;
  lastShownShutdownStep = shutdownStep;
  uint8_t step = shutdownStep;
  if (step >= SHUTDOWN_TOTAL_STEPS) step = SHUTDOWN_TOTAL_STEPS - 1;

  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / SHUTDOWN_TOTAL_STEPS);
  ReactorUI::ui.render(MODE_SHUTDOWN, m, isMuted());
}

void ReactorSequences::drawMeltdownCountdown() {
  // Called from ReactorSystem during meltdown
  // This will need to be updated in ReactorSystem
}

} // namespace ReactorSequences
