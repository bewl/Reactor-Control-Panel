#include "ReactorSecrets.h"
#include "ReactorUI.h"
#include "ReactorAudio.h"
#include "ReactorHeat.h"

namespace ReactorSecrets {

namespace {
  // Sequence capture
  const uint8_t SEQ_MAX = 10;
  char seqBuffer[SEQ_MAX];
  uint8_t seqLength = 0;
  unsigned long seqLastInput = 0;
  const unsigned long SEQ_TIMEOUT_MS = 3000; // 3s between presses

  // Secret modes
  bool g_godMode = false;
  unsigned long g_cryoUntil = 0;
  const unsigned long CRYO_LOCK_MS = 12000; // 12s of heavy cooling
  
  static inline bool isMuted() { return ReactorAudio::isMuted(); }
}

bool isGodMode() {
  return g_godMode;
}

bool isCryoLocked() {
  return g_cryoUntil && millis() < g_cryoUntil;
}

static uint8_t patternLenNoSpaces(const char* p) {
  uint8_t n = 0;
  for (uint8_t i=0; p[i]; ++i) if (p[i] != ' ') n++;
  return n;
}

static bool matchesExact(const char* pattern) {
  uint8_t want = patternLenNoSpaces(pattern);
  if (seqLength != want) return false;
  uint8_t j = 0;
  for (uint8_t i=0; pattern[i]; ++i) {
    if (pattern[i] == ' ') continue;
    if (seqBuffer[j++] != pattern[i]) return false;
  }
  return true;
}

void secretToneSweep() {
  if (isMuted()) return;
  for (int f = 420; f < 1800; f += 90) {
    ReactorAudio::toneHz(f);
    delay(20);
    if (isMuted()) { ReactorAudio::off(); return; }
  }
  ReactorAudio::off();
}

void enterGodMode() {
  g_godMode = true;
  secretToneSweep();
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setTextSize(1);
  int16_t x1, y1; uint16_t w, h;
  ReactorUI::display.getTextBounds("OVERRIDE PROTOCOL", 0, 0, &x1, &y1, &w, &h);
  int16_t x = ((int16_t)ReactorUI::display.width()  - (int16_t)w) / 2;
  int16_t y = ((int16_t)ReactorUI::display.height() - (int16_t)h) / 2;
  ReactorUI::display.setCursor(x, y);
  ReactorUI::display.println("OVERRIDE PROTOCOL");
  ReactorUI::display.display();
  delay(650);
  
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextSize(2);
  ReactorUI::display.getTextBounds("GOD MODE", 0, 0, &x1, &y1, &w, &h);
  x = ((int16_t)ReactorUI::display.width()  - (int16_t)w) / 2;
  y = ((int16_t)ReactorUI::display.height() - (int16_t)h) / 2;
  ReactorUI::display.setCursor(x, y);
  ReactorUI::display.println("GOD MODE");
  ReactorUI::display.display();
  delay(700);
}

void enterCryoLockdown() {
  secretToneSweep();
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setTextSize(2);
  int16_t x1, y1; uint16_t w, h;
  ReactorUI::display.getTextBounds("CRYO LOCKDOWN", 0, 0, &x1, &y1, &w, &h);
  int16_t x = ((int16_t)ReactorUI::display.width()  - (int16_t)w) / 2;
  int16_t y = ((int16_t)ReactorUI::display.height() - (int16_t)h) / 2;
  ReactorUI::display.setCursor(x, y);
  ReactorUI::display.println("CRYO LOCKDOWN");
  ReactorUI::display.display();
  delay(700);
  
  float cooled = max(ReactorHeat::getLevel() - 3.0f, 0.0f);
  ReactorHeat::setLevel(cooled);
  g_cryoUntil = millis() + CRYO_LOCK_MS;
}

void checkSecretSequence() {
  if (millis() - seqLastInput > SEQ_TIMEOUT_MS) {
    seqLength = 0;
    return;
  }
  
  // S - (S)tabilize  (Green Button)
  // O - (O)verride   (Red Button)
  // U - Start(U)p    (Yellow Button)
  // F - (F)reezedown (Blue Button)

  const char* GOD_SEQ   = "O S F U O";
  const char* CHAOS_SEQ = "U U F S O F";
  const char* CRYO_SEQ  = "F F O S";

  uint8_t n = seqLength;
  if (n == patternLenNoSpaces(GOD_SEQ) && matchesExact(GOD_SEQ)) {
    seqLength = 0;
    enterGodMode();
    return;
  }
  if (n == patternLenNoSpaces(CHAOS_SEQ) && matchesExact(CHAOS_SEQ)) {
    seqLength = 0;
    // Chaos entry will be handled by ReactorSystem
    return;
  }
  if (n == patternLenNoSpaces(CRYO_SEQ) && matchesExact(CRYO_SEQ)) {
    seqLength = 0;
    enterCryoLockdown();
    return;
  }

  uint8_t maxPat = max(patternLenNoSpaces(GOD_SEQ), max(patternLenNoSpaces(CHAOS_SEQ), patternLenNoSpaces(CRYO_SEQ)));
  if (seqLength > maxPat) seqLength = 0;
}

void begin() {
  seqLength = 0;
  seqLastInput = 0;
  g_godMode = false;
  g_cryoUntil = 0;
}

void captureInput(char code) {
  if (seqLength < SEQ_MAX) {
    seqBuffer[seqLength++] = code;
    seqLastInput = millis();
    checkSecretSequence();
  } else {
    seqLength = 0;
  }
}

void tick() {
  if (seqLength > 0 && (millis() - seqLastInput > SEQ_TIMEOUT_MS)) {
    seqLength = 0; // timeout-based reset
  }
  if (g_cryoUntil && millis() >= g_cryoUntil) {
    g_cryoUntil = 0;
  }
}

} // namespace ReactorSecrets
