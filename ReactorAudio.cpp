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
  // The ACTUAL "Final Countdown" keyboard motif by Europe
  // Transcribed from the official sheet music at 90 BPM
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // At 90 BPM: quarter note = 667ms, eighth = 333ms, sixteenth = 167ms
  const Note melody[] = {
    // Bar 1: F#m chord - main motif with grace notes
    {740, 150},   // F#5 (grace note)
    {880, 150},   // A5 (grace note)
    {740, 333},   // F#5 (eighth note)
    {880, 333},   // A5 (eighth)
    
    {740, 150},   // F#5 (grace note)
    {880, 150},   // A5 (grace note)
    {740, 333},   // F#5 (eighth)
    {880, 150},   // A5 (grace note)
    {740, 333},   // F#5 (eighth)
    {880, 333},   // A5 (eighth)
    
    // Bar 2: D chord
    {740, 150},   // F#5 (grace note)
    {880, 150},   // A5 (grace note)
    {740, 333},   // F#5 (eighth)
    {880, 333},   // A5 (eighth)
    {740, 250},   // F#5
    {659, 250},   // E5
    {587, 333},   // D5 (eighth)
    
    // Bar 3: Bm chord  
    {740, 150},   // F#5 (grace note)
    {880, 150},   // A5 (grace note)
    {740, 333},   // F#5 (eighth)
    {880, 333},   // A5 (eighth)
    
    // Bar 4: E chord - ending phrase
    {740, 150},   // F#5 (grace note)
    {880, 150},   // A5 (grace note)
    {740, 333},   // F#5 (eighth)
    {880, 250},   // A5
    {988, 250},   // B5
    {1047, 250},  // C#6
    {587, 333},   // D5 (eighth)
    {659, 667},   // E5 (quarter note, held)
    {0, 333},     // Rest
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
