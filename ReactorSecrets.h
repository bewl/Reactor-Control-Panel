#pragma once

#include <Arduino.h>

namespace ReactorSecrets {

void begin();
void captureInput(char code);
bool isGodMode();
bool isCryoLocked();
void tick();

} // namespace ReactorSecrets
