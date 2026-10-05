// ---------------------------------------------------------------------------
// DISPLAY MODULE (implementation)
//
// WHAT: Draws the tab bar and the active tab's values on the ST7796U panel,
//       and switches tabs when the touch controller reports a tap on the bar.
//
// HOW:  The screen is 480 x 320 in landscape (rotation 1). The top TAB_BAR_H
//       pixels hold six equal tabs, each TAB_W wide. Everything below is the
//       content area for the active tab.
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
//       constants, touch.read_td_status(), touch.read_touch1_x/y) are written
//       from how these libraries are usually structured, not checked against
//       the installed versions. Touch coordinates may also need swapping or
//       flipping to match landscape orientation. Test the tap zones on the
//       real panel before trusting them.
// ---------------------------------------------------------------------------

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>
#include <FT6336U.h>
#include "Display.h"
#include "Config.h"

Adafruit_ST7796S_kbv tft(LCD_CS_PIN, LCD_RS_PIN, LCD_RST_PIN);   // Hardware-SPI display object
FT6336U touch(TOUCH_INT_PIN, TOUCH_RST_PIN);                     // I2C touch controller object

static const int16_t SCREEN_W   = 480;
static const int16_t TAB_BAR_H  = 36;
static const int16_t TAB_W      = SCREEN_W / TAB_COUNT;   // 80 px per tab
static const int16_t CONTENT_Y  = TAB_BAR_H + 4;
static const uint16_t TAB_INACTIVE_COLOR = 0x7BEF;        // Mid grey for unselected tabs

static const char* const TAB_LABELS[TAB_COUNT] = {
  "Solar", "Wind", "Bus", "Battery", "Control", "Status"
};

static DisplayTab activeTab = TAB_SOLAR;
static DisplayData latest;       // Most recent values, kept so a tab switch can redraw immediately
static bool haveData = false;

// Draws one value in a box that's cleared first, so the old text never shows through.
static void drawValue(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t size,
                      float value, uint8_t digits, const char* unit, uint16_t color) {
  tft.fillRect(x, y, w, h, BLACK);
  tft.setTextSize(size);
  tft.setTextColor(color, BLACK);
  tft.setCursor(x, y);
  tft.print(value, digits);
  tft.print(unit);
}

static void drawLabel(int16_t x, int16_t y, const __FlashStringHelper* text) {
  tft.setTextSize(2);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(x, y);
  tft.print(text);
}

static void drawTabBar() {
  for (int i = 0; i < TAB_COUNT; i++) {
    uint16_t background = (i == activeTab) ? WHITE : TAB_INACTIVE_COLOR;
    uint16_t foreground = (i == activeTab) ? BLACK : WHITE;
    tft.fillRect(i * TAB_W, 0, TAB_W, TAB_BAR_H, background);
    tft.setTextSize(1);
    tft.setTextColor(foreground, background);
    tft.setCursor(i * TAB_W + 8, 14);   // Roughly centered for the short labels
    tft.print(TAB_LABELS[i]);
  }
}

// Draws the three power values (V, A, W) for one sensor on a tab.
static void drawPowerTab(const PowerReading& r) {
  drawLabel(10, CONTENT_Y + 10, F("Voltage"));
  drawValue(10, CONTENT_Y + 30, 460, 32, 4, r.busVoltage_V, 2, " V", WHITE);

  drawLabel(10, CONTENT_Y + 80, F("Current"));
  drawValue(10, CONTENT_Y + 100, 460, 32, 4, r.current_A, 2, " A", WHITE);

  drawLabel(10, CONTENT_Y + 150, F("Power"));
  drawValue(10, CONTENT_Y + 170, 460, 32, 4, r.power_W, 2, " W", WHITE);
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
      drawValue(10, CONTENT_Y + 30, 460, 32, 4, latest.dutyPercent, 1, " %", WHITE);

      drawLabel(10, CONTENT_Y + 80, F("Efficiency"));
      drawValue(10, CONTENT_Y + 100, 460, 32, 4, latest.efficiencyPercent, 1, " %", WHITE);
      break;
    case TAB_STATUS:
      drawLabel(10, CONTENT_Y + 10, F("Charge controller"));
      if (latest.faulted) {
        tft.fillRect(10, CONTENT_Y + 30, 460, 24, BLACK);
        tft.setTextSize(3);
        tft.setTextColor(RED, BLACK);
        tft.setCursor(10, CONTENT_Y + 30);
        tft.print(F("FAULT"));
        drawLabel(10, CONTENT_Y + 70, F("Output off. Reset the board to clear."));
      } else {
        tft.fillRect(10, CONTENT_Y + 30, 460, 24, BLACK);
        tft.setTextSize(3);
        tft.setTextColor(GREEN, BLACK);
        tft.setCursor(10, CONTENT_Y + 30);
        tft.print(F("OK"));
        drawLabel(10, CONTENT_Y + 70, F("Running normally."));
      }
      break;
    default:
      break;
  }
}

// Clears the content area and draws the whole active tab, including labels.
static void drawActiveTabFull() {
  tft.fillRect(0, CONTENT_Y, SCREEN_W, 320 - CONTENT_Y, BLACK);
  drawActiveTabValues();
}

void display_init() {
  pinMode(LCD_LED_PIN, OUTPUT);     // Backlight pin is a plain digital output
  digitalWrite(LCD_LED_PIN, HIGH);  // Backlight on (no dimming yet)

  tft.begin();          // Bring up the ST7796U over SPI (resets it, sends its startup commands)
  tft.setRotation(1);   // Landscape orientation -- adjust (0-3) to match how the panel is mounted
  tft.fillScreen(BLACK);

  drawTabBar();

  touch.begin();   // Bring up the FT6336U touch controller over the shared I2C bus

  Serial.println(F("Display and touch controller initialized."));
}

void display_update(const DisplayData& data) {
  latest = data;
  haveData = true;
  drawActiveTabValues();
}

void display_handleTouch() {
  if (touch.read_td_status() == 0) {
    return;   // No finger on the screen
  }

  uint16_t x = touch.read_touch1_x();
  uint16_t y = touch.read_touch1_y();

  if (y >= TAB_BAR_H) {
    return;   // Only the tab bar switches tabs
  }

  int tapped = x / TAB_W;
  if (tapped < 0 || tapped >= TAB_COUNT || tapped == activeTab) {
    return;
  }

  activeTab = (DisplayTab)tapped;
  drawTabBar();
  drawActiveTabFull();
}
