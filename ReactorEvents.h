#pragma once

#include <Arduino.h>

namespace ReactorEvents {

void begin();
void tick();
void trigger();
void resolve();
void fail();

bool isActive();
const char* getMessage();
const char* getRequiredButtonName();
char getRequiredButton();

} // namespace ReactorEvents
