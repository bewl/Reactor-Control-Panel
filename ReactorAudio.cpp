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
  // "Final Countdown" - transcribed from sheet music
  // Key: F# minor (4 sharps), 90 BPM, 4/4 time
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // At 90 BPM: quarter = 667ms, eighth = 333ms, triplet eighth = 222ms
  const Note melody[] = {
    // Bar 1 (F#m): Triplet figure - accented notes
    {740, 222},   // F#5 (triplet eighth)
    {880, 222},   // A5 (triplet eighth)
    {988, 222},   // B5 (triplet eighth)
    {880, 333},   // A5 (eighth)
    
    // Bar 2 (D): Triplet figure
    {740, 222},   // F#5 (triplet eighth)
    {880, 222},   // A5 (triplet eighth)
    {988, 222},   // B5 (triplet eighth)
    {880, 333},   // A5 (eighth)
    {880, 333},   // A5 (eighth)
    
    // Bar 3 (Bm): Triplet figure  
    {880, 222},   // A5 (triplet eighth)
    {740, 222},   // F#5 (triplet eighth)
    {880, 222},   // A5 (triplet eighth)
    {880, 333},   // A5 (eighth)
    
    // Bar 4 (E): Triplet figure + held notes
    {740, 222},   // F#5 (triplet eighth)
    {880, 222},   // A5 (triplet eighth)
    {988, 222},   // B5 (triplet eighth)
    {1047, 222},  // C#6 (triplet eighth)
    {1175, 222},  // D6 (triplet eighth)
    {1319, 222},  // E6 (triplet eighth)
    {1319, 1334}, // E6 (held - whole note duration)
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
