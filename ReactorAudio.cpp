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
  // "Final Countdown" - transcribed from treble clef score
  // Key: A major/F# minor (3 sharps), 90 BPM, 4/4 time
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // At 90 BPM: quarter = 667ms, eighth = 333ms, triplet eighth = 222ms
  const Note melody[] = {
    // Bar 1: C#-B-C#-F#
    {1109, 222},  // C#6 (triplet)
    {988, 222},   // B5 (triplet)
    {1109, 222},  // C#6 (triplet)
    {740, 667},   // F#5 (quarter)
    
    // Bar 2: D-C#-D-C#-B
    {1175, 222},  // D6 (triplet)
    {1109, 222},  // C#6 (triplet)
    {1175, 222},  // D6 (triplet)
    {1109, 333},  // C#6 (eighth)
    {988, 333},   // B5 (eighth)
    
    // Bar 3: D-C#-D-F#
    {1175, 222},  // D6 (triplet)
    {1109, 222},  // C#6 (triplet)
    {1175, 222},  // D6 (triplet)
    {740, 667},   // F#5 (quarter)
    
    // Bar 4: B-A-G#-B-A
    {988, 222},   // B5 (triplet)
    {880, 222},   // A5 (triplet)
    {831, 222},   // G#5 (triplet)
    {988, 333},   // B5 (eighth)
    {880, 333},   // A5 (eighth)
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
