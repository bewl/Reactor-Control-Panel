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
  // Transcribed EXACTLY from the provided sheet music
  // 90 BPM, 4/4 time, F# minor
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // At 90 BPM: quarter = 667ms, eighth = 333ms, sixteenth = 167ms, grace = ~80ms
  const Note melody[] = {
    // Bar 1 (F#m): Grace notes + repeated A notes
    {880, 80},    // A5 (grace)
    {988, 80},    // B5 (grace) 
    {1047, 80},   // C6 (grace)
    {880, 333},   // A5 (eighth)
    
    {880, 80},    // A5 (grace)
    {988, 80},    // B5 (grace)
    {1047, 80},   // C6 (grace)
    {880, 333},   // A5 (eighth)
    {880, 333},   // A5 (eighth)
    
    // Bar 2 (D): Grace notes + descent
    {880, 80},    // A5 (grace)
    {988, 80},    // B5 (grace)
    {1047, 80},   // C6 (grace)
    {880, 333},   // A5 (eighth)
    {988, 333},   // B5 (eighth)
    {1047, 333},  // C6 (eighth)
    
    // Bar 3 (Bm): Grace notes + repeated
    {880, 80},    // A5 (grace)
    {988, 80},    // B5 (grace)
    {1047, 80},   // C6 (grace)
    {880, 333},   // A5 (eighth)
    
    // Bar 4 (E): Grace notes + ascending run
    {880, 80},    // A5 (grace)
    {988, 80},    // B5 (grace)
    {1047, 80},   // C6 (grace)
    {880, 333},   // A5 (eighth)
    {988, 333},   // B5 (eighth)
    {1047, 333},  // C6 (eighth)
    {1175, 333},  // D6 (eighth)
    {1319, 667},  // E6 (quarter - held)
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
