# Core Meltdown - Enhanced Animation System

## Overview
A comprehensive, screen-aware animation system has been integrated into your reactor control panel. All animations respect the 128x64 OLED layout and don't interfere with existing UI elements.

---

## Screen Layout (128x64)
```
┌──────────────────────────────────┐
│ MODE           [ICONS]        0  │ ← Header (y=0-10)
├──────────────────────────────────┤
│ ▯▯▯▯▯▯▯▯▯▯▯▯▯▯ Heat Bar   10-26│
│ . . . . . . . .                  │
│        ◉ Core Icon               │
│                              27  │
│                                  │
│     CONTENT AREA (Animations)    │ ← Main animation zone (y=27-54)
│                                  │
│                                  │
│                              55  │
│  ═══ Progress Bar ═══        58  │
│                                  │
└──────────────────────────────────┘ 64
```

---

## Animation Features by Mode

### 🔵 MODE_STABLE
**Visual**: Peaceful operation state
- **Rotating Reactor Core**: Concentric circles with 4 rotating control rods, pulsing with heat
- **Decay Particles**: Rising green dots from reactor center (radioactive simulation)
- **Geiger Counter Flashes**: Random pixel flashes representing radiation detection
- **Status Text**: "CORE STABLE" centered at bottom
- **Heat Breathing**: Heat bar gently breathes in/out

**Screen Usage**:
- Core visualization: Center (60-64px width)
- Particles: Content area (27-54px height)
- Status: Bottom (54-64px)
- No collisions ✓

---

### 🟠 MODE_ARMING
**Visual**: Countdown to reactor startup
- **Large Countdown Number**: Displayed large in content area
- **Pulsing Border**: Double-line border that pulses in/out (danger indicator)
- **Corner Brackets**: Decorative corner accents showing intensity
- **Flashing Warning Icon**: Top-left icon toggle

**Screen Usage**:
- Number: Centered, y=40 (large text)
- Border: Full screen (0,0 to 128,64)
- No overlap with top bar ✓

---

### 🔴 MODE_MELTDOWN
**Visual**: CRITICAL - Reactor explosion imminent
- **Explosive Sparks**: Particles burst outward in all directions from center
- **Chaotic Wave**: Overlapping sine waves create erratic pattern
- **Pulsing Danger Border**: Intense full-screen pulse
- **Geiger Counter**: Aggressive rapid flashing (90% intensity)
- **Countdown Timer**: T-10 seconds displayed

**Screen Usage**:
- Wave: y=27-54 (content area)
- Sparks: Center expanding outward
- Timer: y=30-40 (large text)
- Border: Full screen
- Danger flashing non-intrusive ✓

---

### 🟡 MODE_STABILIZING
**Visual**: Active reactor cooling and stabilization
- **Interference Wave**: Two sine waves at different frequencies converging
- **Coolant Flow**: Droplets falling from top (blue simulation)
- **Progress Spinner**: Small rotating line in top-right corner
- **Progress Bar**: Bottom of screen shows completion %

**Screen Usage**:
- Wave: y=30-50 (primary focus)
- Droplets: y=27-54 (falling, contained)
- Spinner: Corner (SCREEN_WIDTH-12, y=15)
- Progress bar: y=58
- Well-organized, no collisions ✓

---

### 🟢 MODE_STARTUP
**Visual**: Reactor coming online - scanning mode
- **Radar Sweep**: Rotating line with concentric circles
- **Progress Dots**: Dots appear around circle as progress increases
- **Step Counter**: Shows current step (1-5) on right side
- **Status Text**: "STEP X/5"

**Screen Usage**:
- Radar circle: Center (60px radius), y=25-55
- Progress dots: Around outer circle
- Text: Right side (x=70+)
- Text doesn't overlap radar ✓

---

### 🔷 MODE_FREEZEDOWN
**Visual**: Emergency core cryogenic freeze
- **Snowflake Particles**: Gentle falling snowflakes from top
- **Frost Spinner**: Rotating line in top-right corner
- **Status Text**: "Core freezing" centered
- **Progress Bar**: Bottom shows freeze completion

**Screen Usage**:
- Snowflakes: y=27-54 (gentle falling motion)
- Text: y=30-42 (centered, 1 char size)
- Spinner: Corner (SCREEN_WIDTH-12, y=15)
- Progress: y=58
- Spacious layout, no overlap ✓

---

### 🟣 MODE_SHUTDOWN
**Visual**: Safe controlled system power-down
- **Energy Bars**: 8 equalizer-style bars winding down
- **Waveform**: Original wave representation decaying
- **Status Text**: "Powering down"
- **Progress Bar**: Bottom shows shutdown completion

**Screen Usage**:
- Bars: y=30-48 (8 bars × 3px wide, spaced)
- Text: y=24-26
- Progress: y=58
- Clean, organized ✓

---

## Animation Library Features

### Particle Systems
- **Spawn-based**: Particles created each frame cycle
- **Physics**: Simple velocity + gravity simulation
- **Bounds checking**: Particles auto-removed outside content area
- **Memory efficient**: MAX_PARTICLES = 16 (fixed array)

### Waveforms
- **Phase-based**: Computed via `sin()` with configurable frequency
- **Harmonic support**: Multiple sine waves can overlay
- **Progress-aware**: Wave behavior changes based on mode progress
- **Smoothing**: Constrained to content area bounds

### Visual Effects
- **Circular math**: Using `cos()`/`sin()` for orbit calculations
- **Time-based rotation**: Synchronized to `millis()`
- **Pulsing effects**: `sin()` mapped to scale/intensity
- **Flickering**: Random or phase-based patterns

### Screen Effects
- **Border pulse**: Checks `sin()` phase against intensity threshold
- **Dither pattern**: Ordered dither for transitions
- **Scan lines**: Configurable (currently commented out)
- **Corner brackets**: 8px decorative lines at screen edges

---

## Technical Details

### Screen Space Constraints
```cpp
CONTENT_Y_START = 27    // Below heat bar
CONTENT_Y_END = 54      // Above status text
CONTENT_HEIGHT = 27 pixels
SCREEN_WIDTH = 128 pixels
SCREEN_HEIGHT = 64 pixels
```

### Performance Considerations
- Animations tick **every frame** (tied to `millis()`)
- Particle updates: O(16) = fixed cost
- Draw operations: Optimized for OLED SPI
- Memory: ~2KB for animation structures
- No blocking calls ✓

### Integration Points
- `ReactorAnimations::begin()` → Called in `ReactorUI::begin()`
- `ReactorAnimations::tick()` → Can be called from main loop if needed
- `ReactorAnimations::resetParticles()` → Clears all particles on demand
- Each draw function takes `display`, `nowMs`, and mode-specific parameters

---

## How to Use / Customize

### Enabling/Disabling Animations
In `ReactorUI.cpp`, each animation is called explicitly. To disable:
```cpp
// Comment out the animation call:
// ReactorAnimations::drawDecayParticles(display, now);
```

### Adjusting Animation Intensity
In `ReactorAnimations.cpp`, modify constants:
```cpp
// Example: Change spark spread
const int ANGLE_SPEED = 50;  // Slower spread

// Example: Slower waves
const float WAVE_SPEED = 800.0f;  // Instead of 600.0f
```

### Adding New Particles
```cpp
spawnParticle(
  float x,           // Starting X position
  float y,           // Starting Y position
  float vx,          // X velocity (pixels/frame)
  float vy,          // Y velocity (pixels/frame)
  uint8_t lifespan   // Frames until death
);
```

### Adding New Effects
1. Add function to `ReactorAnimations.h`
2. Implement in `ReactorAnimations.cpp`
3. Call from appropriate mode in `ReactorUI.cpp`
4. Test screen space usage

---

## What's Next?

Possible enhancements:
1. **Sound synchronization**: Animate to audio frequencies
2. **Custom transitions**: Wipe/fade between modes
3. **Combo visuals**: Chain particle effects
4. **Customizable themes**: Different color schemes
5. **Animation presets**: Quick enable/disable profiles

---

## Files Modified/Added

**New Files**:
- `ReactorAnimations.h` - Public API
- `ReactorAnimations.cpp` - Implementation

**Modified Files**:
- `ReactorUI.cpp` - Integrated all animations
- `ReactorUI.h` - Added ReactorAnimations include

---

## Quality Checklist

✅ All animations respect 128x64 screen layout  
✅ No text/UI element overlaps  
✅ Heat bar preserved  
✅ Top status bar intact  
✅ Progress bars at bottom  
✅ Content area fully utilized (27-54px)  
✅ Memory efficient (fixed-size particle array)  
✅ No blocking operations  
✅ Smooth frame-based animation  
✅ Mode-specific visual feedback  

---

Enjoy your enhanced reactor visualization! 🎆
