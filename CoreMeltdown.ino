struct UIMetrics;

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <math.h>

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

// ======================= OLED =======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ======================= Pins =======================
const uint8_t PIN_BUTTON_OVERRIDE   = 2;
const uint8_t PIN_BUTTON_STABILIZE  = 5;
const uint8_t PIN_BUTTON_STARTUP    = 10;
const uint8_t PIN_BUTTON_FREEZEDOWN = 8;
const uint8_t PIN_BUTTON_SHUTDOWN   = 6;
const uint8_t PIN_BUTTON_EVENT      = 3;
const uint8_t PIN_LED_MELTDOWN      = 13;
const uint8_t PIN_LED_STABLE        = 12;
const uint8_t PIN_LED_STARTUP       = 11;
const uint8_t PIN_LED_FREEZEDOWN    = 9;
const uint8_t PIN_BUZZER            = 7;
const uint8_t PIN_BUTTON_ACK        = 4;

// ===== Heat Status Bar (12 discrete LEDs on Mega pins 22..33) =====
const uint8_t HEAT_COUNT = 12;
const uint8_t HEAT_PINS[HEAT_COUNT] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33
};
// If LEDs are wired pin->resistor->LED->GND, keep true (HIGH = on).
// If wired to +5V, set to false (LOW = on).
const bool HEAT_ACTIVE_HIGH = true;

// Heat bar smoothing cadence & slew rate
const unsigned long HEAT_TICK_MS = 40;       // ~25 FPS
const float         HEAT_SLEW_LVL_PER_S = 8; // levels/sec (0..12)
float         heatValue  = 2.0f;  // current displayed level (0..12, fractional ok)
float         heatTarget = 2.0f;  // desired level based on mode
unsigned long heatTickAt = 0;     // last update time

// ======================= Tunables =======================
// Debounce
const unsigned long DEBOUNCE_MS = 50;

// Arming (3-2-1)
const unsigned long ARM_STEP_MS   = 500;
const uint8_t       ARM_BLINKS    = 5;
const unsigned int  ARM_CHIRP_HZ  = 1600;

// Meltdown
const unsigned long MELTDOWN_BLINK_MS = 125;
const unsigned int  MELTDOWN_TONE_HZ  = 1000;
// NEW: meltdown countdown to CHAOS
const unsigned long MELTDOWN_COUNTDOWN_MS = 10000; // 10 seconds

// Freezedown Sequence
const unsigned long FREEZE_STEP_MS        = 1200;
const uint8_t       FREEZE_TOTAL_STEPS    = 5;
const unsigned long FREEZE_LED_PERIOD_MS  = 1200;

// Freezedown alarm (slow, cooling “wah-wah”)
const unsigned long FREEZE_ALARM_PERIOD_MS = 500;
const int           FREEZE_ALARM_HIGH_HZ   = 650;
const int           FREEZE_ALARM_LOW_HZ    = 350;

// Stabilization sequence
const unsigned long STAB_STEP_MS       = 1000;
const uint8_t       STAB_TOTAL_STEPS   = 5;
const unsigned long STAB_LED_PERIOD_MS = 1000;

// Stabilization alarm sound (alternating siren)
const unsigned long STAB_ALARM_PERIOD_MS = 400;
const int           STAB_ALARM_LOW_HZ    = 800;
const int           STAB_ALARM_HIGH_HZ   = 1000;

// Shutdown sweep (AFTER stabilization completes)
const unsigned long SWEEP_MS     = 1000;
const int           SWEEP_F0_HZ  = 1800;
const int           SWEEP_F1_HZ  = 140;

// Startup sequence (multi-step + rising pitch, then auto → Stabilizing)
const unsigned long STARTUP_STEP_MS       = 2000;
const uint8_t       STARTUP_TOTAL_STEPS   = 5;
const unsigned long STARTUP_LED_PERIOD_MS = 400;
const int           STARTUP_F0_HZ         = 300;
const int           STARTUP_F1_HZ         = 1600;

// Shutdown sequence (multi-step + falling pitch, then auto → Stable)
const unsigned long SHUTDOWN_STEP_MS       = 2000;
const uint8_t       SHUTDOWN_TOTAL_STEPS   = 5;
const unsigned long SHUTDOWN_LED_PERIOD_MS = 800;
const int           SHUTDOWN_F0_HZ         = 1400;
const int           SHUTDOWN_F1_HZ         = 200;

// Dark mode (post-shutdown)
const unsigned long DARK_SUCCESS_DISPLAY_MS = 2000; // Show success for 2 seconds

// Event alarm sound (urgent alternating tone)
const unsigned long EVENT_ALARM_PERIOD_MS = 300;
const int           EVENT_ALARM_LOW_HZ    = 900;
const int           EVENT_ALARM_HIGH_HZ   = 1400;

// Stable "breathing" animation
const uint16_t STABLE_BREATH_MS   = 2600; // full inhale+exhale period
const uint8_t  STABLE_BREATH_AMPL = 4;    // +/- percent swing (keep small)
const uint16_t UI_FRAME_MS = 100;   // ~10 FPS
unsigned long  uiFrameAt   = 0;

// STABLE UI refresh
const uint16_t STABLE_UI_FRAME_MS = 100;  // ~10 FPS
unsigned long  stableUiAt = 0;

// Timed mute window
const unsigned long ACK_SILENCE_MS = 8000; // 8 seconds
unsigned long g_muteUntil = 0;

// ===== Buzzer control (centralized, mute-aware) =====
unsigned int  g_toneHz = 0;
bool          g_toneOn = false;

inline bool isMuted() {
  return g_muteUntil && (millis() < g_muteUntil);
}

inline void buzzerOff() {
  if (g_toneOn) {
    noTone(PIN_BUZZER);
    g_toneOn = false;
    g_toneHz = 0;
  }
}

inline void buzzerTone(unsigned int hz) {
  if (isMuted()) return;              // never start while muted
  if (hz == 0) { buzzerOff(); return; }
  if (g_toneOn && g_toneHz == hz) return; // avoid restart pops
  tone(PIN_BUZZER, hz);
  g_toneOn = true;
  g_toneHz = hz;
}

// ======================= State Machine =======================
enum Mode : uint8_t {
  MODE_STABLE, MODE_ARMING, MODE_MELTDOWN, MODE_STABILIZING, MODE_STARTUP, MODE_FREEZEDOWN, MODE_SHUTDOWN, MODE_DARK, MODE_CHAOS
};
Mode mode = MODE_STABLE;

// Arming state
uint8_t       armStep = 0;
unsigned long armTickAt = 0;

// Meltdown state
unsigned long meltdownTickAt = 0;
bool          meltdownPhase  = false;
unsigned long meltdownStart  = 0;  // when countdown began
int           lastCountdownSec = -1;
unsigned long lastCountdownDrawAt = 0;

// Stabilizing state
uint8_t       stabStep = 0;
unsigned long stabStepAt = 0;
unsigned long stabLedAt  = 0;
bool          stabLedOn  = false;
int           lastShownStabStep = -1;

// Stabilization alarm state
unsigned long stabAlarmAt = 0;
bool          stabAlarmHigh = false;

// Freezedown state
uint8_t       freezeStep    = 0;
unsigned long freezeStepAt  = 0;
unsigned long freezeLedAt   = 0;
bool          freezeLedOn   = false;
int           lastShownFreezeStep = -1;

// Freezedown alarm state
unsigned long freezeAlarmAt = 0;
bool          freezeAlarmHigh = false;

// Sweep state
bool          sweepActive = false;
unsigned long sweepStart  = 0;

// Startup state
uint8_t       startupStep     = 0;
unsigned long startupStepAt   = 0;
unsigned long startupStart    = 0;
unsigned long startupBlinkAt  = 0;
bool          startupLedOn    = false;
int           lastShownStartupStep = -1;

// Shutdown state
uint8_t       shutdownStep     = 0;
unsigned long shutdownStepAt   = 0;
unsigned long shutdownStart    = 0;
int           lastShownShutdownStep = -1;

// Dark mode state (post-shutdown)
unsigned long darkModeStartAt = 0;
bool          darkModeShowingSuccess = true;

// Random event system
enum EventType : uint8_t {
  EVENT_NONE,
  EVENT_COOLANT_LEAK,
  EVENT_PRESSURE_SPIKE,
  EVENT_SENSOR_FAULT,
  EVENT_CONTROL_ROD_JAM
};
EventType     activeEvent = EVENT_NONE;
char          requiredButton = 0;  // 'O', 'S', 'U', 'F', 'D'
unsigned long eventStartAt = 0;
const unsigned long EVENT_TIMEOUT_MS = 8000;  // 8 seconds to respond
unsigned long eventAlarmAt = 0;
bool          eventAlarmHigh = false;
unsigned long eventLedBlinkAt = 0;
bool          eventLedOn = false;
const unsigned long EVENT_LED_BLINK_MS = 200;  // Fast blink

// CHAOS state (post-explosion)
unsigned long chaosTickAt = 0;
unsigned long chaosInvertAt = 0;

// OLED caching
bool lastWarningShown = false;

// ======================= Debounced Buttons =======================
struct Button {
  uint8_t pin;
  bool stableState;
  bool lastRaw;
  unsigned long changedAt;
  bool fellEvent;
  bool roseEvent;

  void begin(uint8_t p) {
    pin = p;
    pinMode(pin, INPUT_PULLUP);
    bool r = digitalRead(pin);
    stableState = r;
    lastRaw = r;
    changedAt = 0;
    fellEvent = roseEvent = false;
  }

  void update() {
    bool raw = digitalRead(pin);
    unsigned long now = millis();
    if (raw != lastRaw) {
      lastRaw = raw;
      changedAt = now;
    }
    if (now - changedAt > DEBOUNCE_MS) {
      if (raw != stableState) {
        stableState = raw;
        if (stableState == LOW) fellEvent = true; else roseEvent = true;
      }
    }
  }

  bool fell() { bool e = fellEvent; fellEvent = false; return e; }
  bool rose() { bool e = roseEvent; roseEvent = false; return e; }
  bool isPressed() const { return stableState == LOW; }
};

Button btnOverride;
Button btnStabilize;
Button btnStartup;
Button btnFreezedown;
Button btnShutdown;
Button btnEvent;
Button btnAck;

// ======================= Secret Override System =======================
// Sequences (buttons encoded as 'O','S','U','F')
const uint8_t SEQ_MAX = 10;
char          seqBuffer[SEQ_MAX];
uint8_t       seqLength = 0;
unsigned long seqLastInput = 0;
const unsigned long SEQ_TIMEOUT_MS = 3000; // 3s between presses

// Secret modes
bool          g_godMode = false;          // Blocks any meltdown attempts
unsigned long g_cryoUntil = 0;            // Cryo lockdown active until time
const unsigned long CRYO_LOCK_MS = 12000; // 12s of heavy cooling

// ======================= Forward Decls =======================
void enterStable();
void enterArming();
void enterMeltdown();
void enterStabilizing();
void enterStartup();
void enterFreezedown();
void enterShutdown();
void enterDark();
void enterChaos();

void abortStabilizingToMeltdown();
void finishStabilizingToStable();
void finishFreezedownToStable();

void tickArming();
void tickMeltdown();
void tickStabilizing();
void tickStartup();
void tickFreezedown();
void tickShutdown();
void tickDark();
void tickChaos();
void tickSweep();

void drawCoreStatus(bool warning);
void drawCoreStatusForce(bool warning);
void drawCenteredBig(const char* txt, uint8_t size);
void drawArmingNumber(uint8_t n);
void drawStabilizingStep(uint8_t step);
void drawStartupStep(uint8_t step);
void drawFreezedownStep(uint8_t step);
void drawShutdownStep(uint8_t step);
void drawMeltdownCountdown(unsigned long msRemaining);

void stopSweepIfAny();

// Heat bar helpers
void updateHeatTargetForMode();
void tickHeatBar();
inline void heatWrite(uint8_t idx, bool on);

// Secret helpers
void checkSecretSequence();
void secretToneSweep();
void enterGodMode();
void enterCryoLockdown();

// Event system
void tickRandomEvents();
void triggerRandomEvent();
void resolveEvent();
void failEvent();
const char* getEventMessage();
const char* getButtonName(char btn);

// ======================= Messages =======================
const char* const STAB_MSGS[STAB_TOTAL_STEPS] = {
  "Inserting control rods",
  "Coolant flow increasing",
  "Pressure equalizing",
  "Containment securing",
  "Calibrating sensors"
};

const char* const STARTUP_MSGS[STARTUP_TOTAL_STEPS] = {
  "Evacuate chamber",
  "Seal access hatches",
  "Charge pre-heaters",
  "Spin aux pumps",
  "Diagnostics ready"
};

const char* const FREEZE_MSGS[FREEZE_TOTAL_STEPS] = {
  "Cryo coolant engaged",
  "Thermal siphons active",
  "Lattice contraction",
  "Containment frost check",
  "Core hibernation"
};

const char* const SHUTDOWN_MSGS[STAB_TOTAL_STEPS] = {
  "Divert plasma flow",
  "Drain coolant system",
  "Retract control rods",
  "Vent reactor chamber",
  "Power systems offline"
};

// =====================================================================
// =================== CLEAN OLED UI MODULE (fixed order) ===============
// =====================================================================

// ---- 1) Types FIRST ----
static const uint8_t UI_TOP_H = 10; // header height
static const uint8_t UI_BOT_H = 0;  // no footer anymore
static const uint8_t UI_MID_H = 64 - UI_TOP_H - UI_BOT_H;

struct UIMetrics {
  uint8_t  heatPercent = 0;   // 0..100
  int32_t  countdownMs = -1;  // >=0 shows T-xxs (rendered in body)
  uint8_t  progress    = 0;   // 0..100
  bool     warning     = false;
  bool     overheated  = false;
  bool     freezing    = false;
};

// ---- 2) Glyphs ----
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
// New: 8x8 "mute" icon (speaker with slash)
const uint8_t GLYPH_MUTE[] PROGMEM = {
  B00011000, //  ..XX....
  B00111100, //  .XXXX...
  B01111110, //  XXXXXX..
  B01011010, //  X.XX.X..
  B01111110, //  XXXXXX..
  B00111100, //  .XXXX...
  B00011000, //  ..XX....
  B01100110  //  XX..XX..
};

// ---- 3) Small helpers ----
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

// ---- 4) Bars & sections ----
static void uiTopBar(const char* modeLabel, const UIMetrics& m) {
  display.drawLine(0, UI_TOP_H, SCREEN_WIDTH-1, UI_TOP_H, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 1);
  display.print(modeLabel);
  int16_t iconX = SCREEN_WIDTH - 10;

  // Mute icon: show whenever silence is active (except in CHAOS; we simply never render CHAOS here)
  if (isMuted())   { uiDrawIcon(iconX, 1, GLYPH_MUTE);    iconX -= 10; }
  if (m.freezing)  { uiDrawIcon(iconX, 1, GLYPH_FREEZE);  iconX -= 10; }
  if (m.overheated){ uiDrawIcon(iconX, 1, GLYPH_OVERHEAT);iconX -= 10; }
  if (m.warning)   { uiDrawIcon(iconX, 1, GLYPH_WARN);    iconX -= 10; }
}

static void uiHeatBar(const UIMetrics& m) {
  // Compact horizontal meter right under the header
  const uint8_t topY  = UI_TOP_H + 4;      // tighter to header
  const uint8_t h     = 8;                 // short to save space
  const uint8_t leftX = 8;
  const uint8_t rightX= SCREEN_WIDTH - 8;
  const uint8_t w     = rightX - leftX;

  display.drawRect(leftX, topY, w, h, SSD1306_WHITE);
  uint8_t fillW = (uint8_t)((w-2) * m.heatPercent / 100.0f);
  if (fillW > 0) display.fillRect(leftX+1, topY+1, fillW, h-2, SSD1306_WHITE);

  // small tick marks
  for (int i=0;i<=10;i++) {
    int x = leftX + (int)((w-2) * i / 10.0f) + 1;
    display.drawPixel(x, topY + h + 1, SSD1306_WHITE);
  }

  // A tiny swirl icon centered below the bar
  uiDrawIcon((SCREEN_WIDTH-8)/2, topY + h + 3, GLYPH_SWIRL);
}

static void uiStartupSteps(uint8_t progress) {
  struct Step { const char* label; uint8_t threshold; } steps[] = {
    {"IGNITION",        20},
    {"COOLANT FLOW",    60},
    {"REACTOR ONLINE", 100}
  };

  // Start just under the swirl below the heat bar
  const uint8_t startY  = UI_TOP_H + 4 + 8 + 3 + 8; // barTop + barH + gap + icon(8)
  const uint8_t spacing = 12; // slightly roomier without the progress bar
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

// --- STABLE ambient visuals ---
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

static void uiFreezedownCool(uint32_t tMs, const UIMetrics& /*m*/) {
  const uint8_t midTop = UI_TOP_H + 4 + 8 + 3 + 8;
  const uint8_t midH   = SCREEN_HEIGHT - midTop;

  for (int i=0;i<24;i++) {
    int x = (i*5 + (tMs/20)) % (SCREEN_WIDTH-16) + 8;
    int y = midTop + (i*3 + (tMs/30)) % max(8, (int)midH-8);
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}

static void uiArmingNumber(uint8_t n) {
  display.setTextSize((n < 10) ? 3 : 2);
  display.setTextColor(SSD1306_WHITE);
  char buff[6];
  snprintf(buff, sizeof(buff), "%u", n);
  const uint8_t yBody = UI_TOP_H + 4 + 8 + 3 + 8;
  uiTextCentered(buff, yBody + 10, (n<10)?3:2);
}

static inline uint8_t clampU8(int v) {
  if (v < 0) return 0; if (v > 100) return 100; return (uint8_t)v;
}

static uint8_t breathHeatPercent(uint8_t basePercent, uint32_t nowMs) {
  const float twoPi = 6.28318530718f;
  float phase = (nowMs % STABLE_BREATH_MS) / (float)STABLE_BREATH_MS;
  float s = sinf(phase * twoPi);
  int breathed = (int)roundf((float)basePercent + (float)STABLE_BREATH_AMPL * s);
  return clampU8(breathed);
}

// ---- 5) Facade ----
struct ReactorUI {
  void render(Mode mMode, const UIMetrics& m) {
    const uint32_t now = millis();
    if (mMode != MODE_CHAOS) {
      display.clearDisplay();

      // Header
      switch (mMode) {
        case MODE_STABLE:      uiTopBar("STABLE",      m); break;
        case MODE_ARMING:      uiTopBar("ARMING",      m); break;
        case MODE_MELTDOWN:    uiTopBar("MELTDOWN",    m); break;
        case MODE_STABILIZING: uiTopBar("STABILIZING", m); break;
        case MODE_STARTUP:     uiTopBar("STARTUP",     m); break;
        case MODE_FREEZEDOWN:  uiTopBar("FREEZEDOWN",  m); break;
        case MODE_SHUTDOWN:    uiTopBar("SHUTDOWN",    m); break;
        case MODE_CHAOS:       uiTopBar("CHAOS",       m); break;
      }

      // Status (heat) bar — always visible under header
      UIMetrics heatM = m;
      if (mMode == MODE_STABLE)        heatM.heatPercent = breathHeatPercent(m.heatPercent, now);
      else if (mMode == MODE_MELTDOWN) heatM.heatPercent = 100;
      uiHeatBar(heatM);

      // Body
      switch (mMode) {
        case MODE_STABLE: {
          uiStablePulse(now);
          uiStableStatusText();
          if (((now/750) % 2) == 0) uiDrawIcon(4, UI_TOP_H + 2, GLYPH_POWER);
        } break;
        case MODE_ARMING:
          uiArmingNumber((uint8_t)constrain((m.progress>0)?m.progress:0, 0, 99));
          break;
        case MODE_MELTDOWN:
          uiMeltdownCountdown(m);
          if (((now/200) % 2) == 0) uiDrawIcon(4, UI_TOP_H + 2, GLYPH_WARN);
          break;
        case MODE_STABILIZING:
          uiStabilizingWave(now, m.progress);
          break;
        case MODE_STARTUP:
          uiStartupSteps(m.progress);
          break;
        case MODE_FREEZEDOWN: {
          // Display freezing message
          const uint8_t midTop = UI_TOP_H + 4 + 8 + 3 + 8;
          display.setTextSize(1);
          display.setTextColor(SSD1306_WHITE);
          int16_t x1, y1; uint16_t w, h;
          display.getTextBounds("The core is", 0, 0, &x1, &y1, &w, &h);
          display.setCursor((SCREEN_WIDTH - w) / 2, midTop + 8);
          display.println("The core is");
          display.getTextBounds("freezing", 0, 0, &x1, &y1, &w, &h);
          display.setCursor((SCREEN_WIDTH - w) / 2, midTop + 20);
          display.println("freezing");
          
          // Progress bar at bottom
          const uint8_t pbY = SCREEN_HEIGHT - 6;
          display.drawLine(8, pbY, 8 + (int)((SCREEN_WIDTH-16) * m.progress / 100.0f), pbY, SSD1306_WHITE);
        } break;
        case MODE_SHUTDOWN:
          uiStabilizingWave(now, m.progress);
          break;
        case MODE_CHAOS:
          break;
      }
      display.display();
    }
  }
};

ReactorUI ui;

// ======================= Setup =======================
void setup() {
  Wire.setClock(400000);

  pinMode(PIN_LED_MELTDOWN,     OUTPUT);
  pinMode(PIN_LED_STABLE,       OUTPUT);
  pinMode(PIN_LED_STARTUP,      OUTPUT);
  pinMode(PIN_LED_FREEZEDOWN,   OUTPUT);
  pinMode(PIN_BUZZER,           OUTPUT);

  btnOverride.begin(PIN_BUTTON_OVERRIDE);
  btnStabilize.begin(PIN_BUTTON_STABILIZE);
  btnStartup.begin(PIN_BUTTON_STARTUP);
  btnFreezedown.begin(PIN_BUTTON_FREEZEDOWN);
  btnShutdown.begin(PIN_BUTTON_SHUTDOWN);
  btnEvent.begin(PIN_BUTTON_EVENT);
  btnAck.begin(PIN_BUTTON_ACK);

  // Heat bar pins
  for (uint8_t i = 0; i < HEAT_COUNT; ++i) {
    pinMode(HEAT_PINS[i], OUTPUT);
    digitalWrite(HEAT_PINS[i], HEAT_ACTIVE_HIGH ? LOW : HIGH); // all off
  }

  // Seed chaos effects
  randomSeed(analogRead(A0));

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) { /* halt if OLED missing */ }
  }

  // Ensure quiet baseline
  digitalWrite(PIN_BUZZER, LOW);
  buzzerOff();

  // ✅ Prevent first-frame/time-step jumps
  unsigned long now = millis();
  heatTickAt = now;
  uiFrameAt  = now;

  enterStable();
}

// ======================= Main Loop =======================
void loop() {
  // Update debounce state
  btnOverride.update();
  btnStabilize.update();
  btnStartup.update();
  btnFreezedown.update();
  btnShutdown.update();
  btnEvent.update();
  btnAck.update();

  // Read edges ONCE per loop
  bool overrideFell    = btnOverride.fell();
  bool stabilizeFell   = btnStabilize.fell();
  bool startupFell     = btnStartup.fell();
  bool freezedownFell  = btnFreezedown.fell();
  bool shutdownFell    = btnShutdown.fell();
  bool eventFell       = btnEvent.fell();
  bool ackFell         = btnAck.fell();

  // ---- Secret sequence capture ----
  char code = 0;
  if (overrideFell)   code = 'O';
  if (stabilizeFell)  code = 'S';
  if (startupFell)    code = 'U';
  if (freezedownFell) code = 'F';
  if (shutdownFell)   code = 'D';
  if (eventFell)      code = 'E';
  if (code) {
    if (seqLength < SEQ_MAX) {
      seqBuffer[seqLength++] = code;
      seqLastInput = millis();
      checkSecretSequence();
    } else {
      seqLength = 0;
    }
  }

  // ---- Event resolution first ----
  if (activeEvent != EVENT_NONE) {
    char pressed = 0;
    if (overrideFell)   pressed = 'O';
    if (stabilizeFell)  pressed = 'S';
    if (startupFell)    pressed = 'U';
    if (freezedownFell) pressed = 'F';
    if (shutdownFell)   pressed = 'D';
    if (eventFell)      pressed = 'E';
    
    if (pressed == requiredButton) {
      resolveEvent();
      // Don't process normal button actions when resolving event
      return;
    } else if (pressed != 0) {
      // Wrong button pressed during event - ignore it
      return;
    }
  }

  // ---- Button -> Mode transitions ----
  if (overrideFell) {
    if (mode == MODE_STABLE)   enterArming();
    else if (mode == MODE_STABILIZING) abortStabilizingToMeltdown();
    else if (mode == MODE_STARTUP)     enterArming();
  }

  if (stabilizeFell) {
    if (mode == MODE_STABLE)        enterFreezedown(); // shortcut
    else if (mode == MODE_ARMING)   enterStabilizing();
    else if (mode == MODE_MELTDOWN) enterStabilizing();
    // In CHAOS, stabilize is ignored (only Startup can recover)
  }

  if (startupFell) {
    if (mode == MODE_STABLE)  enterStartup();
    else if (mode == MODE_DARK) enterStartup(); // wake from dark
    else if (mode == MODE_CHAOS) enterStartup(); // reboot from chaos
  }

  if (freezedownFell && (mode == MODE_STABLE || mode == MODE_MELTDOWN)) {
    enterFreezedown();
  }

  if (shutdownFell && (mode == MODE_STABLE || mode == MODE_CHAOS)) {
    enterShutdown();
  }

  // Event button triggers random event in stable mode
  if (eventFell && mode == MODE_STABLE && activeEvent == EVENT_NONE) {
    triggerRandomEvent();
  }

  // If ACK pressed: start/extend mute and silence immediately
  if (ackFell) {
    g_muteUntil = millis() + ACK_SILENCE_MS;
    buzzerOff(); // kill current sound immediately
  }

  // ---- Tick current mode ----
  switch (mode) {
    case MODE_ARMING:       tickArming();       break;
    case MODE_MELTDOWN:     tickMeltdown();     break;
    case MODE_STABILIZING:  tickStabilizing();  break;
    case MODE_STARTUP:      tickStartup();      break;
    case MODE_FREEZEDOWN:   tickFreezedown();   break;
    case MODE_SHUTDOWN:     tickShutdown();     break;
    case MODE_DARK:         tickDark();         break;
    case MODE_CHAOS:        tickChaos();        break;
    case MODE_STABLE:       break;
  }

  // ---- Random event system ----
  if (mode == MODE_STABLE) {
    tickRandomEvents();
  }

  unsigned long now = millis();
  if (mode != MODE_CHAOS && mode != MODE_DARK && (now - uiFrameAt) >= UI_FRAME_MS) {
    uiFrameAt = now;
    renderActiveUIFrame();  // repaints current screen (incl. progress bars)
  }

  // Heat bar (skip during CHAOS and DARK)
  if (mode != MODE_CHAOS && mode != MODE_DARK) {
    updateHeatTargetForMode();
    tickHeatBar();
  }

  tickSweep();

  // Enforce buzzer mute if active (prevents any stray tone)
  if (g_muteUntil) {
    if (millis() < g_muteUntil) {
      buzzerOff();
    } else {
      g_muteUntil = 0;
    }
  }

  // ---- Secret system housekeeping ----
  if (seqLength > 0 && (millis() - seqLastInput > SEQ_TIMEOUT_MS)) {
    seqLength = 0; // timeout-based reset
  }
  // Cryo-lock expiry
  if (g_cryoUntil && millis() >= g_cryoUntil) {
    g_cryoUntil = 0;
  }
}

// ======================= State Transitions =======================
void enterStable() {
  mode = MODE_STABLE;
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE,   HIGH);
  digitalWrite(PIN_LED_STARTUP,  LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  for (uint8_t i = 0; i < HEAT_COUNT; ++i) heatWrite(i, false);

  buzzerOff();
  display.invertDisplay(false);
  drawCoreStatusForce(false);
}

void enterArming() {
  stopSweepIfAny();
  mode = MODE_ARMING;
  armStep = 0;
  armTickAt = millis();

  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
  drawArmingNumber(ARM_BLINKS);
}

void enterMeltdown() {
  if (g_godMode) {
    buzzerOff();
    drawCenteredBig("MELTDOWN BLOCKED", 2);
    delay(600);
    enterStable();
    return;
  }

  stopSweepIfAny();
  mode = MODE_MELTDOWN;
  meltdownPhase = false;
  meltdownTickAt = 0;
  meltdownStart = millis();
  lastCountdownSec = -1;
  lastCountdownDrawAt = 0;

  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);

  drawCoreStatusForce(true); // initial banner
}

void enterStabilizing() {
  stopSweepIfAny();
  mode = MODE_STABILIZING;
  stabStep = 0;
  stabStepAt = millis();
  stabLedAt = millis();
  stabLedOn = false;
  lastShownStabStep = -1;

  stabAlarmAt = millis();
  stabAlarmHigh = false;

  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff(); // tick will start tones (gated by mute)
  drawStabilizingStep(stabStep);
}

void enterStartup() {
  stopSweepIfAny();
  mode = MODE_STARTUP;
  startupStep = 0;
  startupStepAt = millis();
  startupStart = startupStepAt;
  startupBlinkAt = startupStepAt;
  startupLedOn = false;
  lastShownStartupStep = -1;

  buzzerOff();
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  display.invertDisplay(false);

  drawStartupStep(startupStep);
}

void enterFreezedown() {
  stopSweepIfAny();
  mode = MODE_FREEZEDOWN;

  freezeStep    = 0;
  freezeStepAt  = millis();
  freezeLedAt   = millis();
  freezeLedOn   = false;
  lastShownFreezeStep = -1;

  freezeAlarmAt   = millis();
  freezeAlarmHigh = false;

  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE,   LOW);
  digitalWrite(PIN_LED_STARTUP,  LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
  display.invertDisplay(false);

  drawFreezedownStep(freezeStep);
}

void enterShutdown() {
  stopSweepIfAny();
  mode = MODE_SHUTDOWN;
  shutdownStep = 0;
  shutdownStepAt = millis();
  shutdownStart = shutdownStepAt;
  lastShownShutdownStep = -1;

  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  buzzerOff();
  display.invertDisplay(false);

  drawShutdownStep(shutdownStep);
}

void enterDark() {
  mode = MODE_DARK;
  darkModeStartAt = millis();
  darkModeShowingSuccess = true;
  
  buzzerOff();
  
  // Show success message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 24);
  display.println("SHUTDOWN");
  display.setCursor(12, 40);
  display.println("SUCCESS");
  display.display();
  
  // LEDs stay on momentarily (will turn off in tickDark)
}

void enterChaos() {
  mode = MODE_CHAOS;
  buzzerOff();
  chaosTickAt = 0;
  chaosInvertAt = 0;
  digitalWrite(PIN_LED_MELTDOWN, LOW);
  digitalWrite(PIN_LED_STABLE, LOW);
  digitalWrite(PIN_LED_STARTUP, LOW);
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  display.clearDisplay();
  display.display();
}

// ---- exits ----
void finishFreezedownToStable() {
  digitalWrite(PIN_LED_FREEZEDOWN, LOW);
  sweepActive = true;
  sweepStart  = millis();
  enterStable();
}

void abortStabilizingToMeltdown() {
  enterMeltdown();
}

void finishStabilizingToStable() {
  enterStable();
  sweepActive = true;
  sweepStart = millis();
}

// ======================= Ticks =======================
void tickArming() {
  unsigned long now = millis();
  if (now - armTickAt < ARM_STEP_MS) return;
  armTickAt = now;

  ++armStep;
  bool onPhase = (armStep % 2) == 1;
  digitalWrite(PIN_LED_MELTDOWN, onPhase ? HIGH : LOW);
  if (onPhase) buzzerTone(ARM_CHIRP_HZ);
  else         buzzerOff();

  uint8_t totalToggles = ARM_BLINKS * 2;
  if (armStep <= totalToggles) {
    uint8_t num = ARM_BLINKS - ((armStep - 1) / 2);
    if (num >= 1) drawArmingNumber(num);
  }

  if (armStep >= (ARM_BLINKS * 2)) {
    buzzerOff();
    enterMeltdown();
  }
}

void tickMeltdown() {
  unsigned long now = millis();

  // Blink LED & basic alarm tone
  if (now - meltdownTickAt >= MELTDOWN_BLINK_MS) {
    meltdownTickAt = now;
    meltdownPhase = !meltdownPhase;
    digitalWrite(PIN_LED_MELTDOWN, meltdownPhase ? HIGH : LOW);
    if (meltdownPhase) buzzerTone(MELTDOWN_TONE_HZ);
    else               buzzerOff();
  }

  // Countdown to CHAOS
  unsigned long elapsed = now - meltdownStart;
  if (elapsed >= MELTDOWN_COUNTDOWN_MS) {
    buzzerOff();
    enterChaos();
    return;
  }

  // Refresh countdown display ~5fps or when the second changes
  int secRemain = (int)((MELTDOWN_COUNTDOWN_MS - elapsed + 999) / 1000); // ceil
  if (secRemain != lastCountdownSec || (now - lastCountdownDrawAt) > 200) {
    lastCountdownSec = secRemain;
    lastCountdownDrawAt = now;
    drawMeltdownCountdown(MELTDOWN_COUNTDOWN_MS - elapsed);
  }
}

void tickStabilizing() {
  unsigned long now = millis();

  // Blink stable LED with a 1s period (toggle every 500ms)
  if (now - stabLedAt >= (STAB_LED_PERIOD_MS / 2)) {
    stabLedAt = now;
    stabLedOn = !stabLedOn;
    digitalWrite(PIN_LED_STABLE, stabLedOn ? HIGH : LOW);
  }

  // Alternate the alarm tone while stabilizing (urgent "waah-waah")
  if (now - stabAlarmAt >= STAB_ALARM_PERIOD_MS) {
    stabAlarmAt = now;
    stabAlarmHigh = !stabAlarmHigh;
    buzzerTone(stabAlarmHigh ? STAB_ALARM_HIGH_HZ : STAB_ALARM_LOW_HZ);
  }

  // Advance progress steps
  if (now - stabStepAt >= STAB_STEP_MS) {
    stabStepAt = now;

    if (stabStep < STAB_TOTAL_STEPS - 1) {
      ++stabStep;
      drawStabilizingStep(stabStep);
    } else {
      // Done: stop alarm and finish stabilization
      buzzerOff();
      finishStabilizingToStable();
    }
  }
}

void tickStartup() {
  unsigned long now = millis();
  unsigned long elapsedSeq = now - startupStart;

  unsigned long totalStartupMs = (unsigned long)STARTUP_TOTAL_STEPS * STARTUP_STEP_MS;
  if (elapsedSeq < totalStartupMs) {
    float t = (float)elapsedSeq / (float)totalStartupMs;
    float ratio = (float)STARTUP_F1_HZ / (float)STARTUP_F0_HZ;
    float f = (float)STARTUP_F0_HZ * powf(ratio, t);
    buzzerTone((unsigned int)f);
  } else {
    buzzerOff();
    digitalWrite(PIN_LED_STARTUP, LOW);
    enterStabilizing();
    return;
  }

  if (now - startupBlinkAt >= STARTUP_LED_PERIOD_MS) {
    startupBlinkAt = now;
    startupLedOn = !startupLedOn;
    digitalWrite(PIN_LED_STARTUP, startupLedOn ? HIGH : LOW);
  }

  if (now - startupStepAt >= STARTUP_STEP_MS) {
    startupStepAt = now;
    if (startupStep < STARTUP_TOTAL_STEPS - 1) {
      ++startupStep;
      drawStartupStep(startupStep);
    }
  }
}

void tickFreezedown() {
  unsigned long now = millis();

  // Pulse the FREEZEDOWN LED slowly
  if (now - freezeLedAt >= (FREEZE_LED_PERIOD_MS / 2)) {
    freezeLedAt = now;
    freezeLedOn = !freezeLedOn;
    digitalWrite(PIN_LED_FREEZEDOWN, freezeLedOn ? HIGH : LOW);
  }

  // Alternate a low “cooling” alarm
  if (now - freezeAlarmAt >= FREEZE_ALARM_PERIOD_MS) {
    freezeAlarmAt = now;
    freezeAlarmHigh = !freezeAlarmHigh;
    buzzerTone(freezeAlarmHigh ? FREEZE_ALARM_HIGH_HZ : FREEZE_ALARM_LOW_HZ);
  }

  // Advance progress steps
  if (now - freezeStepAt >= FREEZE_STEP_MS) {
    freezeStepAt = now;

    if (freezeStep < FREEZE_TOTAL_STEPS - 1) {
      ++freezeStep;
      drawFreezedownStep(freezeStep);
    } else {
      // Done: stop alarm and finish
      buzzerOff();
      finishFreezedownToStable();
    }
  }
}

void tickShutdown() {
  unsigned long now = millis();
  unsigned long elapsedSeq = now - shutdownStart;

  unsigned long totalShutdownMs = (unsigned long)SHUTDOWN_TOTAL_STEPS * SHUTDOWN_STEP_MS;
  
  // Play falling pitch sweep during shutdown
  if (elapsedSeq < totalShutdownMs) {
    float t = (float)elapsedSeq / (float)totalShutdownMs;
    float ratio = (float)SHUTDOWN_F1_HZ / (float)SHUTDOWN_F0_HZ;
    float f = (float)SHUTDOWN_F0_HZ * powf(ratio, t);
    buzzerTone((unsigned int)f);
  } else {
    buzzerOff();
    enterDark();
    return;
  }

  // Advance progress steps
  if (now - shutdownStepAt >= SHUTDOWN_STEP_MS) {
    shutdownStepAt = now;
    if (shutdownStep < SHUTDOWN_TOTAL_STEPS - 1) {
      ++shutdownStep;
      drawShutdownStep(shutdownStep);
    }
  }
}

void tickDark() {
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
    for (uint8_t i = 0; i < HEAT_COUNT; ++i) {
      heatWrite(i, false);
    }
    
    // Clear and turn off display
    display.clearDisplay();
    display.display();
  }
  
  // Stay dark - only startup button will wake us up
}

void tickChaos() {
  unsigned long now = millis();

  // Randomize indicator LEDs fast
  if (now - chaosTickAt >= 60) {
    chaosTickAt = now;
    digitalWrite(PIN_LED_MELTDOWN,  random(2) ? HIGH : LOW);
    digitalWrite(PIN_LED_STABLE,    random(2) ? HIGH : LOW);
    digitalWrite(PIN_LED_STARTUP,   random(2) ? HIGH : LOW);
    digitalWrite(PIN_LED_FREEZEDOWN,random(2) ? HIGH : LOW);

    // Heat bar raw flicker (override smoothing while in CHAOS)
    for (uint8_t i = 0; i < HEAT_COUNT; ++i) {
      heatWrite(i, random(2));
    }

    // Buzzer chaos (still respects mute via wrapper)
    int f = random(220, 2200);
    buzzerTone(f);

    // LCD artifacts
    if (random(4) == 0) {
      display.fillRect(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), random(10, 40), random(4, 20), SSD1306_WHITE);
    } else if (random(4) == 0) {
      display.drawLine(random(SCREEN_WIDTH), random(SCREEN_HEIGHT),
                       random(SCREEN_WIDTH), random(SCREEN_HEIGHT),
                       SSD1306_WHITE);
    } else {
      for (int i = 0; i < 60; ++i)
        display.drawPixel(random(SCREEN_WIDTH), random(SCREEN_HEIGHT), SSD1306_WHITE);
    }
    if (random(5) == 0) display.clearDisplay();
    display.display();
  }

  // Periodic invert flash
  if (now - chaosInvertAt >= 180) {
    chaosInvertAt = now;
    display.invertDisplay(random(2));
  }
}

void tickSweep() {
  if (!sweepActive) return;

  unsigned long now = millis();
  unsigned long elapsed = now - sweepStart;

  if (elapsed >= SWEEP_MS) {
    buzzerOff();
    sweepActive = false;
    return;
  }

  float t = (float)elapsed / (float)SWEEP_MS;
  float ratio = (float)SWEEP_F1_HZ / (float)SWEEP_F0_HZ;
  float f = (float)SWEEP_F0_HZ * powf(ratio, t);
  if (f < 60) f = 60;
  buzzerTone((unsigned int)f);
}

// ======================= Helpers =======================
void stopSweepIfAny() {
  if (sweepActive) {
    sweepActive = false;
    buzzerOff();
  }
}

static inline uint8_t currentHeatPercent() {
  float pct = (heatValue / (float)HEAT_COUNT) * 100.0f;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return (uint8_t)(pct + 0.5f);
}

void drawCoreStatus(bool warning) {
  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.warning = warning;
  m.overheated = warning;
  if (warning) ui.render(MODE_MELTDOWN, m);
  else         ui.render(MODE_STABLE, m);
}

void drawCoreStatusForce(bool warning) {
  lastWarningShown = !warning; // force next draw
  drawCoreStatus(warning);
}

void drawCenteredBig(const char* txt, uint8_t size) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(size);
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (SCREEN_WIDTH - (int)w) / 2;
  int16_t y = (SCREEN_HEIGHT - (int)h) / 2;
  display.setCursor(x, y);
  display.print(txt);
  display.display();
}

void drawArmingNumber(uint8_t n) {
  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.warning = false;
  m.progress = n;
  ui.render(MODE_ARMING, m);
}

void drawStabilizingStep(uint8_t step) {
  if (step == lastShownStabStep) return;
  lastShownStabStep = step;
  if (step >= STAB_TOTAL_STEPS) step = STAB_TOTAL_STEPS - 1;

  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / STAB_TOTAL_STEPS);
  ui.render(MODE_STABILIZING, m);
}

void drawFreezedownStep(uint8_t step) {
  if (step == lastShownFreezeStep) return;
  lastShownFreezeStep = step;
  if (step >= FREEZE_TOTAL_STEPS) step = FREEZE_TOTAL_STEPS - 1;

  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / FREEZE_TOTAL_STEPS);
  m.freezing = true;
  ui.render(MODE_FREEZEDOWN, m);
}

void drawStartupStep(uint8_t step) {
  if (step == lastShownStartupStep) return;
  lastShownStartupStep = step;
  if (step >= STARTUP_TOTAL_STEPS) step = STARTUP_TOTAL_STEPS - 1;

  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / STARTUP_TOTAL_STEPS);
  ui.render(MODE_STARTUP, m);
}

void drawShutdownStep(uint8_t step) {
  if (step == lastShownShutdownStep) return;
  lastShownShutdownStep = step;
  if (step >= SHUTDOWN_TOTAL_STEPS) step = SHUTDOWN_TOTAL_STEPS - 1;

  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.progress = (uint8_t)(((step + 1) * 100) / SHUTDOWN_TOTAL_STEPS);
  ui.render(MODE_SHUTDOWN, m);
}

void drawMeltdownCountdown(unsigned long msRemaining) {
  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  m.countdownMs = (int32_t)msRemaining;
  m.warning = true;
  m.overheated = true;
  ui.render(MODE_MELTDOWN, m);
}

void renderStableUIFrame() {
  UIMetrics m;
  m.heatPercent = currentHeatPercent();
  ui.render(MODE_STABLE, m);
}

void renderActiveUIFrame() {
  UIMetrics m;
  m.heatPercent = currentHeatPercent();

  switch (mode) {
    case MODE_STABLE:
      // Only render background if no event, or render less frequently during events
      if (activeEvent == EVENT_NONE) {
        ui.render(MODE_STABLE, m);
      } else {
        // During event, keep display static - only redraw event box
        static unsigned long lastEventDraw = 0;
        if (millis() - lastEventDraw > 500 || lastEventDraw == 0) {
          lastEventDraw = millis();
          ui.render(MODE_STABLE, m);
        }
        
        // Draw solid event box overlay
        display.fillRect(4, 26, SCREEN_WIDTH-8, 32, SSD1306_BLACK);
        display.drawRect(4, 26, SCREEN_WIDTH-8, 32, SSD1306_WHITE);
        display.drawRect(5, 27, SCREEN_WIDTH-10, 30, SSD1306_WHITE);
        
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        
        // Center the event message
        display.setCursor(10, 31);
        display.println(getEventMessage());
        
        display.setCursor(10, 42);
        display.print("PRESS ");
        display.println(getButtonName(requiredButton));
        
        display.display();
      }
      break;

    case MODE_ARMING: {
      uint8_t totalToggles = ARM_BLINKS * 2;
      uint8_t togglesDone  = MIN((uint8_t)armStep, totalToggles);
      uint8_t remaining    = (togglesDone >= totalToggles) ? 0
                            : (ARM_BLINKS - (togglesDone / 2));
      m.progress = remaining;
      ui.render(MODE_ARMING, m);
    } break;

    case MODE_STARTUP: {
      uint8_t step = MIN((uint8_t)startupStep, (uint8_t)(STARTUP_TOTAL_STEPS - 1));
      m.progress   = (uint8_t)(((step + 1) * 100) / STARTUP_TOTAL_STEPS);
      ui.render(MODE_STARTUP, m);
    } break;

    case MODE_STABILIZING: {
      uint8_t step = MIN((uint8_t)stabStep, (uint8_t)(STAB_TOTAL_STEPS - 1));
      m.progress   = (uint8_t)(((step + 1) * 100) / STAB_TOTAL_STEPS);
      ui.render(MODE_STABILIZING, m);
    } break;

    case MODE_FREEZEDOWN: {
      uint8_t step = MIN((uint8_t)freezeStep, (uint8_t)(FREEZE_TOTAL_STEPS - 1));
      m.progress   = (uint8_t)(((step + 1) * 100) / FREEZE_TOTAL_STEPS);
      m.freezing   = true;
      ui.render(MODE_FREEZEDOWN, m);
    } break;

    case MODE_SHUTDOWN: {
      uint8_t step = MIN((uint8_t)shutdownStep, (uint8_t)(SHUTDOWN_TOTAL_STEPS - 1));
      m.progress   = (uint8_t)(((step + 1) * 100) / SHUTDOWN_TOTAL_STEPS);
      ui.render(MODE_SHUTDOWN, m);
    } break;

    case MODE_MELTDOWN: {
      unsigned long now = millis();
      unsigned long elapsed = now - meltdownStart;
      long remain = (long)MELTDOWN_COUNTDOWN_MS - (long)elapsed;
      if (remain < 0) remain = 0;
      m.countdownMs = (int32_t)remain;
      m.warning     = true;
      m.overheated  = true;
      ui.render(MODE_MELTDOWN, m);
    } break;

    case MODE_CHAOS:
      // Chaos renders elsewhere
      break;
  }
}

// ======================= Heat Bar (helpers) =======================
inline void heatWrite(uint8_t idx, bool on) {
  if (idx >= HEAT_COUNT) return;
  digitalWrite(HEAT_PINS[idx], (on ^ !HEAT_ACTIVE_HIGH) ? HIGH : LOW);
}

void updateHeatTargetForMode() {
  switch (mode) {
    case MODE_STABLE:
      heatTarget = 4.0f; // safe-ish idle
      break;

    case MODE_ARMING: {
      float phase = (float)armStep / (float)(ARM_BLINKS * 2); // 0..1
      if (phase < 0) phase = 0; if (phase > 1) phase = 1;
      heatTarget = 6.0f + 4.0f * phase; // 6..10
      } break;

    case MODE_STARTUP: {
      unsigned long now = millis();
      unsigned long total = (unsigned long)STARTUP_TOTAL_STEPS * STARTUP_STEP_MS;
      float t = (float)(now - startupStart) / (float)total;  // 0..1
      if (t < 0) t = 0; if (t > 1) t = 1;
      heatTarget = 3.0f + 6.0f * t; // 3..9 over startup
      } break;

    case MODE_STABILIZING:
      heatTarget = 9.0f - (9.0f - 3.0f) * ((float)stabStep / (float)(STAB_TOTAL_STEPS - 1));
      break;

    case MODE_FREEZEDOWN:
      heatTarget = 1.0f; // near freezing
      break;

    case MODE_SHUTDOWN: {
      // Gradually decrease heat during shutdown
      unsigned long now = millis();
      unsigned long total = (unsigned long)SHUTDOWN_TOTAL_STEPS * SHUTDOWN_STEP_MS;
      float t = (float)(now - shutdownStart) / (float)total;  // 0..1
      if (t < 0) t = 0; if (t > 1) t = 1;
      heatTarget = 6.0f - 3.0f * t; // 6..3 over shutdown
      } break;

    case MODE_DARK:
      heatTarget = 0.0f; // completely off
      break;

    case MODE_MELTDOWN:
      heatTarget = 12.0f; // pegged hot
      break;

    case MODE_CHAOS:
      // No smoothing control here; we override pins directly in tickChaos()
      break;
  }

  // 🔐 Secret-mode modifiers
  if (g_cryoUntil) {
    if (heatTarget > 2.0f) heatTarget = 2.0f;
  }
}

void tickHeatBar() {
  unsigned long now = millis();
  if (now - heatTickAt < HEAT_TICK_MS) return;
  float dt = (now - heatTickAt) / 1000.0f;
  heatTickAt = now;

  float maxDelta = HEAT_SLEW_LVL_PER_S * dt;
  float delta = heatTarget - heatValue;
  if (delta >  maxDelta) delta =  maxDelta;
  if (delta < -maxDelta) delta = -maxDelta;
  heatValue += delta;

  int lit = (int)roundf(heatValue);              // 0..12
  if (lit < 0) lit = 0; if (lit > HEAT_COUNT) lit = HEAT_COUNT;

  for (int i = 0; i < HEAT_COUNT; ++i) {
    bool on = (i < lit);
    heatWrite(i, on);
  }

  if (mode == MODE_MELTDOWN) {
    bool blink = ((now / 150) % 2) == 0;
    heatWrite(HEAT_COUNT - 1, blink);
    heatWrite(HEAT_COUNT - 2, blink);
  }

  if (mode == MODE_FREEZEDOWN) {
    bool twinkle = ((now / 250) % 2) == 0;
    heatWrite(0, twinkle);
    heatWrite(1, twinkle);
  }
}

// ======================= Secret Override Logic =======================
static uint8_t patternLenNoSpaces(const char* p) {
  uint8_t n = 0;
  for (uint8_t i=0; p[i]; ++i) if (p[i] != ' ') n++;
  return n;
}

static bool matchesExact(const char* pattern) {
  uint8_t want = patternLenNoSpaces(pattern);
  if (seqLength != want) return false;
  uint8_t j = 0;
  for (uint8_t i=0; pattern[i]; ++i) {
    if (pattern[i] == ' ') continue;
    if (seqBuffer[j++] != pattern[i]) return false;
  }
  return true;
}

void checkSecretSequence() {
  if (millis() - seqLastInput > SEQ_TIMEOUT_MS) {
    seqLength = 0;
    return;
  }
  
  // S - (S)tabilize  (Green Button)
  // O - (O)verride   (Red Button)
  // U - Start(U)p    (Yellow Button)
  // F - (F)reezedown (Blue Button)

  const char* GOD_SEQ   = "O S F U O";
  const char* CHAOS_SEQ = "U U F S O F";
  const char* CRYO_SEQ  = "F F O S";

  uint8_t n = seqLength;
  if (n == patternLenNoSpaces(GOD_SEQ) && matchesExact(GOD_SEQ)) {
    seqLength = 0;
    enterGodMode();
    return;
  }
  if (n == patternLenNoSpaces(CHAOS_SEQ) && matchesExact(CHAOS_SEQ)) {
    seqLength = 0;
    enterChaos();
    return;
  }
  if (n == patternLenNoSpaces(CRYO_SEQ) && matchesExact(CRYO_SEQ)) {
    seqLength = 0;
    enterCryoLockdown();
    return;
  }

  uint8_t maxPat = max(patternLenNoSpaces(GOD_SEQ), max(patternLenNoSpaces(CHAOS_SEQ), patternLenNoSpaces(CRYO_SEQ)));
  if (seqLength > maxPat) seqLength = 0;
}

void secretToneSweep() {
  if (isMuted()) return;
  for (int f = 420; f < 1800; f += 90) {
    buzzerTone(f);
    delay(20);
    if (isMuted()) { buzzerOff(); return; }
  }
  buzzerOff();
}

void enterGodMode() {
  g_godMode = true;
  secretToneSweep();
  drawCenteredBig("OVERRIDE PROTOCOL", 1);
  delay(650);
  drawCenteredBig("GOD MODE", 2);
  delay(700);
  enterStable();
}

void enterCryoLockdown() {
  secretToneSweep();
  drawCenteredBig("CRYO LOCKDOWN", 2);
  delay(700);
  heatValue = max(heatValue - 3.0f, 0.0f);
  g_cryoUntil = millis() + CRYO_LOCK_MS;
}
// ======================= Random Event System =======================
const char* getEventMessage() {
  switch (activeEvent) {
    case EVENT_COOLANT_LEAK:    return "COOLANT LEAK!";
    case EVENT_PRESSURE_SPIKE:  return "PRESSURE SPIKE!";
    case EVENT_SENSOR_FAULT:    return "SENSOR FAULT!";
    case EVENT_CONTROL_ROD_JAM: return "ROD JAM!";
    default: return "";
  }
}

const char* getButtonName(char btn) {
  switch (btn) {
    case 'O': return "OVERRIDE";
    case 'S': return "STABILIZE";
    case 'U': return "STARTUP";
    case 'F': return "FREEZE";
    case 'D': return "SHUTDOWN";
    case 'E': return "EVENT";
    default: return "???";
  }
}

void triggerRandomEvent() {
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
  buzzerTone(1200);
  delay(100);
  buzzerOff();
}

void resolveEvent() {
  activeEvent = EVENT_NONE;
  requiredButton = 0;
  digitalWrite(PIN_LED_MELTDOWN, LOW);  // Turn off event LED
  
  // Success tone
  buzzerTone(1600);
  delay(80);
  buzzerTone(1800);
  delay(80);
  buzzerOff();
  
  // Brief success message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 24);
  display.println("EVENT");
  display.setCursor(12, 42);
  display.println("RESOLVED");
  display.display();
  delay(600);
}

void failEvent() {
  activeEvent = EVENT_NONE;
  requiredButton = 0;
  digitalWrite(PIN_LED_MELTDOWN, LOW);  // Turn off event LED
  
  // Warning tone
  buzzerTone(800);
  delay(150);
  buzzerOff();
  
  // Increase heat as penalty
  heatValue = min(heatValue + 2.0f, 12.0f);
  
  // Brief failure message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28, 24);
  display.println("EVENT");
  display.setCursor(24, 42);
  display.println("FAILED!");
  display.display();
  delay(600);
}

void tickRandomEvents() {
  unsigned long now = millis();
  
  // If event is active, check for timeout
  if (activeEvent != EVENT_NONE) {
    // Play alternating alarm tone
    if (now - eventAlarmAt >= EVENT_ALARM_PERIOD_MS) {
      eventAlarmAt = now;
      eventAlarmHigh = !eventAlarmHigh;
      buzzerTone(eventAlarmHigh ? EVENT_ALARM_HIGH_HZ : EVENT_ALARM_LOW_HZ);
    }
    
    // Blink the meltdown LED
    if (now - eventLedBlinkAt >= EVENT_LED_BLINK_MS) {
      eventLedBlinkAt = now;
      eventLedOn = !eventLedOn;
      digitalWrite(PIN_LED_MELTDOWN, eventLedOn ? HIGH : LOW);
    }
    
    if (now - eventStartAt >= EVENT_TIMEOUT_MS) {
      failEvent();
    }
  }
}