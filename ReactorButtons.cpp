#include "ReactorButtons.h"

namespace ReactorButtons {

namespace {
  const unsigned long DEBOUNCE_MS = 50;

  const uint8_t PIN_BUTTON_OVERRIDE   = 2;
  const uint8_t PIN_BUTTON_STABILIZE  = 5;
  const uint8_t PIN_BUTTON_STARTUP    = 10;
  const uint8_t PIN_BUTTON_FREEZEDOWN = 8;
  const uint8_t PIN_BUTTON_SHUTDOWN   = 6;
  const uint8_t PIN_BUTTON_EVENT      = 3;
  const uint8_t PIN_BUTTON_ACK        = 4;
}

Button overrideBtn;
Button stabilizeBtn;
Button startupBtn;
Button freezedownBtn;
Button shutdownBtn;
Button eventBtn;
Button ackBtn;

void Button::begin(uint8_t p) {
  pin = p;
  pinMode(pin, INPUT_PULLUP);
  bool r = digitalRead(pin);
  stableState = r;
  lastRaw = r;
  changedAt = 0;
  fellEvent = roseEvent = false;
}

void Button::update() {
  bool raw = digitalRead(pin);
  unsigned long now = millis();
  if (raw != lastRaw) {
    lastRaw = raw;
    changedAt = now;
  }
  if (now - changedAt > DEBOUNCE_MS) {
    if (raw != stableState) {
      stableState = raw;
      if (stableState == LOW) fellEvent = true; else roseEvent = true;
    }
  }
}

bool Button::fell() { bool e = fellEvent; fellEvent = false; return e; }
bool Button::rose() { bool e = roseEvent; roseEvent = false; return e; }
bool Button::isPressed() const { return stableState == LOW; }

void begin() {
  overrideBtn.begin(PIN_BUTTON_OVERRIDE);
  stabilizeBtn.begin(PIN_BUTTON_STABILIZE);
  startupBtn.begin(PIN_BUTTON_STARTUP);
  freezedownBtn.begin(PIN_BUTTON_FREEZEDOWN);
  shutdownBtn.begin(PIN_BUTTON_SHUTDOWN);
  eventBtn.begin(PIN_BUTTON_EVENT);
  ackBtn.begin(PIN_BUTTON_ACK);
}

void update() {
  overrideBtn.update();
  stabilizeBtn.update();
  startupBtn.update();
  freezedownBtn.update();
  shutdownBtn.update();
  eventBtn.update();
  ackBtn.update();
}

} // namespace ReactorButtons
