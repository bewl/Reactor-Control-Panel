#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ReactorTypes.h"

namespace ReactorUI {

// Screen metrics used by the renderer
struct UIMetrics {
  uint8_t  heatPercent = 0;   // 0..100
  int32_t  countdownMs = -1;  // >=0 shows T-xxs (rendered in body)
  uint8_t  progress    = 0;   // 0..100
  bool     warning     = false;
  bool     overheated  = false;
  bool     freezing    = false;
};

// Renderer facade used by the state machine
class Renderer {
public:
  void render(Mode mMode, const UIMetrics& m, bool muteActive);
};

// Accessors
bool begin();
extern Renderer ui;
extern Adafruit_SSD1306 display;

} // namespace ReactorUI
