#include "ReactorChaos.h"
#include "ReactorAudio.h"
#include "ReactorHeat.h"
#include "ReactorUI.h"

namespace ReactorChaos {

// ======================= State Variables =======================
unsigned long chaosTickAt = 0;
unsigned long chaosInvertAt = 0;

// ======================= Pins =======================
const uint8_t PIN_LED_MELTDOWN      = 13;
const uint8_t PIN_LED_STABLE        = 12;
const uint8_t PIN_LED_STARTUP       = 11;
const uint8_t PIN_LED_FREEZEDOWN    = 9;

// ======================= Helpers =======================
inline void buzzerTone(unsigned int hz) { ReactorAudio::toneHz(hz); }

// ======================= API =======================
void begin() {
  pinMode(PIN_LED_MELTDOWN, OUTPUT);
  pinMode(PIN_LED_STABLE, OUTPUT);
  pinMode(PIN_LED_STARTUP, OUTPUT);
  pinMode(PIN_LED_FREEZEDOWN, OUTPUT);
  reset();
}

void reset() {
  chaosTickAt = 0;
  chaosInvertAt = 0;
  
  // Kill everything
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  ReactorUI::display.clearDisplay();
  ReactorUI::display.display();
}

void tick() {
  unsigned long now = millis();

  // Randomize indicator LEDs fast
  if (now - chaosTickAt >= 60) {
    chaosTickAt = now;
    digitalWrite(PIN_LED_MELTDOWN,  random(2) ? HIGH : LOW);
    digitalWrite(PIN_LED_STABLE,    random(2) ? HIGH : LOW);
    digitalWrite(PIN_LED_STARTUP,   random(2) ? HIGH : LOW);
    digitalWrite(PIN_LED_FREEZEDOWN,random(2) ? HIGH : LOW);

    // Heat bar raw flicker (override smoothing while in CHAOS)
    ReactorHeat::chaosFlicker();

    // Buzzer chaos (still respects mute via wrapper)
    int f = random(220, 2200);
    buzzerTone(f);

    // LCD artifacts
    int w = ReactorUI::display.width();
    int h = ReactorUI::display.height();
    if (random(4) == 0) {
      ReactorUI::display.fillRect(random(w), random(h), random(10, 40), random(4, 20), SSD1306_WHITE);
    } else if (random(4) == 0) {
      ReactorUI::display.drawLine(random(w), random(h),
                                  random(w), random(h),
                                  SSD1306_WHITE);
    } else {
      for (int i = 0; i < 60; ++i)
        ReactorUI::display.drawPixel(random(w), random(h), SSD1306_WHITE);
    }
    if (random(5) == 0) ReactorUI::display.clearDisplay();
    ReactorUI::display.display();
  }

  // Periodic invert flash
  if (now - chaosInvertAt >= 180) {
    chaosInvertAt = now;
    ReactorUI::display.invertDisplay(random(2));
  }
}

} // namespace ReactorChaos
