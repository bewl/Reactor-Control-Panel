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
  // The "Final Countdown" keyboard motif by Europe
  // Transcribed from sheet music - 90 BPM, 4/4 time, F# minor
  struct Note {
    unsigned int freq;
    unsigned int durationMs;
  };
  
  // At 90 BPM: quarter = 667ms, eighth = 333ms, sixteenth = 167ms
  const Note melody[] = {
    // Bar 1 (F#m): Sixteenth note runs
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    
    // Bar 2 (D): Pattern continues
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    
    // Bar 3 (Bm): Continues
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    
    // Bar 4 (E): Final ascending run
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {880, 167},   // A5 (sixteenth)
    {988, 167},   // B5 (sixteenth)
    {1047, 167},  // C#6 (sixteenth)
    {1175, 167},  // D6 (sixteenth)
    {1319, 167},  // E6 (sixteenth)
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
