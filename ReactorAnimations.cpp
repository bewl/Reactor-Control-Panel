#include "ReactorAnimations.h"
#include <Arduino.h>

namespace ReactorAnimations {

// ======================= Constants =======================
// Screen layout (must match ReactorUI layout)
const uint8_t CONTENT_Y_START = 27;
const uint8_t CONTENT_Y_END = 54;
const uint8_t CONTENT_HEIGHT = CONTENT_Y_END - CONTENT_Y_START;
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;

// ======================= Particle System =======================
static Particle particles[MAX_PARTICLES];
static uint32_t lastParticleSpawn = 0;

static void spawnParticle(float x, float y, float vx, float vy, uint8_t life) {
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life == 0) {
      particles[i].x = x;
      particles[i].y = y;
      particles[i].vx = vx;
      particles[i].vy = vy;
      particles[i].life = life;
      particles[i].maxLife = life;
      return;
    }
  }
}

static void updateParticles() {
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      particles[i].x += particles[i].vx;
      particles[i].y += particles[i].vy;
      particles[i].life--;
      
      // Bounds check - kill particles outside content area
      if (particles[i].y < CONTENT_Y_START || particles[i].y > CONTENT_Y_END ||
          particles[i].x < 8 || particles[i].x > SCREEN_WIDTH - 8) {
        particles[i].life = 0;
      }
    }
  }
}

void drawDecayParticles(Adafruit_SSD1306& display, uint32_t nowMs) {
  // Spawn new particle every 200ms
  if (nowMs - lastParticleSpawn > 200) {
    lastParticleSpawn = nowMs;
    float x = SCREEN_WIDTH / 2 + random(-10, 10);
    float y = (CONTENT_Y_START + CONTENT_Y_END) / 2;
    float vx = (random(-20, 20)) / 20.0f;
    float vy = -0.5f - (random(0, 10) / 20.0f);  // Upward
    spawnParticle(x, y, vx, vy, 40);
  }
  
  updateParticles();
  
  // Draw particles
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      // Fade based on remaining life
      if (particles[i].life > particles[i].maxLife / 2) {
        display.drawPixel((int16_t)particles[i].x, (int16_t)particles[i].y, SSD1306_WHITE);
      }
    }
  }
}

void drawCoolantFlow(Adafruit_SSD1306& display, uint32_t nowMs) {
  // Spawn droplets from top of content area
  if (nowMs - lastParticleSpawn > 150) {
    lastParticleSpawn = nowMs;
    float x = 8 + random(0, SCREEN_WIDTH - 16);
    float y = CONTENT_Y_START;
    float vx = (random(-5, 5)) / 10.0f;
    float vy = 0.8f + (random(0, 10) / 20.0f);  // Downward
    spawnParticle(x, y, vx, vy, 35);
  }
  
  updateParticles();
  
  // Draw droplets as small lines
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      int16_t x = (int16_t)particles[i].x;
      int16_t y = (int16_t)particles[i].y;
      display.drawPixel(x, y, SSD1306_WHITE);
      display.drawPixel(x, y + 1, SSD1306_WHITE);
    }
  }
}

void drawMeltdownSparks(Adafruit_SSD1306& display, uint32_t nowMs) {
  // Frequent explosive sparks
  if (nowMs - lastParticleSpawn > 80) {
    lastParticleSpawn = nowMs;
    float x = SCREEN_WIDTH / 2 + random(-20, 20);
    float y = (CONTENT_Y_START + CONTENT_Y_END) / 2;
    float angle = random(0, 628) / 100.0f;  // 0 to 2*PI
    float speed = 1.0f + random(0, 15) / 10.0f;
    float vx = cos(angle) * speed;
    float vy = sin(angle) * speed;
    spawnParticle(x, y, vx, vy, 25);
  }
  
  updateParticles();
  
  // Draw bright sparks
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0) {
      int16_t x = (int16_t)particles[i].x;
      int16_t y = (int16_t)particles[i].y;
      // Draw as cross for brightness
      display.drawPixel(x, y, SSD1306_WHITE);
      display.drawPixel(x - 1, y, SSD1306_WHITE);
      display.drawPixel(x + 1, y, SSD1306_WHITE);
      display.drawPixel(x, y - 1, SSD1306_WHITE);
      display.drawPixel(x, y + 1, SSD1306_WHITE);
    }
  }
}

void drawFreezeParticles(Adafruit_SSD1306& display, uint32_t nowMs) {
  // Gentle falling snowflakes
  if (nowMs - lastParticleSpawn > 250) {
    lastParticleSpawn = nowMs;
    float x = 8 + random(0, SCREEN_WIDTH - 16);
    float y = CONTENT_Y_START;
    float vx = (random(-8, 8)) / 20.0f;
    float vy = 0.3f + (random(0, 10) / 30.0f);  // Slow fall
    spawnParticle(x, y, vx, vy, 60);
  }
  
  updateParticles();
  
  // Draw snowflakes
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    if (particles[i].life > 0 && (particles[i].life % 3) == 0) {
      int16_t x = (int16_t)particles[i].x;
      int16_t y = (int16_t)particles[i].y;
      // Snowflake shape (+ pattern)
      display.drawPixel(x, y, SSD1306_WHITE);
      display.drawPixel(x - 1, y, SSD1306_WHITE);
      display.drawPixel(x + 1, y, SSD1306_WHITE);
      display.drawPixel(x, y - 1, SSD1306_WHITE);
      display.drawPixel(x, y + 1, SSD1306_WHITE);
    }
  }
}

// ======================= Waveforms =======================

void drawEnhancedPulse(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t activity) {
  const uint8_t y0 = 46;  // Base Y position (between content and status text)
  const float speed = 600.0f / (1.0f + activity / 100.0f);
  const float amp = 3.0f + (activity / 50.0f);  // More activity = bigger amplitude
  
  for (int x = 8; x < SCREEN_WIDTH - 8; x++) {
    float phase = (nowMs / speed) + (x * 0.08f);
    // Add harmonics for complexity
    float wave = sin(phase) + sin(phase * 2.0f) * 0.3f;
    int y = y0 + (int)(wave * amp);
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}

void drawInterferenceWave(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t progress) {
  const uint8_t y0 = 40;  // Centered in content area
  const float progressFactor = progress / 100.0f;
  
  for (int x = 8; x < SCREEN_WIDTH - 8; x++) {
    // Two waves at slightly different frequencies
    float phase1 = (nowMs / 70.0f) + (x * 0.25f);
    float phase2 = (nowMs / 85.0f) + (x * 0.20f);
    float wave1 = sin(phase1) * 4.0f;
    float wave2 = sin(phase2) * 3.0f * (1.0f - progressFactor);
    int y = y0 + (int)(wave1 + wave2);
    
    // Constrain to content area
    if (y >= CONTENT_Y_START && y <= CONTENT_Y_END) {
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }
}

void drawChaoticWave(Adafruit_SSD1306& display, uint32_t nowMs) {
  const uint8_t y0 = 40;
  int lastY = y0;
  
  for (int x = 8; x < SCREEN_WIDTH - 8; x++) {
    // Combine multiple frequencies for chaos
    float phase = (nowMs / 40.0f) + (x * 0.3f);
    float chaos = sin(phase) * 6.0f + sin(phase * 3.7f) * 3.0f + sin(phase * 7.2f) * 2.0f;
    int y = y0 + (int)chaos;
    
    // Constrain and draw line for continuity
    y = constrain(y, CONTENT_Y_START, CONTENT_Y_END);
    display.drawLine(x - 1, lastY, x, y, SSD1306_WHITE);
    lastY = y;
  }
}

// ======================= Visual Effects =======================

void drawRadarSweep(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t progress) {
  const int16_t centerX = SCREEN_WIDTH / 2;
  const int16_t centerY = 40;  // Center of content area
  const uint8_t radius = 15;
  
  // Draw outer circle
  display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
  display.drawCircle(centerX, centerY, radius / 2, SSD1306_WHITE);
  
  // Rotating sweep line
  float angle = (nowMs / 1000.0f) * 6.28318f;  // Full rotation every second
  int16_t x2 = centerX + (int16_t)(cos(angle) * radius);
  int16_t y2 = centerY + (int16_t)(sin(angle) * radius);
  display.drawLine(centerX, centerY, x2, y2, SSD1306_WHITE);
  
  // Progress dots around circle
  uint8_t dots = (progress * 8) / 100;
  for (uint8_t i = 0; i < dots; i++) {
    float dotAngle = (i * 6.28318f) / 8.0f;
    int16_t dx = centerX + (int16_t)(cos(dotAngle) * radius);
    int16_t dy = centerY + (int16_t)(sin(dotAngle) * radius);
    display.fillCircle(dx, dy, 1, SSD1306_WHITE);
  }
}

void drawReactorCore(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t heatPercent) {
  const int16_t centerX = SCREEN_WIDTH / 2;
  const int16_t centerY = 40;
  const uint8_t baseRadius = 8;
  
  // Pulsing based on heat
  float pulseFactor = 1.0f + (heatPercent / 400.0f) * sin(nowMs / 200.0f);
  uint8_t radius = (uint8_t)(baseRadius * pulseFactor);
  
  // Draw concentric circles
  display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
  display.drawCircle(centerX, centerY, radius / 2, SSD1306_WHITE);
  
  // Rotating control rods (4 lines)
  float angle = (nowMs / 800.0f);
  for (uint8_t i = 0; i < 4; i++) {
    float rodAngle = angle + (i * 1.57079632f);  // 90 degrees apart
    int16_t x1 = centerX + (int16_t)(cos(rodAngle) * (radius / 2));
    int16_t y1 = centerY + (int16_t)(sin(rodAngle) * (radius / 2));
    int16_t x2 = centerX + (int16_t)(cos(rodAngle) * radius);
    int16_t y2 = centerY + (int16_t)(sin(rodAngle) * radius);
    display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
  }
}

void drawSpinner(Adafruit_SSD1306& display, int16_t x, int16_t y, uint32_t nowMs) {
  const uint8_t radius = 3;
  float angle = (nowMs / 100.0f);
  int16_t x2 = x + (int16_t)(cos(angle) * radius);
  int16_t y2 = y + (int16_t)(sin(angle) * radius);
  display.drawLine(x, y, x2, y2, SSD1306_WHITE);
  display.drawPixel(x, y, SSD1306_WHITE);
}

void drawGeigerFlashes(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t intensity) {
  // Random flashes based on intensity
  uint8_t flashChance = intensity / 4;  // 0-25 range
  
  for (uint8_t i = 0; i < flashChance; i++) {
    if (random(100) < intensity) {
      int16_t x = 8 + random(SCREEN_WIDTH - 16);
      int16_t y = CONTENT_Y_START + random(CONTENT_HEIGHT);
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }
}

void drawBars(Adafruit_SSD1306& display, uint8_t startY, uint8_t height, uint32_t nowMs, uint8_t energy) {
  const uint8_t numBars = 8;
  const uint8_t barWidth = 3;
  const uint8_t spacing = (SCREEN_WIDTH - 16) / numBars;
  
  for (uint8_t i = 0; i < numBars; i++) {
    // Each bar oscillates at different phase
    float phase = (nowMs / 100.0f) + (i * 0.5f);
    uint8_t barHeight = (uint8_t)((sin(phase) * 0.5f + 0.5f) * height * energy / 100.0f);
    
    int16_t x = 8 + (i * spacing);
    int16_t y = startY + height - barHeight;
    
    display.fillRect(x, y, barWidth, barHeight, SSD1306_WHITE);
  }
}

// ======================= Screen Effects =======================

void drawPulsingBorder(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t intensity) {
  // Only draw if pulse is active
  float phase = (nowMs / 300.0f);
  if (sin(phase) * intensity > 50) {
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.drawRect(1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2, SSD1306_WHITE);
  }
}

void drawScanLines(Adafruit_SSD1306& display, uint32_t nowMs) {
  uint8_t offset = (nowMs / 100) % 4;
  for (uint8_t y = offset; y < SCREEN_HEIGHT; y += 4) {
    for (uint8_t x = 0; x < SCREEN_WIDTH; x += 2) {
      display.drawPixel(x, y, SSD1306_BLACK);
    }
  }
}

void drawCornerBrackets(Adafruit_SSD1306& display, uint8_t inset) {
  const uint8_t len = 8;
  // Top-left
  display.drawLine(inset, inset, inset + len, inset, SSD1306_WHITE);
  display.drawLine(inset, inset, inset, inset + len, SSD1306_WHITE);
  // Top-right
  display.drawLine(SCREEN_WIDTH - 1 - inset - len, inset, SCREEN_WIDTH - 1 - inset, inset, SSD1306_WHITE);
  display.drawLine(SCREEN_WIDTH - 1 - inset, inset, SCREEN_WIDTH - 1 - inset, inset + len, SSD1306_WHITE);
  // Bottom-left
  display.drawLine(inset, SCREEN_HEIGHT - 1 - inset, inset + len, SCREEN_HEIGHT - 1 - inset, SSD1306_WHITE);
  display.drawLine(inset, SCREEN_HEIGHT - 1 - inset - len, inset, SCREEN_HEIGHT - 1 - inset, SSD1306_WHITE);
  // Bottom-right
  display.drawLine(SCREEN_WIDTH - 1 - inset - len, SCREEN_HEIGHT - 1 - inset, SCREEN_WIDTH - 1 - inset, SCREEN_HEIGHT - 1 - inset, SSD1306_WHITE);
  display.drawLine(SCREEN_WIDTH - 1 - inset, SCREEN_HEIGHT - 1 - inset - len, SCREEN_WIDTH - 1 - inset, SCREEN_HEIGHT - 1 - inset, SSD1306_WHITE);
}

// ======================= Transitions =======================

void transitionWipe(Adafruit_SSD1306& display, uint8_t progress, bool leftToRight) {
  uint8_t wipeX = (SCREEN_WIDTH * progress) / 100;
  if (leftToRight) {
    display.fillRect(0, 0, wipeX, SCREEN_HEIGHT, SSD1306_BLACK);
  } else {
    display.fillRect(SCREEN_WIDTH - wipeX, 0, wipeX, SCREEN_HEIGHT, SSD1306_BLACK);
  }
}

void transitionFade(Adafruit_SSD1306& display, uint8_t progress) {
  // Dither pattern based on progress
  uint8_t threshold = (100 - progress) * 255 / 100;
  
  for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
    for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
      // Simple ordered dither pattern
      uint8_t pattern = ((x & 1) + (y & 1) * 2) * 64;
      if (pattern > threshold) {
        display.drawPixel(x, y, SSD1306_BLACK);
      }
    }
  }
}

// ======================= Utility =======================

void begin() {
  resetParticles();
}

void resetParticles() {
  for (uint8_t i = 0; i < MAX_PARTICLES; i++) {
    particles[i].life = 0;
  }
  lastParticleSpawn = millis();
}

void tick() {
  // Any per-frame updates that don't require display
  // Currently particles are updated during draw calls
}

} // namespace ReactorAnimations
