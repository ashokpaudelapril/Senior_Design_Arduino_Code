#ifndef RELAYS_H
#define RELAYS_H

// ---------------------------------------------------------------------------
// RELAYS: mechanism for switching the 6 relay-controlled circuits
//
// WHAT: The "how to turn a relay on/off" layer -- nothing in here decides
//       WHEN a relay should be on or off, only HOW to actually do it once
//       something else (the display's touch buttons, the load-shedding
//       check) has made that decision.
//
// HOW:  Six plain digital output pins (BLOCK 25 in Config.h), one per
//       relay-module channel. This header only exposes a relay ID, an
//       init function, and a set/get pair -- the caller never needs to
//       know which physical pin a relay lives on or which electrical
//       level (HIGH/LOW) that module needs to turn on.
//
// WHY:  Same "mechanism vs. policy" split already used for BoostConverter
//       (which only knows how to drive a PWM duty cycle, not what duty
//       cycle is correct) and ChargeController (which decides the duty
//       cycle). Keeping this module dumb and narrow means the actual
//       on/off decisions -- made by Display's touch handling and by the
//       load-shedding power check -- can change without ever touching
//       this file.
// ---------------------------------------------------------------------------

// WHAT: One named ID per relay-controlled circuit, in the same order as
//       the relayPins[] lookup table in Relays.cpp -- RELAY_COUNT is a
//       sentinel (not a real relay), used to size arrays and bound-check
//       indexes, not something you'd ever pass to relays_setState().
enum RelayId {
  RELAY_SOLAR,            // Solar array -> common bus
  RELAY_WIND,             // Whole wind subsystem (drive motor + generators) -> common bus
  RELAY_LOAD_DATACENTER,  // Data center demo load
  RELAY_LOAD_EV,          // EV charging demo load
  RELAY_LOAD_LED_LAMP,    // LED lamp demo load
  RELAY_LOAD_OTHER,       // Other / general demonstration load
  RELAY_COUNT             // Sentinel: total number of relays, not a real one
};

// WHAT: Sets up all 6 relay pins as outputs and leaves every relay
//       de-energized (OFF). Call once from setup().
void relays_init();

// WHAT: Turns one relay on (energized = true) or off (energized = false).
// HOW:  Translates that plain on/off intent into whatever electrical
//       level (HIGH or LOW) this relay module actually needs, using
//       RELAY_ACTIVE_LOW from Config.h -- the caller never has to think
//       about that detail.
void relays_setState(RelayId relay, bool energized);

// WHAT: Returns the last commanded state of a relay (true = energized/
//       circuit closed), in plain on/off terms -- NOT a reading of the
//       physical pin level, which would be inverted if RELAY_ACTIVE_LOW
//       is true.
bool relays_getState(RelayId relay);

#endif
