#include "ReactorHeat.h"

namespace ReactorHeat {

namespace {
  const uint8_t HEAT_COUNT = 12;
  const uint8_t HEAT_PINS[HEAT_COUNT] = {
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
  };
  const bool HEAT_ACTIVE_HIGH = true;

  const unsigned long HEAT_TICK_MS = 40;       // ~25 FPS
  const float         HEAT_SLEW_LVL_PER_S = 8; // levels/sec (0..12)

  float heatValue  = 2.0f;
  float heatTarget = 2.0f;
  unsigned long heatTickAt = 0;

  inline void heatWrite(uint8_t idx, bool on) {
    if (idx >= HEAT_COUNT) return;
    digitalWrite(HEAT_PINS[idx], (on ^ !HEAT_ACTIVE_HIGH) ? HIGH : LOW);
  }

  float clampLevel(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > (float)HEAT_COUNT) return (float)HEAT_COUNT;
    return v;
  }
}

void begin() {
  for (uint8_t i = 0; i < HEAT_COUNT; ++i) {
    pinMode(HEAT_PINS[i], OUTPUT);
    heatWrite(i, false);
  }
  heatTickAt = millis();
}

void setTarget(float level) {
  heatTarget = clampLevel(level);
}

void setLevel(float level) {
  heatValue = clampLevel(level);
  heatTarget = heatValue;
}

float getLevel() {
  return heatValue;
}

uint8_t percent() {
  float pct = (heatValue / (float)HEAT_COUNT) * 100.0f;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return (uint8_t)(pct + 0.5f);
}

void tick(Mode mode) {
  unsigned long now = millis();
  if (now - heatTickAt < HEAT_TICK_MS) return;
  float dt = (now - heatTickAt) / 1000.0f;
  heatTickAt = now;

  float maxDelta = HEAT_SLEW_LVL_PER_S * dt;
  float delta = heatTarget - heatValue;
  if (delta >  maxDelta) delta =  maxDelta;
  if (delta < -maxDelta) delta = -maxDelta;
  heatValue += delta;

  int lit = (int)roundf(heatValue);
  if (lit < 0) lit = 0; if (lit > HEAT_COUNT) lit = HEAT_COUNT;

  for (int i = 0; i < HEAT_COUNT; ++i) {
    bool on = (i < lit);
    heatWrite(i, on);
  }

  if (mode == MODE_MELTDOWN) {
    bool blink = ((now / 150) % 2) == 0;
    heatWrite(HEAT_COUNT - 1, blink);
    heatWrite(HEAT_COUNT - 2, blink);
  }

  if (mode == MODE_FREEZEDOWN) {
    bool twinkle = ((now / 250) % 2) == 0;
    heatWrite(0, twinkle);
    heatWrite(1, twinkle);
  }
}

void allOff() {
  for (uint8_t i = 0; i < HEAT_COUNT; ++i) heatWrite(i, false);
}

void chaosFlicker() {
  for (uint8_t i = 0; i < HEAT_COUNT; ++i) {
    heatWrite(i, random(2));
  }
}

} // namespace ReactorHeat
