#include "ReactorSweep.h"
#include "ReactorAudio.h"
#include <math.h>

namespace ReactorSweep {

// Shutdown sweep (used by transitions)
const unsigned long SWEEP_MS     = 1000;
const int           SWEEP_F0_HZ  = 1800;
const int           SWEEP_F1_HZ  = 140;

// State
bool          sweepActive = false;
unsigned long sweepStart  = 0;

inline void buzzerOff() { ReactorAudio::off(); }
inline void buzzerTone(unsigned int hz) { ReactorAudio::toneHz(hz); }

void start() {
  sweepActive = true;
  sweepStart = millis();
}

void stop() {
  if (sweepActive) {
    sweepActive = false;
    buzzerOff();
  }
}

void tick() {
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

} // namespace ReactorSweep
