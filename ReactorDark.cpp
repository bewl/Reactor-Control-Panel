#include "ReactorDark.h"
#include "ReactorUI.h"
#include "ReactorHeat.h"

namespace ReactorDark {

// ======================= Timing Constants =======================
const unsigned long DARK_SUCCESS_DISPLAY_MS = 2000;

// ======================= State Variables =======================
unsigned long darkModeStartAt = 0;
bool          darkModeShowingSuccess = true;

// ======================= Pins =======================
const uint8_t PIN_LED_MELTDOWN      = 13;
const uint8_t PIN_LED_STABLE        = 12;
const uint8_t PIN_LED_STARTUP       = 11;
const uint8_t PIN_LED_FREEZEDOWN    = 9;

// ======================= API =======================
void begin() {
  pinMode(PIN_LED_MELTDOWN, OUTPUT);
  pinMode(PIN_LED_STABLE, OUTPUT);
  pinMode(PIN_LED_STARTUP, OUTPUT);
  pinMode(PIN_LED_FREEZEDOWN, OUTPUT);
  reset();
}

void reset() {
  darkModeStartAt = millis();
  darkModeShowingSuccess = false;
  
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  ReactorHeat::allOff();
  ReactorUI::display.clearDisplay();
  ReactorUI::display.display();
}

void enterDarkWithSuccess() {
  darkModeStartAt = millis();
  darkModeShowingSuccess = true;
  
  // Show success message
  ReactorUI::display.clearDisplay();
  ReactorUI::display.setTextSize(2);
  ReactorUI::display.setTextColor(SSD1306_WHITE);
  ReactorUI::display.setCursor(0, 24);
  ReactorUI::display.println("SHUTDOWN");
  ReactorUI::display.setCursor(12, 40);
  ReactorUI::display.println("SUCCESS");
  ReactorUI::display.display();
  
  // LEDs stay on momentarily (will turn off in tick)
}

void tick() {
  unsigned long now = millis();
  unsigned long elapsed = now - darkModeStartAt;
  
  // After showing success message, go completely dark
  if (darkModeShowingSuccess && elapsed >= DARK_SUCCESS_DISPLAY_MS) {
    darkModeShowingSuccess = false;
    
    // Turn everything off
    digitalWrite(PIN_LED_MELTDOWN, LOW);
    digitalWrite(PIN_LED_STABLE, LOW);
    digitalWrite(PIN_LED_STARTUP, LOW);
    digitalWrite(PIN_LED_FREEZEDOWN, LOW);
    
    // Turn off all heat bar LEDs
    ReactorHeat::allOff();
    
    // Clear and turn off display
    ReactorUI::display.clearDisplay();
    ReactorUI::display.display();
  }
  
  // Stay dark - only startup button will wake us up
}

} // namespace ReactorDark
