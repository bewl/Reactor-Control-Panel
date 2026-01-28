#include "ReactorStateMachine.h"
#include "ReactorTypes.h"
#include "ReactorSequences.h"
#include "ReactorSweep.h"
#include "ReactorAudio.h"
#include "ReactorHeat.h"
#include "ReactorSecrets.h"
#include "ReactorMeltdown.h"
#include "ReactorChaos.h"
#include "ReactorDark.h"
#include "ReactorUIFrames.h"
#include "ReactorUI.h"
#include <Arduino.h>

namespace ReactorStateMachine {

// ======================= Pins =======================
const uint8_t PIN_LED_MELTDOWN      = 13;
const uint8_t PIN_LED_STABLE        = 12;
const uint8_t PIN_LED_STARTUP       = 11;
const uint8_t PIN_LED_FREEZEDOWN    = 9;

// Arming (3-2-1)
const uint8_t ARM_BLINKS = 5;

// ======================= State =======================
Mode currentMode = MODE_STABLE;

// Sequence timing
unsigned long armingStartAt = 0;    // 5 second countdown
unsigned long stabStartAt = 0;
unsigned long freezeStartAt = 0;
unsigned long startupStartAt = 0;
unsigned long shutdownStartAt = 0;
unsigned long meltdownStartAt = 0;  // 10 second countdown

// ======================= Helpers =======================
inline void buzzerOff() { ReactorAudio::off(); }

// ======================= API =======================
Mode getMode() {
  return currentMode;
}

void enterStable() {
  currentMode = MODE_STABLE;
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE,   HIGH);
  digitalWrite(PIN_LED_STARTUP,  LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  ReactorHeat::allOff();

  buzzerOff();
  ReactorUI::display.invertDisplay(false);
  ReactorUIFrames::drawCoreStatusForce(false);
}

void enterArming() {
  ReactorSweep::stop();
  currentMode = MODE_ARMING;
  ReactorSequences::reset();
  armingStartAt = millis();  // Start 5 second countdown

  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
}

void enterMeltdown() {
  if (ReactorSecrets::isGodMode()) {
    buzzerOff();
    ReactorUIFrames::drawCenteredBig("MELTDOWN BLOCKED", 2);
    delay(600);
    enterStable();
    return;
  }

  ReactorSweep::stop();
  currentMode = MODE_MELTDOWN;
  meltdownStartAt = millis();
  ReactorMeltdown::reset();

  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);

  ReactorUIFrames::drawCoreStatusForce(true); // initial banner
}

void enterStabilizing() {
  ReactorSweep::stop();
  currentMode = MODE_STABILIZING;
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
  ReactorSweep::stop();
  currentMode = MODE_STARTUP;
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
  ReactorSweep::stop();
  currentMode = MODE_FREEZEDOWN;
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
  ReactorSweep::stop();
  currentMode = MODE_SHUTDOWN;
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
  currentMode = MODE_DARK;
  ReactorDark::enterDarkWithSuccess();
}

void enterChaos() {
  currentMode = MODE_CHAOS;
  buzzerOff();
  ReactorChaos::reset();
}

// ---- exits ----
void finishFreezedownToStable() {
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  ReactorSweep::start();
  enterStable();
}

void abortStabilizingToMeltdown() {
  enterMeltdown();
}

void finishStabilizingToStable() {
  enterStable();
  ReactorSweep::start();
}

} // namespace ReactorStateMachine
