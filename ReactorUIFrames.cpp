#include "ReactorUIFrames.h"
#include "ReactorUI.h"
#include "ReactorHeat.h"
#include "ReactorAudio.h"
#include "ReactorEvents.h"
#include "ReactorSequences.h"
#include "ReactorStateMachine.h"

namespace ReactorUIFrames {

namespace {
  bool lastWarningShown = false;
  inline uint8_t currentHeatPercent() { return ReactorHeat::percent(); }
  inline bool isMuted() { return ReactorAudio::isMuted(); }
}

void drawCoreStatus(bool warning) {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.warning = warning;
  m.overheated = warning;
  if (warning) ReactorUI::ui.render(MODE_MELTDOWN, m, isMuted());
  else         ReactorUI::ui.render(MODE_STABLE, m, isMuted());
}

void drawCoreStatusForce(bool warning) {
  lastWarningShown = !warning; // force next draw
  drawCoreStatus(warning);
}

void drawCenteredBig(const char* txt, uint8_t size) {
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  ReactorUI::display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  int16_t x = ((int16_t)ReactorUI::display.width()  - (int16_t)w) / 2;
  int16_t y = ((int16_t)ReactorUI::display.height() - (int16_t)h) / 2;
  ReactorUI::display.setCursor(x, y);
  ReactorUI::display.print(txt);
  ReactorUI::display.display();
}

void renderStableUIFrame() {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();
  ReactorUI::ui.render(MODE_STABLE, m, isMuted());
}

void renderActiveUIFrame(Mode mode, unsigned long meltdownStartAt) {
  ReactorUI::UIMetrics m;
  m.heatPercent = currentHeatPercent();

  switch (mode) {
    case MODE_STABLE:
      // Only render background if no event, or render less frequently during events
      if (!ReactorEvents::isActive()) {
        ReactorUI::ui.render(MODE_STABLE, m, isMuted());
      } else {
        // During event, keep display static - only redraw event box
        static unsigned long lastEventDraw = 0;
        if (millis() - lastEventDraw > 500 || lastEventDraw == 0) {
          lastEventDraw = millis();
          ReactorUI::ui.render(MODE_STABLE, m, isMuted());
        }
        
        // Draw solid event box overlay
        int w = ReactorUI::display.width();
        ReactorUI::display.fillRect(4, 26, w-8, 32, SSD1306_BLACK);
        ReactorUI::display.drawRect(4, 26, w-8, 32, SSD1306_WHITE);
        ReactorUI::display.drawRect(5, 27, w-10, 30, SSD1306_WHITE);
        
        ReactorUI::display.setTextSize(1);
        ReactorUI::display.setTextColor(SSD1306_WHITE);
        
        // Center the event message
        ReactorUI::display.setCursor(10, 31);
        ReactorUI::display.println(ReactorEvents::getMessage());
        
        ReactorUI::display.setCursor(10, 42);
        ReactorUI::display.print("PRESS ");
        ReactorUI::display.println(ReactorEvents::getRequiredButtonName());
        
        ReactorUI::display.display();
      }
      break;

    case MODE_ARMING: {
      // Display 5-second countdown
      unsigned long now = millis();
      unsigned long elapsed = now - ReactorStateMachine::armingStartAt;
      long remaining = 5000 - (long)elapsed;
      if (remaining < 0) remaining = 0;
      
      int seconds = (remaining + 999) / 1000;  // Ceiling division to round up
      
      ReactorUI::display.setTextSize(3);
      ReactorUI::display.setTextColor(SSD1306_WHITE);
      char buf[4];
      snprintf(buf, sizeof(buf), "%d", seconds);
      int16_t x1, y1;
      uint16_t w, h;
      ReactorUI::display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
      int16_t x = (ReactorUI::display.width() - (int)w) / 2;
      int16_t y = 35;
      ReactorUI::display.setCursor(x, y);
      ReactorUI::display.println(buf);
      
      // Status text below
      ReactorUI::display.setTextSize(1);
      ReactorUI::display.setCursor(35, 56);
      ReactorUI::display.println("ARMING");
      
      // Animations
      ReactorAnimations::drawPulsingBorder(ReactorUI::display, now, 80);
      ReactorAnimations::drawCornerBrackets(ReactorUI::display, 2);
    } break;

    case MODE_STARTUP: {
      uint8_t step = ReactorSequences::getStep(MODE_STARTUP);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_STARTUP);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_STARTUP, m, isMuted());
    } break;

    case MODE_STABILIZING: {
      uint8_t step = ReactorSequences::getStep(MODE_STABILIZING);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_STABILIZING);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_STABILIZING, m, isMuted());
    } break;

    case MODE_FREEZEDOWN: {
      uint8_t step = ReactorSequences::getStep(MODE_FREEZEDOWN);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_FREEZEDOWN);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      m.freezing   = true;
      ReactorUI::ui.render(MODE_FREEZEDOWN, m, isMuted());
    } break;

    case MODE_SHUTDOWN: {
      uint8_t step = ReactorSequences::getStep(MODE_SHUTDOWN);
      uint8_t total = ReactorSequences::getTotalSteps(MODE_SHUTDOWN);
      m.progress = (total > 0) ? (uint8_t)(((step + 1) * 100) / total) : 0;
      ReactorUI::ui.render(MODE_SHUTDOWN, m, isMuted());
    } break;

    case MODE_MELTDOWN: {
      unsigned long now = millis();
      unsigned long meltdownElapsed = (now >= meltdownStartAt) ? (now - meltdownStartAt) : 0;
      long remain = 10000L - (long)meltdownElapsed;  // 10 second countdown
      if (remain < 0) remain = 0;
      
      int seconds = (remain + 999) / 1000;  // Ceiling division to round up
      
      // Clear and draw header bar
      ReactorUI::uiTopBar();
      ReactorUI::uiHeatBar();
      
      ReactorUI::display.setTextSize(3);
      ReactorUI::display.setTextColor(SSD1306_WHITE);
      char buf[4];
      snprintf(buf, sizeof(buf), "%d", seconds);
      int16_t x1, y1;
      uint16_t w, h;
      ReactorUI::display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
      int16_t x = (ReactorUI::display.width() - (int)w) / 2;
      int16_t y = 35;
      ReactorUI::display.setCursor(x, y);
      ReactorUI::display.println(buf);
      
      // Status text below
      ReactorUI::display.setTextSize(1);
      ReactorUI::display.setCursor(30, 56);
      ReactorUI::display.println("MELTDOWN");
      
      // Animations
      ReactorAnimations::drawMeltdownSparks(ReactorUI::display, now);
      ReactorAnimations::drawChaoticWave(ReactorUI::display, now);
      ReactorAnimations::drawPulsingBorder(ReactorUI::display, now, 100);
      ReactorAnimations::drawGeigerFlashes(ReactorUI::display, now, 90);
      
      ReactorUI::display.display();
    } break;

    case MODE_CHAOS:
      // Chaos renders elsewhere
      break;
  }
}

} // namespace ReactorUIFrames
