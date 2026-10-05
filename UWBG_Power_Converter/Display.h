#ifndef DISPLAY_H
#define DISPLAY_H

// ---------------------------------------------------------------------------
// DISPLAY MODULE (public interface)
//
// WHAT: A tabbed status screen on the Hosyond 3.5" ST7796U panel. A tab bar
//       runs across the top, and one tab is shown at a time:
//         Solar | Wind | Bus | Battery | Control | Status
//       Tapping a tab in the bar switches to it.
//
// HOW:  display_init() brings up the panel and touch controller and draws
//       the tab bar. display_update() takes a DisplayData struct and redraws
//       the values on the active tab. display_handleTouch() checks for a tap
//       on the tab bar and switches tabs.
//
// WHY:  Each tab shows one category of information at a readable size,
//       instead of cramming everything onto one screen. The display only
//       draws what it's given and does no calculations.
// ---------------------------------------------------------------------------

#include "Sensors.h"

struct DisplayData {
  PowerReading solar;        // Solar branch voltage/current/power
  PowerReading wind;         // Wind branch voltage/current/power
  PowerReading commonBus;    // Common DC bus (boost input) voltage/current/power
  PowerReading battery;      // Battery / boost output voltage/current/power
  float dutyPercent;         // Commanded PWM duty cycle, 0-100
  float efficiencyPercent;   // Boost efficiency, 0 when input is negligible
  bool faulted;              // True if the charge controller has latched a fault
};

enum DisplayTab {
  TAB_SOLAR = 0,
  TAB_WIND,
  TAB_BUS,
  TAB_BATTERY,
  TAB_CONTROL,
  TAB_STATUS,
  TAB_COUNT
};

void display_init();
void display_update(const DisplayData& data);
void display_handleTouch();

#endif
