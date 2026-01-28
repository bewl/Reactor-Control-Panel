#include "ReactorUI.h"
#include "ReactorAnimations.h"

#include <Wire.h>
#include <math.h>

namespace ReactorUI {

// ======================= OLED =======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---- Layout constants ----
static const uint8_t UI_TOP_H = 10; // header height
static const uint8_t UI_BOT_H = 0;  // no footer
static const uint8_t UI_MID_H = 64 - UI_TOP_H - UI_BOT_H;

// ---- Glyphs ----
const uint8_t GLYPH_POWER[] PROGMEM = {
  B00011000, B00111100, B01111110, B11100111,
  B11100111, B01111110, B00111100, B00011000
};
const uint8_t GLYPH_WARN[] PROGMEM = {
  B00011000, B00011000, B00111100, B00111100,
  B00111100, B00000000, B00111100, B00111100
};
const uint8_t GLYPH_OVERHEAT[] PROGMEM = {
  B00011000, B00111100, B00100100, B00011000,
  B00011000, B00100100, B00111100, B00011000
};
const uint8_t GLYPH_FREEZE[] PROGMEM = {
  B00011000, B01011010, B00111100, B11111111,
  B00111100, B01011010, B00011000, B00011000
};
const uint8_t GLYPH_SWIRL[] PROGMEM = {
  B00000000, B00011100, B00100010, B01000010,
  B01000100, B00111000, B00000000, B00000000
};
const uint8_t GLYPH_MUTE[] PROGMEM = {
  B00011000,
  B00111100,
  B01111110,
  B01011010,
  B01111110,
  B00111100,
  B00011000,
  B01100110
};

// ---- Helpers ----
static inline void uiDrawIcon(int16_t x, int16_t y, const uint8_t* bmp8x8) {
  display.drawBitmap(x, y, bmp8x8, 8, 8, SSD1306_WHITE);
}

static inline void uiTextCentered(const char* s, int16_t y, uint8_t size=1) {
  int16_t x1,y1; uint16_t w,h;
  display.setTextSize(size);
  display.setTextColor(SSD1306_WHITE);
  display.getTextBounds((char*)s, 0, y, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - w) / 2;
  display.setCursor(x, y);
  display.print(s);
}

static inline uint8_t clampU8(int v) {
  if (v < 0) return 0; if (v > 100) return 100; return (uint8_t)v;
}

// ---- Bars & sections ----
static void uiTopBar(const char* modeLabel, const UIMetrics& m, bool muteActive) {
  display.drawLine(0, UI_TOP_H, SCREEN_WIDTH-1, UI_TOP_H, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 1);
  display.print(modeLabel);
  int16_t iconX = SCREEN_WIDTH - 10;

  if (muteActive)   { uiDrawIcon(iconX, 1, GLYPH_MUTE);    iconX -= 10; }
  if (m.freezing)   { uiDrawIcon(iconX, 1, GLYPH_FREEZE);  iconX -= 10; }
  if (m.overheated) { uiDrawIcon(iconX, 1, GLYPH_OVERHEAT);iconX -= 10; }
  if (m.warning)    { uiDrawIcon(iconX, 1, GLYPH_WARN);    iconX -= 10; }
}

static void uiHeatBar(const UIMetrics& m) {
  const uint8_t topY  = UI_TOP_H + 4;
  const uint8_t h     = 8;
  const uint8_t leftX = 8;
  const uint8_t rightX= SCREEN_WIDTH - 8;
  const uint8_t w     = rightX - leftX;

  display.drawRect(leftX, topY, w, h, SSD1306_WHITE);
  uint8_t fillW = (uint8_t)((w-2) * m.heatPercent / 100.0f);
  if (fillW > 0) display.fillRect(leftX+1, topY+1, fillW, h-2, SSD1306_WHITE);

  for (int i=0;i<=10;i++) {
    int x = leftX + (int)((w-2) * i / 10.0f) + 1;
    display.drawPixel(x, topY + h + 1, SSD1306_WHITE);
  }

  uiDrawIcon((SCREEN_WIDTH-8)/2, topY + h + 3, GLYPH_SWIRL);
}

static void uiStartupSteps(uint8_t progress) {
  struct Step { const char* label; uint8_t threshold; } steps[] = {
    {"IGNITION",        20},
    {"COOLANT FLOW",    60},
    {"REACTOR ONLINE", 100}
  };

  const uint8_t startY  = UI_TOP_H + 4 + 8 + 3 + 8;
  const uint8_t spacing = 12;
  uint8_t y = startY;

  for (int i = 0; i < 3; i++) {
    bool done = progress >= steps[i].threshold;
    if (done) display.fillCircle(8, y + 3, 3, SSD1306_WHITE);
    else      display.drawCircle(8, y + 3, 3, SSD1306_WHITE);

    display.setCursor(18, y);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print(steps[i].label);

    y += spacing;
  }
}

static void uiStablePulse(uint32_t tMs) {
  const uint8_t left   = 8;
  const uint8_t right  = SCREEN_WIDTH - 8;
  const uint8_t width  = right - left;
  const uint8_t baseY  = SCREEN_HEIGHT - 18;
  const float   speed  = 600.0f;
  const float   kx     = 0.08f;
  const float   amp    = 3.0f;

  for (int x = 0; x < width; x++) {
    float phase = (tMs / speed) + (x * kx);
    int y = baseY + (int)(sin(phase) * amp);
    display.drawPixel(left + x, y, SSD1306_WHITE);
  }
}

static void uiStableStatusText() {
  const char* label = "CORE STABLE";
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds((char*)label, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - (int)w) / 2;
  int16_t y = SCREEN_HEIGHT - 10;
  display.setCursor(x, y);
  display.print(label);
}

static void uiStabilizingWave(uint32_t tMs, uint8_t progress) {
  const uint8_t midTop = UI_TOP_H + 4 + 8 + 3 + 8;
  uint8_t y0 = midTop + 16;
  for (int x=8; x<SCREEN_WIDTH-8; x++) {
    float phase = (tMs/70.0f) + (x*0.25f);
    int y = y0 + (int)(sin(phase) * 6.0);
    display.drawPixel(x, y, SSD1306_WHITE);
  }
  const uint8_t pbY = SCREEN_HEIGHT - 6;
  display.drawLine(8, pbY, 8 + (int)((SCREEN_WIDTH-16) * progress / 100.0f), pbY, SSD1306_WHITE);
}

static void uiMeltdownCountdown(const UIMetrics& m) {
  char buf[8];
  uint32_t s = (uint32_t)((m.countdownMs + 999) / 1000);
  snprintf(buf, sizeof(buf), "%lus", (unsigned long)s);
  const uint8_t topY = UI_TOP_H + 4 + 8 + 3 + 6;
  uiTextCentered(buf, topY + 10, 2);
}

static void uiArmingNumber(uint8_t n) {
  display.setTextSize((n < 10) ? 3 : 2);
  display.setTextColor(SSD1306_WHITE);
  char buff[6];
  snprintf(buff, sizeof(buff), "%u", n);
  const uint8_t yBody = UI_TOP_H + 4 + 8 + 3 + 8;
  uiTextCentered(buff, yBody + 10, (n<10)?3:2);
}

static uint8_t breathHeatPercent(uint8_t basePercent, uint32_t nowMs) {
  const float twoPi = 6.28318530718f;
  float phase = (nowMs % 2600) / 2600.0f;
  float s = sinf(phase * twoPi);
  int breathed = (int)roundf((float)basePercent + 4.0f * s);
  return clampU8(breathed);
}

// ---- Renderer ----
Renderer ui;

bool begin() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    return false;
  }
  ReactorAnimations::begin();
  display.clearDisplay();
  display.display();
  return true;
}

void Renderer::render(Mode mMode, const UIMetrics& m, bool muteActive) {
  const uint32_t now = millis();
  if (mMode == MODE_CHAOS) return;

  display.clearDisplay();

  switch (mMode) {
    case MODE_STABLE:      uiTopBar("STABLE",      m, muteActive); break;
    case MODE_ARMING:      uiTopBar("ARMING",      m, muteActive); break;
    case MODE_MELTDOWN:    uiTopBar("MELTDOWN",    m, muteActive); break;
    case MODE_STABILIZING: uiTopBar("STABILIZING", m, muteActive); break;
    case MODE_STARTUP:     uiTopBar("STARTUP",     m, muteActive); break;
    case MODE_FREEZEDOWN:  uiTopBar("FREEZEDOWN",  m, muteActive); break;
    case MODE_SHUTDOWN:    uiTopBar("SHUTDOWN",    m, muteActive); break;
    case MODE_DARK:        uiTopBar("DARK",        m, muteActive); break;
    case MODE_CHAOS:       break;
  }

  UIMetrics heatM = m;
  if (mMode == MODE_STABLE)        heatM.heatPercent = breathHeatPercent(m.heatPercent, now);
  else if (mMode == MODE_MELTDOWN) heatM.heatPercent = 100;
  uiHeatBar(heatM);

  switch (mMode) {
    case MODE_STABLE: {
      // Draw reactor core centerpiece with decay particles
      ReactorAnimations::drawReactorCore(display, now, m.heatPercent);
      ReactorAnimations::drawDecayParticles(display, now);
      uiStableStatusText();
      if (((now/750) % 2) == 0) uiDrawIcon(4, UI_TOP_H + 2, GLYPH_POWER);
      // Add subtle Geiger clicks
      ReactorAnimations::drawGeigerFlashes(display, now, m.heatPercent / 5);
    } break;

    case MODE_ARMING: {
      // Large arming number with pulsing border
      uiArmingNumber((uint8_t)constrain((m.progress>0)?m.progress:0, 0, 99));
      ReactorAnimations::drawPulsingBorder(display, now, 80);
      // Bottom corner brackets for intensity
      ReactorAnimations::drawCornerBrackets(display, 2);
    } break;

    case MODE_MELTDOWN: {
      // Explosive sparks everywhere + chaotic wave
      uiMeltdownCountdown(m);
      ReactorAnimations::drawMeltdownSparks(display, now);
      ReactorAnimations::drawChaoticWave(display, now);
      if (((now/200) % 2) == 0) uiDrawIcon(4, UI_TOP_H + 2, GLYPH_WARN);
      // Pulsing danger border
      ReactorAnimations::drawPulsingBorder(display, now, 100);
      // Intense Geiger flashing
      ReactorAnimations::drawGeigerFlashes(display, now, 90);
    } break;

    case MODE_STABILIZING: {
      // Interference wave showing stabilization convergence
      ReactorAnimations::drawInterferenceWave(display, now, m.progress);
      ReactorAnimations::drawCoolantFlow(display, now);
      // Progress bar at bottom
      const uint8_t pbY = SCREEN_HEIGHT - 6;
      display.drawLine(8, pbY, 8 + (int)((SCREEN_WIDTH-16) * m.progress / 100.0f), pbY, SSD1306_WHITE);
      // Spinning indicator in corner
      ReactorAnimations::drawSpinner(display, SCREEN_WIDTH - 12, 15, now);
    } break;

    case MODE_STARTUP: {
      // Radar sweep with progress indicators
      ReactorAnimations::drawRadarSweep(display, now, m.progress);
      // Steps text at bottom
      uint8_t step = (m.progress * 5) / 100;
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(35, 56);
      display.print("STEP ");
      display.print(step);
      display.print("/5");
    } break;

    case MODE_FREEZEDOWN: {
      // Snowflake particles with status text
      ReactorAnimations::drawFreezeParticles(display, now);
      const uint8_t midTop = UI_TOP_H + 4 + 8 + 3 + 8;
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      int16_t x1, y1; uint16_t w, h;
      display.getTextBounds("Core freezing", 0, 0, &x1, &y1, &w, &h);
      display.setCursor((SCREEN_WIDTH - w) / 2, midTop + 8);
      display.println("Core freezing");
      // Bottom progress bar
      const uint8_t pbY = SCREEN_HEIGHT - 6;
      display.drawLine(8, pbY, 8 + (int)((SCREEN_WIDTH-16) * m.progress / 100.0f), pbY, SSD1306_WHITE);
      // Spinning frost effect
      ReactorAnimations::drawSpinner(display, SCREEN_WIDTH - 12, 15, now);
    } break;

    case MODE_SHUTDOWN: {
      // Energy bars winding down with chaotic wave fading
      ReactorAnimations::drawBars(display, 30, 18, now, 100 - m.progress);
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      int16_t x1, y1; uint16_t w, h;
      const char* label = "Powering down";
      display.getTextBounds((char*)label, 0, 0, &x1, &y1, &w, &h);
      display.setCursor((SCREEN_WIDTH - w) / 2, 24);
      display.println(label);
      // Progress bar
      const uint8_t pbY = SCREEN_HEIGHT - 6;
      display.drawLine(8, pbY, 8 + (int)((SCREEN_WIDTH-16) * m.progress / 100.0f), pbY, SSD1306_WHITE);
    } break;

    case MODE_DARK:
    case MODE_CHAOS:
      break;
  }

  // Optional: add subtle scan lines for retro effect (can disable if too intense)
  // ReactorAnimations::drawScanLines(display, now);

  display.display();
}

} // namespace ReactorUI
