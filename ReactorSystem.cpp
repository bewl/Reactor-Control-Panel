#include "ReactorSystem.h"
#include "ReactorTypes.h"
#include "ReactorUI.h"
#include "ReactorButtons.h"
#include "ReactorAudio.h"
#include "ReactorHeat.h"
#include "ReactorEvents.h"
#include "ReactorSecrets.h"
#include "ReactorSequences.h"
#include "ReactorMeltdown.h"
#include "ReactorChaos.h"
#include "ReactorDark.h"

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
// Shutdown sweep (used by transitions)
const unsigned long SWEEP_MS     = 1000;
const int           SWEEP_F0_HZ  = 1800;
const int           SWEEP_F1_HZ  = 140;

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

// STABLE UI refresh
const uint16_t STABLE_UI_FRAME_MS = 100;  // ~10 FPS
unsigned long  stableUiAt = 0;

// Timed mute window
const unsigned long ACK_SILENCE_MS = 8000; // 8 seconds

// ======================= State Machine =======================
Mode mode = MODE_STABLE;

// Sweep state (for transitions)
bool          sweepActive = false;
unsigned long sweepStart  = 0;

// Sequence timing checks
unsigned long stabStartAt = 0;  // for checking when stabilizing completes
unsigned long freezeStartAt = 0;  // for checking when freezedown completes
unsigned long startupStartAt = 0;  // for checking when startup completes
unsigned long shutdownStartAt = 0;  // for checking when shutdown completes
unsigned long meltdownStartAt = 0;  // for tracking meltdown countdown display

// OLED caching
bool lastWarningShown = false;

// ======================= Forward Decls =======================
void enterStable();
void enterArming();
void enterMeltdown();
void enterStabilizing();
void enterStartup();
void enterFreezedown();
void enterShutdown();
void enterDark();
void enterChaos();

void abortStabilizingToMeltdown();
void finishStabilizingToStable();
void finishFreezedownToStable();

void tickSweep();

void drawCoreStatus(bool warning);
void drawCoreStatusForce(bool warning);
void drawCenteredBig(const char* txt, uint8_t size);

void renderStableUIFrame();
void renderActiveUIFrame();
void stopSweepIfAny();

// Heat bar helpers
void updateHeatTargetForMode();
void tickHeatBar();

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

  enterStable();
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
  if (ReactorEvents::isActive()) {
    char pressed = 0;
    if (overrideFell)   pressed = 'O';
    if (stabilizeFell)  pressed = 'S';
    if (startupFell)    pressed = 'U';
    if (freezedownFell) pressed = 'F';
    if (shutdownFell)   pressed = 'D';
    if (eventFell)      pressed = 'E';
    
    if (pressed == ReactorEvents::getRequiredButton()) {
      ReactorEvents::resolve();
      // Don't process normal button actions when resolving event
      return;
    } else if (pressed != 0) {
      // Wrong button pressed during event - ignore it
      return;
    }
  }

  // ---- Button -> Mode transitions ----
  if (overrideFell) {
    if (mode == MODE_STABLE)   enterArming();
    else if (mode == MODE_STABILIZING) abortStabilizingToMeltdown();
    else if (mode == MODE_STARTUP)     enterArming();
  }

  if (stabilizeFell) {
    if (mode == MODE_STABLE)        enterFreezedown(); // shortcut
    else if (mode == MODE_ARMING)   enterStabilizing();
    else if (mode == MODE_MELTDOWN) enterStabilizing();
    // In CHAOS, stabilize is ignored (only Startup can recover)
  }

  if (startupFell) {
    if (mode == MODE_STABLE)  enterStartup();
    else if (mode == MODE_DARK) enterStartup(); // wake from dark
    else if (mode == MODE_CHAOS) enterStartup(); // reboot from chaos
  }

  if (freezedownFell && (mode == MODE_STABLE || mode == MODE_MELTDOWN)) {
    enterFreezedown();
  }

  if (shutdownFell && (mode == MODE_STABLE || mode == MODE_CHAOS)) {
    enterShutdown();
  }

  // Event button triggers random event in stable mode
  if (eventFell && mode == MODE_STABLE && !ReactorEvents::isActive()) {
    ReactorEvents::trigger();
  }

  // If ACK pressed: start/extend mute and silence immediately
  if (ackFell) {
    ReactorAudio::muteFor(ACK_SILENCE_MS);
  }

  // ---- Tick current mode ----
  switch (mode) {
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
  ReactorSequences::tick(mode);

  // ---- Check for sequence completions ----
  unsigned long now = millis();
  
  // Stabilizing completes after STAB_TOTAL_STEPS steps
  if (mode == MODE_STABILIZING) {
    unsigned long elapsed = now - stabStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_STABILIZING)) * 1000;
    if (elapsed >= totalMs) {
      buzzerOff();
      finishStabilizingToStable();
    }
  }
  
  // Startup transitions to Stabilizing after STARTUP_TOTAL_STEPS steps
  if (mode == MODE_STARTUP) {
    unsigned long elapsed = now - startupStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_STARTUP)) * 2000;
    if (elapsed >= totalMs) {
      buzzerOff();
      digitalWrite(PIN_LED_STARTUP, LOW);
      enterStabilizing();
    }
  }
  
  // Freezedown completes after FREEZE_TOTAL_STEPS steps
  if (mode == MODE_FREEZEDOWN) {
    unsigned long elapsed = now - freezeStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_FREEZEDOWN)) * 1200;
    if (elapsed >= totalMs) {
      buzzerOff();
      finishFreezedownToStable();
    }
  }
  
  // Shutdown transitions to Dark after SHUTDOWN_TOTAL_STEPS steps
  if (mode == MODE_SHUTDOWN) {
    unsigned long elapsed = now - shutdownStartAt;
    unsigned long totalMs = (unsigned long)(ReactorSequences::getTotalSteps(MODE_SHUTDOWN)) * 2000;
    if (elapsed >= totalMs) {
      buzzerOff();
      enterDark();
    }
  }

  // ---- Event and secrets tick ----
  ReactorEvents::tick();
  ReactorSecrets::tick();

  // ---- Event trigger in stable mode ----
  if (eventFell && mode == MODE_STABLE && !ReactorEvents::isActive()) {
    ReactorEvents::trigger();
  }

  if (mode != MODE_CHAOS && mode != MODE_DARK && (now - uiFrameAt) >= UI_FRAME_MS) {
    uiFrameAt = now;
    renderActiveUIFrame();  // repaints current screen (incl. progress bars)
  }

  // Heat bar (skip during CHAOS and DARK)
  if (mode != MODE_CHAOS && mode != MODE_DARK) {
    updateHeatTargetForMode();
    ReactorHeat::tick(mode);
  }

  tickSweep();

  // Enforce buzzer mute if active (prevents any stray tone)
  ReactorAudio::tickMute();
}

// ======================= State Transitions =======================
void enterStable() {
  mode = MODE_STABLE;
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE,   HIGH);
  digitalWrite(PIN_LED_STARTUP,  LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  ReactorHeat::allOff();

  buzzerOff();
  ReactorUI::display.invertDisplay(false);
  drawCoreStatusForce(false);
}

void enterArming() {
  stopSweepIfAny();
  mode = MODE_ARMING;
  ReactorSequences::reset();

  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
  ReactorSequences::drawArmingNumber(ARM_BLINKS);
}

void enterMeltdown() {
  if (ReactorSecrets::isGodMode()) {
    buzzerOff();
    drawCenteredBig("MELTDOWN BLOCKED", 2);
    delay(600);
    enterStable();
    return;
  }

  stopSweepIfAny();
  mode = MODE_MELTDOWN;
  meltdownStartAt = millis();
  ReactorMeltdown::reset();

  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);

  drawCoreStatusForce(true); // initial banner
}

void enterStabilizing() {
  stopSweepIfAny();
  mode = MODE_STABILIZING;
  ReactorSequences::reset();
  stabStartAt = millis();

  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff(); // tick will start tones (gated by mute)
  ReactorSequences::drawStabilizingStep();
}

void enterStartup() {
  stopSweepIfAny();
  mode = MODE_STARTUP;
  ReactorSequences::reset();
  startupStartAt = millis();

  buzzerOff();
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  ReactorUI::display.invertDisplay(false);

  ReactorSequences::drawStartupStep();
}

void enterFreezedown() {
  stopSweepIfAny();
  mode = MODE_FREEZEDOWN;
  ReactorSequences::reset();
  freezeStartAt = millis();

  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE,   LOW);
  digitalWrite(PIN_LED_STARTUP,  LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
  ReactorUI::display.invertDisplay(false);

  ReactorSequences::drawFreezedownStep();
}

void enterShutdown() {
  stopSweepIfAny();
  mode = MODE_SHUTDOWN;
  ReactorSequences::reset();
  shutdownStartAt = millis();

  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
  ReactorUI::display.invertDisplay(false);

  ReactorSequences::drawShutdownStep();
}

void enterDark() {
  mode = MODE_DARK;
  ReactorDark::enterDarkWithSuccess();
}

void enterChaos() {
  mode = MODE_CHAOS;
  buzzerOff();
  ReactorChaos::reset();
}

// ---- exits ----
void finishFreezedownToStable() {
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  sweepActive = true;
  sweepStart  = millis();
  enterStable();
}

void abortStabilizingToMeltdown() {
  enterMeltdown();
}

void finishStabilizingToStable() {
  enterStable();
  sweepActive = true;
  sweepStart = millis();
}

// ======================= Ticks =======================


void tickSweep() {
  if (!sweepActive) return;

  unsigned long now = millis();
  unsigned long elapsed = now - sweepStart;

  if (elapsed >= SWEEP_MS) {
    buzzerOff();
    sweepActive = false;
    return;
  }

  float t = (float)elapsed / (float)SWEEP_MS;
  float ratio = (float)SWEEP_F1_HZ / (float)SWEEP_F0_HZ;
  float f = (float)SWEEP_F0_HZ * powf(ratio, t);
  if (f < 60) f = 60;
  buzzerTone((unsigned int)f);
}

// ======================= Helpers =======================
void stopSweepIfAny() {
  if (sweepActive) {
    sweepActive = false;
    buzzerOff();
  }
}

inline uint8_t currentHeatPercent() { return ReactorHeat::percent(); }

void drawCoreStatus(bool warning) {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.warning = warning;
  m.overheated = warning;
  if (warning) ReactorUI::ui.render(MODE_MELTDOWN, m, isMuted());
  else         ReactorUI::ui.render(MODE_STABLE, m, isMuted());
}

void drawCoreStatusForce(bool warning) {
  lastWarningShown = !warning; // force next draw
  drawCoreStatus(warning);
}

void drawCenteredBig(const char* txt, uint8_t size) {
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  ReactorUI::display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  int16_t x = ((int16_t)ReactorUI::display.width()  - (int16_t)w) / 2;
  int16_t y = ((int16_t)ReactorUI::display.height() - (int16_t)h) / 2;
  ReactorUI::display.setCursor(x, y);
  ReactorUI::display.print(txt);
  ReactorUI::display.display();
}


void renderStableUIFrame() {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  ReactorUI::ui.render(MODE_STABLE, m, isMuted());
}

void renderActiveUIFrame() {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();

  switch (mode) {
    case MODE_STABLE:
      // Only render background if no event, or render less frequently during events
      if (!ReactorEvents::isActive()) {
        ReactorUI::ui.render(MODE_STABLE, m, isMuted());
      } else {
        // During event, keep display static - only redraw event box
        static unsigned long lastEventDraw = 0;
        if (millis() - lastEventDraw > 500 || lastEventDraw == 0) {
          lastEventDraw = millis();
          ReactorUI::ui.render(MODE_STABLE, m, isMuted());
        }
        
        // Draw solid event box overlay
        int w = ReactorUI::display.width();
        ReactorUI::display.fillRect(4, 26, w-8, 32, SSD1306_BLACK);
        ReactorUI::display.drawRect(4, 26, w-8, 32, SSD1306_WHITE);
        ReactorUI::display.drawRect(5, 27, w-10, 30, SSD1306_WHITE);
        
        ReactorUI::display.setTextSize(1);
        ReactorUI::display.setTextColor(SSD1306_WHITE);
        
        // Center the event message
        ReactorUI::display.setCursor(10, 31);
        ReactorUI::display.println(ReactorEvents::getMessage());
        
        ReactorUI::display.setCursor(10, 42);
        ReactorUI::display.print("PRESS ");
        ReactorUI::display.println(ReactorEvents::getRequiredButtonName());
        
        ReactorUI::display.display();
      }
      break;

    case MODE_ARMING: {
      uint8_t step = ReactorSequences::getStep(MODE_ARMING);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_ARMING);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_ARMING, m, isMuted());
    } break;

    case MODE_STARTUP: {
      uint8_t step = ReactorSequences::getStep(MODE_STARTUP);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_STARTUP);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_STARTUP, m, isMuted());
    } break;

    case MODE_STABILIZING: {
      uint8_t step = ReactorSequences::getStep(MODE_STABILIZING);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_STABILIZING);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_STABILIZING, m, isMuted());
    } break;

    case MODE_FREEZEDOWN: {
      uint8_t step = ReactorSequences::getStep(MODE_FREEZEDOWN);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_FREEZEDOWN);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      m.freezing   = true;
      ReactorUI::ui.render(MODE_FREEZEDOWN, m, isMuted());
    } break;

    case MODE_SHUTDOWN: {
      uint8_t step = ReactorSequences::getStep(MODE_SHUTDOWN);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_SHUTDOWN);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_SHUTDOWN, m, isMuted());
    } break;

    case MODE_MELTDOWN: {
      unsigned long now = millis();
      unsigned long meltdownElapsed = (now >= meltdownStartAt) ? (now - meltdownStartAt) : 0;
      long remain = 10000L - (long)meltdownElapsed;  // 10 second countdown
      if (remain < 0) remain = 0;
      m.countdownMs = (int32_t)remain;
      m.warning     = true;
      m.overheated  = true;
      ReactorUI::ui.render(MODE_MELTDOWN, m, isMuted());
    } break;

    case MODE_CHAOS:
      // Chaos renders elsewhere
      break;
  }
}

// ======================= Heat Bar (helpers) =======================
void updateHeatTargetForMode() {
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

} // namespace ReactorSystem

