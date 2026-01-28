#include "ReactorEvents.h"
#include "ReactorAudio.h"
#include "ReactorUI.h"

namespace ReactorEvents {

namespace {
  enum EventType : uint8_t {
    EVENT_NONE,
    EVENT_COOLANT_LEAK,
    EVENT_PRESSURE_SPIKE,
    EVENT_SENSOR_FAULT,
    EVENT_CONTROL_ROD_JAM
  };

  EventType activeEvent = EVENT_NONE;
  char requiredButton = 0;  // 'O', 'S', 'U', 'F', 'D', 'E'
  unsigned long eventStartAt = 0;
  
  const unsigned long EVENT_TIMEOUT_MS = 8000;  // 8 seconds to respond
  const unsigned long EVENT_ALARM_PERIOD_MS = 300;
  const int EVENT_ALARM_LOW_HZ = 900;
  const int EVENT_ALARM_HIGH_HZ = 1400;
  const unsigned long EVENT_LED_BLINK_MS = 200;
  
  unsigned long eventAlarmAt = 0;
  bool eventAlarmHigh = false;
  unsigned long eventLedBlinkAt = 0;
  bool eventLedOn = false;
}

const char* getMessage() {
  switch (activeEvent) {
    case EVENT_COOLANT_LEAK:    return "COOLANT LEAK!";
    case EVENT_PRESSURE_SPIKE:  return "PRESSURE SPIKE!";
    case EVENT_SENSOR_FAULT:    return "SENSOR FAULT!";
    case EVENT_CONTROL_ROD_JAM: return "ROD JAM!";
    default: return "";
  }
}

const char* getRequiredButtonName() {
  switch (requiredButton) {
    case 'O': return "OVERRIDE";
    case 'S': return "STABILIZE";
    case 'U': return "STARTUP";
    case 'F': return "FREEZE";
    case 'D': return "SHUTDOWN";
    case 'E': return "EVENT";
    default: return "???";
  }
}

char getRequiredButton() {
  return requiredButton;
}

bool isActive() {
  return activeEvent != EVENT_NONE;
}

bool handleInput(bool overrideFell, bool stabilizeFell, bool startupFell,
                 bool freezedownFell, bool shutdownFell, bool eventFell) {
  if (activeEvent == EVENT_NONE) return false;

  char pressed = 0;
  if (overrideFell)   pressed = 'O';
  if (stabilizeFell)  pressed = 'S';
  if (startupFell)    pressed = 'U';
  if (freezedownFell) pressed = 'F';
  if (shutdownFell)   pressed = 'D';
  if (eventFell)      pressed = 'E';

  if (pressed == 0) return false;

  if (pressed == requiredButton) {
    resolve();
    return true;
  }

  // Wrong button pressed during event - consume input
  return true;
}

void begin() {
  activeEvent = EVENT_NONE;
  requiredButton = 0;
  eventStartAt = 0;
  eventAlarmAt = millis();
  eventAlarmHigh = false;
  eventLedBlinkAt = millis();
  eventLedOn = false;
}

void trigger() {
  // Pick a random event type
  EventType events[] = {
    EVENT_COOLANT_LEAK,
    EVENT_PRESSURE_SPIKE,
    EVENT_SENSOR_FAULT,
    EVENT_CONTROL_ROD_JAM
  };
  activeEvent = events[random(4)];
  
  // Assign a random button
  char buttons[] = {'O', 'S', 'U', 'F', 'D', 'E'};
  requiredButton = buttons[random(6)];
  
  eventStartAt = millis();
  eventAlarmAt = millis();
  eventAlarmHigh = false;
  eventLedBlinkAt = millis();
  eventLedOn = false;
  
  // Brief alarm chirp
  ReactorAudio::toneHz(1200);
  delay(100);
  ReactorAudio::off();
}

void resolve() {
  activeEvent = EVENT_NONE;
  requiredButton = 0;
  digitalWrite(13, LOW);  // Turn off meltdown LED (PIN_LED_MELTDOWN = 13)
  
  // Success tone
  ReactorAudio::toneHz(1600);
  delay(80);
  ReactorAudio::toneHz(1800);
  delay(80);
  ReactorAudio::off();
  
  // Brief success message
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextSize(2);
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setCursor(20, 24);
  ReactorUI::display.println("EVENT");
  ReactorUI::display.setCursor(12, 42);
  ReactorUI::display.println("RESOLVED");
  ReactorUI::display.display();
  delay(600);
}

void fail() {
  activeEvent = EVENT_NONE;
  requiredButton = 0;
  digitalWrite(13, LOW);  // Turn off meltdown LED
  
  // Warning tone
  ReactorAudio::toneHz(800);
  delay(150);
  ReactorAudio::off();
  
  // Brief failure message
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextSize(2);
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setCursor(28, 24);
  ReactorUI::display.println("EVENT");
  ReactorUI::display.setCursor(24, 42);
  ReactorUI::display.println("FAILED!");
  ReactorUI::display.display();
  delay(600);
}

void tick() {
  unsigned long now = millis();
  
  // If event is active, check for timeout
  if (activeEvent != EVENT_NONE) {
    // Play alternating alarm tone
    if (now - eventAlarmAt >= EVENT_ALARM_PERIOD_MS) {
      eventAlarmAt = now;
      eventAlarmHigh = !eventAlarmHigh;
      ReactorAudio::toneHz(eventAlarmHigh ? EVENT_ALARM_HIGH_HZ : EVENT_ALARM_LOW_HZ);
    }
    
    // Blink the meltdown LED
    if (now - eventLedBlinkAt >= EVENT_LED_BLINK_MS) {
      eventLedBlinkAt = now;
      eventLedOn = !eventLedOn;
      digitalWrite(13, eventLedOn ? HIGH : LOW);  // PIN_LED_MELTDOWN = 13
    }
    
    if (now - eventStartAt >= EVENT_TIMEOUT_MS) {
      fail();
    }
  }
}

} // namespace ReactorEvents
