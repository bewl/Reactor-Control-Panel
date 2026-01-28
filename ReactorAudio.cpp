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
  // The ACTUAL iconic "Final Countdown" keyboard riff by Europe
  // This is the famous opening synth fanfare
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // Key of F# minor - the real melody!
  const Note melody[] = {
    // First phrase: "doo doo doo DOO doo" 
    {740, 200},   // F#5
    {740, 200},   // F#5  
    {587, 200},   // D5
    {554, 400},   // C#5 (held longer)
    {0, 100},     // Short rest
    
    // Second phrase: descending pattern
    {740, 200},   // F#5
    {659, 200},   // E5
    {587, 200},   // D5  
    {554, 400},   // C#5 (held)
    {0, 150},     // Rest
    
    // Repeat first phrase
    {740, 200},   // F#5
    {740, 200},   // F#5
    {587, 200},   // D5
    {554, 400},   // C#5
    {0, 100},     // Short rest
    
    // Final phrase with slight variation
    {740, 200},   // F#5
    {659, 200},   // E5
    {587, 200},   // D5
    {494, 600},   // B4 (finish note, held longer)
    {0, 300},     // Final rest
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
