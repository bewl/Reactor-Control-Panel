#pragma once

#include <Adafruit_SSD1306.h>

namespace ReactorAnimations {

// All animations respect the UI layout:
// - Top bar: y=0-10 (header + divider line)
// - Heat section: y=11-26 (bar + ticks + icon)
// - Content area: y=27-54 (main animations)
// - Bottom area: y=55-64 (status/progress)

// ======================= Particle System =======================
struct Particle {
  float x, y;      // Position
  float vx, vy;    // Velocity
  uint8_t life;    // Remaining frames (0 = dead)
  uint8_t maxLife; // For fade calculations
};

const uint8_t MAX_PARTICLES = 16;

// Radioactive decay particles (rising from center)
void drawDecayParticles(Adafruit_SSD1306& display, uint32_t nowMs);

// Coolant droplets (falling particles)
void drawCoolantFlow(Adafruit_SSD1306& display, uint32_t nowMs);

// Sparks during meltdown (explosive particles)
void drawMeltdownSparks(Adafruit_SSD1306& display, uint32_t nowMs);

// Snowflakes during freezedown
void drawFreezeParticles(Adafruit_SSD1306& display, uint32_t nowMs);

// ======================= Waveforms =======================

// Enhanced pulse wave with complexity based on activity level
void drawEnhancedPulse(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t activity);

// Dual-wave interference pattern (for stabilizing)
void drawInterferenceWave(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t progress);

// Chaotic jagged wave (for critical states)
void drawChaoticWave(Adafruit_SSD1306& display, uint32_t nowMs);

// ======================= Visual Effects =======================

// Circular radar sweep (for startup/scanning)
// Draws in content area: y=30-50
void drawRadarSweep(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t progress);

// Rotating reactor core visualization
// Draws in content area centered at y=40
void drawReactorCore(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t heatPercent);

// Progress spinner (small, for top corner)
void drawSpinner(Adafruit_SSD1306& display, int16_t x, int16_t y, uint32_t nowMs);

// Geiger counter visual (random flashes)
void drawGeigerFlashes(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t intensity);

// Vertical bars equalizer (for audio visualization concept)
// Draws in specified region
void drawBars(Adafruit_SSD1306& display, uint8_t startY, uint8_t height, uint32_t nowMs, uint8_t energy);

// ======================= Screen Effects =======================

// Border pulse effect (danger indicator)
void drawPulsingBorder(Adafruit_SSD1306& display, uint32_t nowMs, uint8_t intensity);

// Scan lines (retro CRT effect)
void drawScanLines(Adafruit_SSD1306& display, uint32_t nowMs);

// Corner brackets (framing effect)
void drawCornerBrackets(Adafruit_SSD1306& display, uint8_t inset);

// ======================= Transitions =======================

// Wipe transition (for mode changes)
void transitionWipe(Adafruit_SSD1306& display, uint8_t progress, bool leftToRight);

// Fade effect (checkerboard dither based on progress)
void transitionFade(Adafruit_SSD1306& display, uint8_t progress);

// ======================= Utility =======================

// Initialize animation system (seed random, reset states)
void begin();

// Reset all particle systems
void resetParticles();

// Tick function for any background animation state updates
void tick();

} // namespace ReactorAnimations
