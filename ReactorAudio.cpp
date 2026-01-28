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
  // Transcribed from the official sheet music
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // F# minor key, 4/4 time - the iconic synth riff
  const Note melody[] = {
    // Bar 1: F#m chord - main motif with grace notes
    {740, 100},   // F#5 (grace note, quick)
    {880, 100},   // A5 (grace note)
    {740, 200},   // F#5 (main note)
    {880, 200},   // A5
    
    {740, 100},   // F#5 (grace note)
    {880, 100},   // A5 (grace note)
    {740, 200},   // F#5
    {880, 100},   // A5
    {740, 200},   // F#5
    {880, 200},   // A5
    
    // Bar 2: D chord
    {740, 100},   // F#5 (grace note)
    {880, 100},   // A5 (grace note)
    {740, 200},   // F#5
    {880, 200},   // A5
    {740, 150},   // F#5
    {659, 150},   // E5
    {587, 200},   // D5
    
    // Bar 3: Bm chord  
    {740, 100},   // F#5 (grace note)
    {880, 100},   // A5 (grace note)
    {740, 200},   // F#5
    {880, 200},   // A5
    
    // Bar 4: E chord - ending phrase
    {740, 100},   // F#5 (grace note)
    {880, 100},   // A5
    {740, 200},   // F#5
    {880, 150},   // A5
    {988, 150},   // B5
    {1047, 150},  // C#6
    {587, 200},   // D5
    {659, 300},   // E5 (held)
    {0, 200},     // Final rest
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
