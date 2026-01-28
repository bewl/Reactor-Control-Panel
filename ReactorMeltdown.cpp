#include "ReactorMeltdown.h"
#include "ReactorAudio.h"
#include "ReactorSequences.h"

namespace ReactorMeltdown {

// ======================= Timing Constants =======================
const unsigned long MELTDOWN_BLINK_MS = 125;
const unsigned int  MELTDOWN_TONE_HZ  = 1000;
const unsigned long MELTDOWN_COUNTDOWN_MS = 10000; // 10 seconds

// ======================= State Variables =======================
unsigned long meltdownTickAt = 0;
bool          meltdownPhase  = false;
unsigned long meltdownStart  = 0;
int           lastCountdownSec = -1;
unsigned long lastCountdownDrawAt = 0;

// ======================= Pins =======================
const uint8_t PIN_LED_MELTDOWN = 13;

// ======================= Helpers =======================
inline void buzzerOff() { ReactorAudio::off(); }
inline void buzzerTone(unsigned int hz) { ReactorAudio::toneHz(hz); }

// Forward declarations for ReactorSystem transitions
namespace ReactorSystem {
  void enterChaos();
}

// ======================= API =======================
void begin() {
  pinMode(PIN_LED_MELTDOWN, OUTPUT);
  reset();
}

void reset() {
  meltdownPhase = false;
  meltdownTickAt = 0;
  meltdownStart = millis();
  lastCountdownSec = -1;
  lastCountdownDrawAt = 0;
}

void tick() {
  unsigned long now = millis();

  // Blink LED & basic alarm tone
  if (now - meltdownTickAt >= MELTDOWN_BLINK_MS) {
    meltdownTickAt = now;
    meltdownPhase = !meltdownPhase;
    digitalWrite(PIN_LED_MELTDOWN, meltdownPhase ? HIGH : LOW);
    if (meltdownPhase) buzzerTone(MELTDOWN_TONE_HZ);
    else               buzzerOff();
  }

  // Countdown to CHAOS
  unsigned long elapsed = now - meltdownStart;
  if (elapsed >= MELTDOWN_COUNTDOWN_MS) {
    buzzerOff();
    ReactorSystem::enterChaos();
    return;
  }

  // Refresh countdown display ~5fps or when the second changes
  int secRemain = (int)((MELTDOWN_COUNTDOWN_MS - elapsed + 999) / 1000); // ceil
  if (secRemain != lastCountdownSec || (now - lastCountdownDrawAt) > 200) {
    lastCountdownSec = secRemain;
    lastCountdownDrawAt = now;
    ReactorSequences::drawMeltdownCountdown();
  }
}

} // namespace ReactorMeltdown
