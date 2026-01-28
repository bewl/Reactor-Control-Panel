#pragma once

#include <Arduino.h>

namespace ReactorEvents {

void begin();
void tick();
void trigger();
void resolve();
void fail();

// Returns true if input was consumed by an active event
bool handleInput(bool overrideFell, bool stabilizeFell, bool startupFell,
				 bool freezedownFell, bool shutdownFell, bool eventFell);

bool isActive();
const char* getMessage();
const char* getRequiredButtonName();
char getRequiredButton();

} // namespace ReactorEvents
