#pragma once

#include <Arduino.h>

namespace ReactorButtons {

struct Button {
  uint8_t pin;
  bool stableState;
  bool lastRaw;
  unsigned long changedAt;
  bool fellEvent;
  bool roseEvent;

  void begin(uint8_t p);
  void update();
  bool fell();
  bool rose();
  bool isPressed() const;
};

extern Button overrideBtn;
extern Button stabilizeBtn;
extern Button startupBtn;
extern Button freezedownBtn;
extern Button shutdownBtn;
extern Button eventBtn;
extern Button ackBtn;

void begin();
void update();

} // namespace ReactorButtons
