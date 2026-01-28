#include "ReactorAudio.h"

namespace ReactorAudio {

namespace {
  uint8_t g_buzzerPin = 255;
  unsigned long g_muteUntil = 0;
  unsigned int g_toneHz = 0;
  bool g_toneOn = false;
}

void begin(uint8_t buzzerPin) {
  g_buzzerPin = buzzerPin;
  pinMode(g_buzzerPin, OUTPUT);
  digitalWrite(g_buzzerPin, LOW);
  off();
}

bool isMuted() {
  return g_muteUntil && (millis() < g_muteUntil);
}

void off() {
  if (g_toneOn) {
    noTone(g_buzzerPin);
    g_toneOn = false;
    g_toneHz = 0;
  }
}

void toneHz(unsigned int hz) {
  if (hz == 0) { off(); return; }
  if (isMuted()) return;
  if (g_toneOn && g_toneHz == hz) return;
  tone(g_buzzerPin, hz);
  g_toneOn = true;
  g_toneHz = hz;
}

void muteFor(unsigned long ms) {
  g_muteUntil = millis() + ms;
  off();
}

void tickMute() {
  if (!g_muteUntil) return;
  if (millis() < g_muteUntil) {
    off();
  } else {
    g_muteUntil = 0;
  }
}

void playFinalCountdown() {
  // The iconic "Final Countdown" synth riff by Europe
  // Frequencies and durations for the classic melody motif
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  const Note melody[] = {
    // Main riff - iconic descending pattern
    {659, 200},   // E5
    {659, 200},   // E5
    {659, 200},   // E5
    {1047, 200},  // C6
    {659, 200},   // E5
    {1047, 200},  // C6
    {523, 200},   // C5
    {494, 200},   // B4
    {494, 200},   // B4
    {494, 200},   // B4
    {784, 200},   // G5
    {880, 200},   // A5
    {659, 200},   // E5
    {659, 200},   // E5
    {659, 200},   // E5
    {1047, 200},  // C6
    {659, 200},   // E5
    {1047, 200},  // C6
    {523, 200},   // C5
    {494, 200},   // B4
    {0, 400},     // Rest for dramatic pause
  };
  
  const uint8_t numNotes = sizeof(melody) / sizeof(Note);
  
  for (uint8_t i = 0; i < numNotes; i++) {
    if (melody[i].freq > 0) {
      toneHz(melody[i].freq);
    } else {
      off();
    }
    delay(melody[i].durationMs);
  }
  
  off();
}

} // namespace ReactorAudio
