// ---------------------------------------------------------------------------
// DISPLAY MODULE (implementation)
//
// WHAT: Draws the tab bar and the active tab's values on the ST7796U panel,
//       and switches tabs when the touch controller reports a tap on the bar.
//
// HOW:  The screen is 480 x 320 in landscape (rotation 1). The top TAB_BAR_H
//       pixels hold TAB_COUNT equal tabs, each TAB_W wide. Everything below
//       is the content area for the active tab.
//
//       display_update() stores the latest data, then redraws only the value
//       boxes on the active tab. Each value box is cleared before printing,
//       so a shorter number leaves no stray characters. Switching tabs does a
//       full redraw of the content area.
//
//       display_handleTouch() reads one touch point. If it lands in the tab
//       bar, the tab under it becomes active.
//
// WHY:  VERIFY BEFORE RELYING ON THIS: the library names (tft.begin(), color
//       constants, touchCtrl.read_td_status(), touchCtrl.read_touch1_x/y) are written
//       from how these libraries are usually structured, not checked against
//       the installed versions. Touch coordinates may also need swapping or
//       flipping to match landscape orientation. Test the tap zones on the
//       real panel before trusting them.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>
#include <FT6336U.h>
#include "Display.h"
#include "Config.h"
#include "Relays.h"        // RelayId, RELAY_COUNT -- for the Relays tab's button layout
#include "LoadControl.h"   // loadControl_getDesired/getActual/setDesired -- see the note in Display.h about this exception

Adafruit_ST7796S_kbv tft(LCD_CS_PIN, LCD_RS_PIN, LCD_RST_PIN);   // Hardware-SPI display object
FT6336U touchCtrl(TOUCH_RST_PIN, TOUCH_INT_PIN);                // I2C touch controller (library order: rst, int)

static const uint16_t COLOR_BLACK  = 0x0000;   // RGB565
static const uint16_t COLOR_WHITE  = 0xFFFF;
static const uint16_t COLOR_RED    = 0xF800;
static const uint16_t COLOR_GREEN  = 0x07E0;
static const uint16_t COLOR_YELLOW = 0xFFE0;   // Used only for a load relay that's desired ON but held off by the power check

static const int16_t SCREEN_W   = 480;
static const int16_t TAB_BAR_H  = 36;
static const int16_t TAB_W      = SCREEN_W / TAB_COUNT;   // 80 px per tab
static const int16_t CONTENT_Y  = TAB_BAR_H + 4;
static const uint16_t TAB_INACTIVE_COLOR = 0x7BEF;        // Mid grey for unselected tabs

static const char* const TAB_LABELS[TAB_COUNT] = {
  "Solar", "Wind", "Bus", "Battery", "Control", "Temp", "Relays", "Status"
};

static DisplayTab activeTab = TAB_SOLAR;
static DisplayData latest;       // Most recent values, kept so a tab switch can redraw immediately
static bool haveData = false;

// Draws one value in a box that's cleared first, so the old text never shows through.
static void drawValue(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size,
                      float value, uint8_t digits, const char* unit, uint16_t color) {
  tft.fillRect(x, y, w, h, COLOR_BLACK);
  tft.setTextSize(size);
  tft.setTextColor(color, COLOR_BLACK);
  tft.setCursor(x, y);
  tft.print(value, digits);
  tft.print(unit);
}

static void drawLabel(int16_t x, int16_t y, const __FlashStringHelper* text) {
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setCursor(x, y);
  tft.print(text);
}

// Same as drawLabel(), but clears its line first -- for a label whose text can be
// replaced by a different, possibly shorter, message later (e.g. the Status tab's
// instruction line, which differs between the OK and FAULT states).
static void drawLabelCleared(int16_t x, int16_t y, const __FlashStringHelper* text) {
  tft.fillRect(x, y, 460, 16, COLOR_BLACK);
  drawLabel(x, y, text);
}

// Same as drawLabel(), but clears its line first and takes a runtime string (not F()),
// for text that changes at runtime -- e.g. the fault reason, which isn't known at compile time.
static void drawStatusLine(int16_t y, const char* text, uint16_t color) {
  tft.fillRect(10, y, 460, 16, COLOR_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(color, COLOR_BLACK);
  tft.setCursor(10, y);
  tft.print(text);
}

// ---------------------------------------------------------------------------
// BLOCK 20: mode footer
//
// WHAT: A one-line footer at the bottom of every tab showing whether the
//       firmware is in manual test mode or automatic CC/CV mode.
//
// HOW:  MANUAL_PWM_TEST_MODE is a compile-time #define from Config.h, so
//       this never changes while the board is running -- it's drawn once at
//       startup and again whenever a tab switch clears the content area.
//
// WHY:  The Manual's own display guide used to list "operating mode" under
//       what's NOT shown on screen. Someone looking only at the panel
//       (no laptop attached) couldn't tell manual test mode from real
//       automatic charging. This closes that gap.
// ---------------------------------------------------------------------------
static void drawModeFooter() {
  tft.fillRect(0, 296, SCREEN_W, 24, COLOR_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setCursor(10, 300);
  if (MANUAL_PWM_TEST_MODE) {
    tft.print(F("Mode: MANUAL TEST"));
  } else {
    tft.print(F("Mode: AUTOMATIC"));
  }
}

static void drawTabBar() {
  for (int i = 0; i < TAB_COUNT; i++) {
    uint16_t background = (i == activeTab) ? COLOR_WHITE : TAB_INACTIVE_COLOR;
    uint16_t foreground = (i == activeTab) ? COLOR_BLACK : COLOR_WHITE;
    tft.fillRect(i * TAB_W, 0, TAB_W, TAB_BAR_H, background);
    tft.setTextSize(1);
    tft.setTextColor(foreground, background);
    tft.setCursor(i * TAB_W + 8, 14);   // Roughly centered for the short labels
    tft.print(TAB_LABELS[i]);
  }
}

// Draws one temperature value, or "--" if the reading isn't valid (not ready yet, or
// the probe isn't connected). Always clears the same box size regardless of which
// branch runs, so switching between a real value and "--" never leaves stray pixels
// from the other case's text.
static void drawTempValue(int16_t x, int16_t y, bool valid, float value) {
  tft.fillRect(x, y, 460, 32, COLOR_BLACK);
  tft.setTextSize(4);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setCursor(x, y);
  if (valid) {
    tft.print(value, 1);
    tft.print(F(" C"));
  } else {
    tft.print(F("--"));
  }
}

// ---------------------------------------------------------------------------
// BLOCK 28: Relays tab -- button layout, drawing, and touch handling
//
// WHAT: A 2-column x 3-row grid of 6 buttons (Solar, Wind, Data Center, EV
//       Charging, LED Lamp, Other), one per relay. Tapping a button toggles
//       what the operator wants (loadControl_setDesired()); the button's
//       color reflects what's actually happening right now.
//
// HOW:  RELAY_BUTTONS below is the single source of truth for where each
//       button is drawn AND where a tap is tested against -- both
//       drawRelayTab() and handleRelayTabTouch() loop over this same
//       table, so the drawn button and its tap zone can never drift apart.
//
// WHY:  Three button colors, not two, because the 4 load relays have a
//       third real state that solar/wind never do:
//         grey   = OFF              (operator hasn't asked for it)
//         green  = ON               (operator asked, and it's energized)
//         yellow = WAITING          (operator asked, but the automatic
//                                    power-available check is holding it
//                                    off -- only possible for the 4 load
//                                    relays; solar/wind have no override,
//                                    so they can only ever be grey or green)
// ---------------------------------------------------------------------------
struct RelayButtonLayout {
  RelayId relay;          // which relay this button controls
  const char* label;      // short text drawn on the button
  int16_t x, y;            // top-left corner, in screen pixels
};

static const int16_t RELAY_BTN_W = 220;   // button width -- 2 columns fit in 480px wide with margins
static const int16_t RELAY_BTN_H = 68;    // button height -- 3 rows fit in the content area with margins

static const RelayButtonLayout RELAY_BUTTONS[RELAY_COUNT] = {
  { RELAY_SOLAR,           "Solar",    10,  CONTENT_Y + 6   },
  { RELAY_WIND,            "Wind",     250, CONTENT_Y + 6   },
  { RELAY_LOAD_DATACENTER, "Data Ctr", 10,  CONTENT_Y + 90  },
  { RELAY_LOAD_EV,         "EV Chg",   250, CONTENT_Y + 90  },
  { RELAY_LOAD_LED_LAMP,   "LED Lamp", 10,  CONTENT_Y + 174 },
  { RELAY_LOAD_OTHER,      "Other",    250, CONTENT_Y + 174 },
};

// Draws one relay button, colored by its current desired/actual combination.
static void drawRelayButton(int16_t x, int16_t y, const char* label, bool desired, bool actual) {
  uint16_t bg;
  const char* stateText;

  if (!desired) {
    bg = TAB_INACTIVE_COLOR;   // operator hasn't asked for this one -- same grey as an inactive tab
    stateText = "OFF";
  } else if (actual) {
    bg = COLOR_GREEN;          // asked for, and it's really on
    stateText = "ON";
  } else {
    bg = COLOR_YELLOW;         // asked for, but the power check is holding it off (load relays only)
    stateText = "WAIT";
  }

  tft.fillRect(x, y, RELAY_BTN_W, RELAY_BTN_H, bg);          // button background
  tft.drawRect(x, y, RELAY_BTN_W, RELAY_BTN_H, COLOR_WHITE); // thin border so it reads as a tappable button
  tft.setTextSize(2);
  tft.setTextColor(COLOR_BLACK, bg);   // black text reads clearly on all three background colors above
  tft.setCursor(x + 8, y + 10);
  tft.print(label);
  tft.setCursor(x + 8, y + 38);
  tft.print(stateText);
}

// Draws all 6 relay buttons, reading each one's current state from LoadControl.
static void drawRelayTab() {
  for (int i = 0; i < RELAY_COUNT; i++) {
    const RelayButtonLayout& btn = RELAY_BUTTONS[i];
    bool desired = loadControl_getDesired(btn.relay);
    bool actual  = loadControl_getActual(btn.relay);
    drawRelayButton(btn.x, btn.y, btn.label, desired, actual);
  }
}

// Checks a touch point against every button's rectangle; toggles the first match.
static void handleRelayTabTouch(uint16_t x, uint16_t y) {
  for (int i = 0; i < RELAY_COUNT; i++) {
    const RelayButtonLayout& btn = RELAY_BUTTONS[i];
    bool withinX = (x >= btn.x) && (x < btn.x + RELAY_BTN_W);
    bool withinY = (y >= btn.y) && (y < btn.y + RELAY_BTN_H);
    if (withinX && withinY) {
      bool current = loadControl_getDesired(btn.relay);
      loadControl_setDesired(btn.relay, !current);   // flip operator intent
      drawRelayTab();                                 // redraw immediately so the tap feels responsive
      return;                                          // one tap toggles exactly one button
    }
  }
}

// Draws the three power values (V, A, W) for one sensor on a tab.
static void drawPowerTab(const PowerReading& r) {
  drawLabel(10, CONTENT_Y + 10, F("Voltage"));
  drawValue(10, CONTENT_Y + 30, 460, 32, 4, r.busVoltage_V, 2, " V", COLOR_WHITE);

  drawLabel(10, CONTENT_Y + 80, F("Current"));
  drawValue(10, CONTENT_Y + 100, 460, 32, 4, r.current_A, 2, " A", COLOR_WHITE);

  drawLabel(10, CONTENT_Y + 150, F("Power"));
  drawValue(10, CONTENT_Y + 170, 460, 32, 4, r.power_W, 2, " W", COLOR_WHITE);
}

static void drawActiveTabValues() {
  if (!haveData) {
    return;
  }

  switch (activeTab) {
    case TAB_SOLAR:
      drawPowerTab(latest.solar);
      break;
    case TAB_WIND:
      drawPowerTab(latest.wind);
      break;
    case TAB_BUS:
      drawPowerTab(latest.commonBus);
      break;
    case TAB_BATTERY:
      drawPowerTab(latest.battery);
      break;
    case TAB_CONTROL:
      drawLabel(10, CONTENT_Y + 10, F("Duty cycle"));
      drawValue(10, CONTENT_Y + 30, 460, 32, 4, latest.dutyPercent, 1, " %", COLOR_WHITE);

      drawLabel(10, CONTENT_Y + 80, F("Efficiency"));
      drawValue(10, CONTENT_Y + 100, 460, 32, 4, latest.efficiencyPercent, 1, " %", COLOR_WHITE);
      break;
    case TAB_TEMP:
      drawLabel(10, CONTENT_Y + 10, F("GaN Switch"));
      drawTempValue(10, CONTENT_Y + 30, latest.temps.valid, latest.temps.ganSwitchTemp_C);

      drawLabel(10, CONTENT_Y + 80, F("Inductor"));
      drawTempValue(10, CONTENT_Y + 100, latest.temps.valid, latest.temps.inductorTemp_C);
      break;
    case TAB_RELAYS:
      drawRelayTab();
      break;
    case TAB_STATUS:
      drawLabel(10, CONTENT_Y + 10, F("Charge controller"));
      if (latest.faulted) {
        tft.fillRect(10, CONTENT_Y + 30, 460, 24, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(COLOR_RED, COLOR_BLACK);
        tft.setCursor(10, CONTENT_Y + 30);
        tft.print(F("FAULT"));
        drawStatusLine(CONTENT_Y + 70, latest.faultReason, COLOR_WHITE);   // Why it faulted
        drawLabelCleared(10, CONTENT_Y + 95, F("Output off. Reset the board to clear."));
      } else {
        tft.fillRect(10, CONTENT_Y + 30, 460, 24, COLOR_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(COLOR_GREEN, COLOR_BLACK);
        tft.setCursor(10, CONTENT_Y + 30);
        tft.print(F("OK"));
        drawStatusLine(CONTENT_Y + 70, "", COLOR_WHITE);   // Clear any leftover fault reason text
        drawLabelCleared(10, CONTENT_Y + 95, F("Running normally."));
      }
      break;
    default:
      break;
  }
}

// Clears the content area and draws the whole active tab, including labels.
static void drawActiveTabFull() {
  tft.fillRect(0, CONTENT_Y, SCREEN_W, 320 - CONTENT_Y, COLOR_BLACK);
  drawActiveTabValues();
  drawModeFooter();   // The content-area clear above would otherwise erase this
}

void display_init() {
  pinMode(LCD_LED_PIN, OUTPUT);     // Backlight pin is a plain digital output
  digitalWrite(LCD_LED_PIN, HIGH);  // Backlight on (no dimming yet)

  tft.begin();          // Bring up the ST7796U over SPI (resets it, sends its startup commands)
  tft.setRotation(1);   // Landscape orientation -- adjust (0-3) to match how the panel is mounted
  tft.fillScreen(COLOR_BLACK);

  drawTabBar();
  drawModeFooter();   // Drawn once here; also redrawn on every tab switch (see drawActiveTabFull())

  touchCtrl.begin();   // Bring up the FT6336U touch controller over the shared I2C bus

  Serial.println(F("Display and touch controller initialized."));
}

void display_update(const DisplayData& data) {
  latest = data;
  haveData = true;
  drawActiveTabValues();
}

void display_handleTouch() {
  if (touchCtrl.read_td_status() == 0) {
    return;   // No finger on the screen
  }

  uint16_t x = touchCtrl.read_touch1_x();
  uint16_t y = touchCtrl.read_touch1_y();

  if (y < TAB_BAR_H) {
    // Tap landed in the tab bar -- switch tabs (unchanged from before).
    int tapped = x / TAB_W;
    if (tapped < 0 || tapped >= TAB_COUNT || tapped == activeTab) {
      return;
    }
    activeTab = (DisplayTab)tapped;
    drawTabBar();
    drawActiveTabFull();
    return;
  }

  // Tap landed below the tab bar. On every OTHER tab this still does
  // nothing (same as before this feature existed) -- only the Relays tab
  // has anything to tap down here.
  if (activeTab == TAB_RELAYS) {
    handleRelayTabTouch(x, y);
  }
}
