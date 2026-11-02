#include <Arduino.h>   // WHAT: pulls in pinMode()/digitalWrite()/HIGH/LOW -- same reason every other .cpp here includes it.
#include "Relays.h"    // WHAT: this module's own public interface (RelayId, the 3 functions).
#include "Config.h"    // WHAT: the 6 pin numbers and RELAY_ACTIVE_LOW.

// ---------------------------------------------------------------------------
// BLOCK 1: pin lookup table
//
// WHAT: Maps each RelayId to the actual Giga pin number that drives it.
// HOW:  A plain array, indexed by the enum values from Relays.h -- since
//       RELAY_SOLAR is 0, RELAY_WIND is 1, and so on, relayPins[RELAY_WIND]
//       just works as an array index with no lookup logic needed.
// WHY:  Keeping the RelayId-to-pin mapping in exactly one place means
//       changing a pin assignment later is a one-line edit in Config.h,
//       not a hunt through every function that uses a relay.
// ---------------------------------------------------------------------------
static const uint8_t relayPins[RELAY_COUNT] = {
  RELAY_SOLAR_PIN,            // index 0 -- matches RELAY_SOLAR
  RELAY_WIND_PIN,              // index 1 -- matches RELAY_WIND
  RELAY_LOAD_DATACENTER_PIN,   // index 2 -- matches RELAY_LOAD_DATACENTER
  RELAY_LOAD_EV_PIN,           // index 3 -- matches RELAY_LOAD_EV
  RELAY_LOAD_LED_LAMP_PIN,     // index 4 -- matches RELAY_LOAD_LED_LAMP
  RELAY_LOAD_OTHER_PIN         // index 5 -- matches RELAY_LOAD_OTHER
};

// ---------------------------------------------------------------------------
// BLOCK 2: remembered state
//
// WHAT: The last commanded on/off state of each relay, in plain terms
//       (true = energized/circuit closed) -- independent of whatever
//       electrical level that actually required.
// HOW:  A plain bool array, same indexing as relayPins[] above.
// WHY:  relays_getState() needs something to read back -- digitalRead()
//       on the pin would work too, but would return the INVERTED value
//       whenever RELAY_ACTIVE_LOW is true, which is exactly the kind of
//       easy-to-get-backwards detail this module exists to hide from
//       every other file that asks "is this relay on?".
// ---------------------------------------------------------------------------
static bool relayState[RELAY_COUNT] = { false, false, false, false, false, false };

// ---------------------------------------------------------------------------
// BLOCK 3: relays_init()
//
// WHAT: Configures all 6 relay pins as outputs and forces every relay
//       OFF (de-energized) at startup.
// HOW:  Loops over every index 0..RELAY_COUNT-1, calling pinMode() then
//       reusing relays_setState() itself (instead of writing the pin
//       translation logic twice) to actually turn each one off.
// WHY:  Starting with everything OFF is the safe default -- we never
//       want the solar panel, wind subsystem, or any demo load
//       energized just because the board powered up, before anything
//       has decided it should be.
// ---------------------------------------------------------------------------
void relays_init() {
  for (int i = 0; i < RELAY_COUNT; i++) {       // walk every relay index
    pinMode(relayPins[i], OUTPUT);              // this pin now drives, doesn't sense
    relays_setState((RelayId)i, false);         // and starts fully OFF
  }
}

// ---------------------------------------------------------------------------
// BLOCK 4: relays_setState()
//
// WHAT: Turns one relay on or off, by its plain meaning (energized or not).
// HOW:  Bounds-checks the index first, records the plain on/off intent,
//       then works out which electrical level (HIGH or LOW) actually
//       achieves that on THIS relay module, per RELAY_ACTIVE_LOW.
// WHY:  See Relays.h -- this is the one place that has to know about the
//       active-low quirk, so nothing else in the firmware ever has to.
// ---------------------------------------------------------------------------
void relays_setState(RelayId relay, bool energized) {
  if (relay < 0 || relay >= RELAY_COUNT) return;   // bad index -- do nothing rather than write a random pin

  relayState[relay] = energized;                   // remember the plain on/off intent

  bool pinShouldBeHigh;                             // which electrical level actually turns this relay the way we want
  if (RELAY_ACTIVE_LOW) {
    // Active-low module: LOW = relay ON, so "energized" means we want LOW,
    // which means pinShouldBeHigh is the OPPOSITE of energized.
    pinShouldBeHigh = !energized;
  } else {
    // Active-high module: HIGH = relay ON, so pinShouldBeHigh matches
    // energized directly.
    pinShouldBeHigh = energized;
  }

  digitalWrite(relayPins[relay], pinShouldBeHigh ? HIGH : LOW);  // actually drive the pin
}

// ---------------------------------------------------------------------------
// BLOCK 5: relays_getState()
//
// WHAT: Reports the last commanded on/off state of a relay, in plain
//       terms -- true means energized/circuit closed, regardless of
//       RELAY_ACTIVE_LOW.
// WHY:  So callers (the display's button-drawing code, the load-shedding
//       check) can ask "is this relay on?" without needing to know or
//       care about the active-low detail.
// ---------------------------------------------------------------------------
bool relays_getState(RelayId relay) {
  if (relay < 0 || relay >= RELAY_COUNT) return false;  // bad index -- report OFF, not garbage memory
  return relayState[relay];
}
