#include "ReactorSystem.h"
#include "ReactorTypes.h"
#include "ReactorUI.h"
#include "ReactorButtons.h"
#include "ReactorAudio.h"
#include "ReactorHeat.h"
#include "ReactorHeatControl.h"
#include "ReactorEvents.h"
#include "ReactorSecrets.h"
#include "ReactorSequences.h"
#include "ReactorMeltdown.h"
#include "ReactorChaos.h"
#include "ReactorDark.h"
#include "ReactorSweep.h"
#include "ReactorUIFrames.h"
#include "ReactorStateMachine.h"

#include <Wire.h>
#include <math.h>

namespace ReactorSystem {

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// Local shims to the audio module for concise calls
inline bool isMuted() { return ReactorAudio::isMuted(); }
inline void buzzerOff() { ReactorAudio::off(); }
inline void buzzerTone(unsigned int hz) { ReactorAudio::toneHz(hz); }

// ======================= Pins =======================
const uint8_t PIN_LED_MELTDOWN      = 13;
const uint8_t PIN_LED_STABLE        = 12;
const uint8_t PIN_LED_STARTUP       = 11;
const uint8_t PIN_LED_FREEZEDOWN    = 9;
const uint8_t PIN_BUZZER            = 7;

// ======================= Tunables =======================
// Arming (3-2-1)
const unsigned long ARM_STEP_MS   = 500;
const uint8_t       ARM_BLINKS    = 5;
const unsigned int  ARM_CHIRP_HZ  = 1600;

// Meltdown constants moved to ReactorMeltdown

// Dark mode (post-shutdown)
const unsigned long DARK_SUCCESS_DISPLAY_MS = 2000; // Show success for 2 seconds

// Event alarm sound (urgent alternating tone)
const unsigned long EVENT_ALARM_PERIOD_MS = 300;
const int           EVENT_ALARM_LOW_HZ    = 900;
const int           EVENT_ALARM_HIGH_HZ   = 1400;

// Stable "breathing" animation
const uint16_t STABLE_BREATH_MS   = 2600; // full inhale+exhale period
const uint8_t  STABLE_BREATH_AMPL = 4;    // +/- percent swing (keep small)
const uint16_t UI_FRAME_MS = 100;   // ~10 FPS
unsigned long  uiFrameAt   = 0;

// Timed mute window
const unsigned long ACK_SILENCE_MS = 8000; // 8 seconds

// ======================= State Machine =======================
// Delegated to ReactorStateMachine namespace


// ======================= Setup =======================
void begin() {
  Wire.setClock(400000);

  pinMode(PIN_LED_MELTDOWN,     OUTPUT);
  pinMode(PIN_LED_STABLE,       OUTPUT);
  pinMode(PIN_LED_STARTUP,      OUTPUT);
  pinMode(PIN_LED_FREEZEDOWN,   OUTPUT);
  ReactorAudio::begin(PIN_BUZZER);
  ReactorButtons::begin();
  ReactorHeat::begin();
  ReactorEvents::begin();
  ReactorSecrets::begin();
  ReactorSequences::begin();
  ReactorMeltdown::begin();
  ReactorChaos::begin();
  ReactorDark::begin();

  // Seed chaos effects
  randomSeed(analogRead(A0));

  if (!ReactorUI::begin()) {
    while (true) { /* halt if OLED missing */ }
  }

  // Ensure quiet baseline
  ReactorAudio::off();

  // Prevent first-frame/time-step jumps
  unsigned long now = millis();
  uiFrameAt  = now;

  ReactorStateMachine::enterStable();
}

// ======================= Main Loop =======================
void tick() {
  // Update debounce state
  ReactorButtons::update();

  // Read edges ONCE per loop
  bool overrideFell    = ReactorButtons::overrideBtn.fell();
  bool stabilizeFell   = ReactorButtons::stabilizeBtn.fell();
  bool startupFell     = ReactorButtons::startupBtn.fell();
  bool freezedownFell  = ReactorButtons::freezedownBtn.fell();
  bool shutdownFell    = ReactorButtons::shutdownBtn.fell();
  bool eventFell       = ReactorButtons::eventBtn.fell();
  bool ackFell         = ReactorButtons::ackBtn.fell();

  // ---- Secret sequence capture ----
  char code = 0;
  if (overrideFell)   code = 'O';
  if (stabilizeFell)  code = 'S';
  if (startupFell)    code = 'U';
  if (freezedownFell) code = 'F';
  if (shutdownFell)   code = 'D';
  if (eventFell)      code = 'E';
  if (code) {
    ReactorSecrets::captureInput(code);
  }

  // ---- Event resolution first ----
  if (ReactorEvents::handleInput(overrideFell, stabilizeFell, startupFell,
                                 freezedownFell, shutdownFell, eventFell)) {
    // Don't process normal button actions when resolving event
    return;
  }

  // ---- Heat emergency check ----
  // If stabilizing and heat reaches critical, trigger meltdown automatically
  if (ReactorStateMachine::getMode() == MODE_STABILIZING && ReactorHeat::getLevel() >= 11.5f) {
    ReactorStateMachine::abortStabilizingToMeltdown();
  }

  // ---- Button -> Mode transitions ----
  if (overrideFell) {
    if (ReactorStateMachine::getMode() == MODE_STABLE)   ReactorStateMachine::enterArming();
    else if (ReactorStateMachine::getMode() == MODE_STABILIZING) ReactorStateMachine::abortStabilizingToMeltdown();
    else if (ReactorStateMachine::getMode() == MODE_STARTUP)     ReactorStateMachine::enterArming();
  }

  if (stabilizeFell) {
    if (ReactorStateMachine::getMode() == MODE_STABLE)        ReactorStateMachine::enterFreezedown(); // shortcut
    else if (ReactorStateMachine::getMode() == MODE_ARMING)   ReactorStateMachine::enterStabilizing();
    else if (ReactorStateMachine::getMode() == MODE_MELTDOWN) ReactorStateMachine::enterStabilizing();
    // In CHAOS, stabilize is ignored (only Startup can recover)
  }

  if (startupFell) {
    if (ReactorStateMachine::getMode() == MODE_STABLE)  ReactorStateMachine::enterStartup();
    else if (ReactorStateMachine::getMode() == MODE_DARK) ReactorStateMachine::enterStartup(); // wake from dark
    else if (ReactorStateMachine::getMode() == MODE_CHAOS) ReactorStateMachine::enterStartup(); // reboot from chaos
  }

  if (freezedownFell && (ReactorStateMachine::getMode() == MODE_STABLE || ReactorStateMachine::getMode() == MODE_MELTDOWN)) {
    ReactorStateMachine::enterFreezedown();
  }

  if (shutdownFell && (ReactorStateMachine::getMode() == MODE_STABLE || ReactorStateMachine::getMode() == MODE_CHAOS)) {
    ReactorStateMachine::enterShutdown();
  }

  // Event button triggers random event in stable mode
  if (eventFell && ReactorStateMachine::getMode() == MODE_STABLE && !ReactorEvents::isActive()) {
    ReactorEvents::trigger();
  }

  // If ACK pressed: start/extend mute and silence immediately
  if (ackFell) {
    ReactorAudio::muteFor(ACK_SILENCE_MS);
  }

  // ---- Tick current mode ----
  switch (ReactorStateMachine::getMode()) {
    case MODE_ARMING:       break;
    case MODE_MELTDOWN:     ReactorMeltdown::tick(); break;
    case MODE_STABILIZING:  break;
    case MODE_STARTUP:      break;
    case MODE_FREEZEDOWN:   break;
    case MODE_SHUTDOWN:     break;
    case MODE_DARK:         ReactorDark::tick(); break;
    case MODE_CHAOS:        ReactorChaos::tick(); break;
    case MODE_STABLE:       break;
  }

  // ---- Sequence timing and alarms ----
  ReactorSequences::tick(ReactorStateMachine::getMode());

  // ---- Check for sequence completions ----
  unsigned long now = millis();
  
  // Stabilizing completes after STAB_TOTAL_STEPS steps
  if (ReactorStateMachine::getMode() == MODE_STABILIZING) {
    unsigned long elapsed = now - ReactorStateMachine::stabStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_STABILIZING)) * 1000;
    if (elapsed >= totalMs) {
      buzzerOff();
      ReactorStateMachine::finishStabilizingToStable();
    }
  }
  
  // Startup transitions to Stabilizing after STARTUP_TOTAL_STEPS steps
  if (ReactorStateMachine::getMode() == MODE_STARTUP) {
    unsigned long elapsed = now - ReactorStateMachine::startupStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_STARTUP)) * 2000;
    if (elapsed >= totalMs) {
      buzzerOff();
      digitalWrite(PIN_LED_STARTUP, LOW);
      ReactorStateMachine::enterStabilizing();
    }
  }
  
  // Freezedown completes after FREEZE_TOTAL_STEPS steps
  if (ReactorStateMachine::getMode() == MODE_FREEZEDOWN) {
    unsigned long elapsed = now - ReactorStateMachine::freezeStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_FREEZEDOWN)) * 1200;
    if (elapsed >= totalMs) {
      buzzerOff();
      ReactorStateMachine::finishFreezedownToStable();
    }
  }
  
  // Shutdown transitions to Dark after SHUTDOWN_TOTAL_STEPS steps
  if (ReactorStateMachine::getMode() == MODE_SHUTDOWN) {
    unsigned long elapsed = now - ReactorStateMachine::shutdownStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_SHUTDOWN)) * 2000;
    if (elapsed >= totalMs) {
      buzzerOff();
      ReactorStateMachine::enterDark();
    }
  }

  // ---- Event and secrets tick ----
  ReactorEvents::tick();
  ReactorSecrets::tick();

  if (ReactorStateMachine::getMode() != MODE_CHAOS && ReactorStateMachine::getMode() != MODE_DARK && (now - uiFrameAt) >= UI_FRAME_MS) {
    uiFrameAt = now;
    ReactorUIFrames::renderActiveUIFrame(ReactorStateMachine::getMode(), ReactorStateMachine::meltdownStartAt);  // repaints current screen (incl. progress bars)
  }

  // Heat bar (skip during CHAOS and DARK)
  if (ReactorStateMachine::getMode() != MODE_CHAOS && ReactorStateMachine::getMode() != MODE_DARK) {
    ReactorHeatControl::tick(ReactorStateMachine::getMode());
  }

  ReactorSweep::tick();

  // Enforce buzzer mute if active (prevents any stray tone)
  ReactorAudio::tickMute();
}

} // namespace ReactorSystem



