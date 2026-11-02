#include <Arduino.h>     // WHAT: not strictly needed for anything in here, but every module includes it for consistency/future-proofing.
#include "LoadControl.h" // WHAT: this module's own public interface.
#include "Relays.h"      // WHAT: RelayId, relays_setState(), relays_getState() -- the mechanism this module drives.
#include "Sensors.h"     // WHAT: sensors_readBattery() -- the real sensor the power-available check reads.
#include "Config.h"       // WHAT: LOAD_ENABLE_MIN_BATTERY_POWER_W.

// ---------------------------------------------------------------------------
// BLOCK 1: operator intent
//
// WHAT: What the operator last asked for, per relay, via the touchscreen.
// HOW:  A plain bool array, indexed the same way as Relays.cpp's own
//       arrays (RELAY_SOLAR = 0, RELAY_WIND = 1, etc.).
// WHY:  This has to be stored SEPARATELY from Relays.cpp's energized
//       state, specifically because they're allowed to disagree for the
//       4 load relays -- an operator can ask for the data center load to
//       be on, and it can sit there NOT actually energized (desired=true,
//       actual=false) until enough power is available. If we only stored
//       one state per relay, there'd be nowhere to remember "the operator
//       still wants this on" while it's being held off.
// ---------------------------------------------------------------------------
static bool desiredState[RELAY_COUNT] = { false, false, false, false, false, false };

void loadControl_init() {
  for (int i = 0; i < RELAY_COUNT; i++) {
    desiredState[i] = false;   // Nobody has asked for anything yet -- matches relays_init()'s own all-OFF startup state
  }
}

void loadControl_setDesired(RelayId relay, bool desiredOn) {
  if (relay < 0 || relay >= RELAY_COUNT) return;   // bad index -- ignore rather than write out of bounds
  desiredState[relay] = desiredOn;                  // just records intent -- loadControl_update() decides what actually happens
}

bool loadControl_getDesired(RelayId relay) {
  if (relay < 0 || relay >= RELAY_COUNT) return false;
  return desiredState[relay];
}

bool loadControl_getActual(RelayId relay) {
  // WHAT: proxies straight to Relays.cpp's own tracked state.
  // WHY:  Relays.cpp already remembers exactly this (see relays_getState()
  //       in Relays.cpp) -- no reason to track a second, possibly-stale
  //       copy of the same information here.
  return relays_getState(relay);
}

// ---------------------------------------------------------------------------
// BLOCK 2: loadControl_update()
//
// WHAT: The actual policy decision, run every loop() pass. For
//       RELAY_SOLAR and RELAY_WIND, actual always matches desired
//       directly -- there's no automatic override for those two. For the
//       4 load relays, actual = desired AND "enough power is available".
//
// HOW:  Reads the battery sensor once per call (not once per relay --
//       the same power reading applies to all 4 load relays equally,
//       since this is a single shared threshold, not a per-load one).
//
// WHY:  This is the ONE place that knows the load relays have an
//       automatic veto and the solar/wind relays don't -- see LoadControl.h.
//       Running it every loop() pass (rather than on its own timer) is
//       safe because it's cheap: one sensor read already being done
//       elsewhere in loop() anyway, a handful of comparisons, and
//       digitalWrite() calls that only actually toggle a pin when the
//       commanded state changes (Relays.cpp doesn't skip redundant writes,
//       but digitalWrite() itself is fast enough that this isn't a
//       concern at loop() speed).
// ---------------------------------------------------------------------------
void loadControl_update() {
  // Solar and wind: no automatic override. What the operator asked for
  // is exactly what gets energized.
  relays_setState(RELAY_SOLAR, desiredState[RELAY_SOLAR]);
  relays_setState(RELAY_WIND, desiredState[RELAY_WIND]);

  // The 4 load relays: desired AND "enough power available" -- read the
  // battery/charge-output sensor once, reuse the result for all 4.
  float batteryPower_W = sensors_readBattery().power_W;
  bool powerAvailable = (batteryPower_W >= LOAD_ENABLE_MIN_BATTERY_POWER_W);

  relays_setState(RELAY_LOAD_DATACENTER, desiredState[RELAY_LOAD_DATACENTER] && powerAvailable);
  relays_setState(RELAY_LOAD_EV,         desiredState[RELAY_LOAD_EV]         && powerAvailable);
  relays_setState(RELAY_LOAD_LED_LAMP,   desiredState[RELAY_LOAD_LED_LAMP]   && powerAvailable);
  relays_setState(RELAY_LOAD_OTHER,      desiredState[RELAY_LOAD_OTHER]      && powerAvailable);
}
