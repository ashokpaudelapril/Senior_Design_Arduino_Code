#ifndef LOAD_CONTROL_H
#define LOAD_CONTROL_H

#include "Relays.h"

// ---------------------------------------------------------------------------
// LOAD CONTROL: deciding what each relay should actually do
//
// WHAT: Tracks two separate things per relay:
//         "desired"  -- what the operator asked for, via the touchscreen's
//                        Relays tab (RELAY_SOLAR and RELAY_WIND have no
//                        further check -- desired IS what happens).
//         "actual"   -- what's really energized right now (just a mirror
//                        of Relays.cpp's own state -- see loadControl_getActual()).
//       For the 4 demo load relays (data center, EV, LED lamp, other),
//       desired and actual can DIFFER: tapping a load's button ON only
//       records that the operator wants it on. Whether it actually
//       energizes still depends on loadControl_update()'s automatic
//       "enough power available" check.
//
// HOW:  loadControl_setDesired() is called from Display.cpp's touch
//       handler whenever a relay button is tapped. loadControl_update()
//       is called every loop() pass (cheap -- a handful of comparisons and
//       digitalWrite() calls) and is the ONLY place that actually calls
//       relays_setState().
//
// WHY:  This is the "policy" layer on top of Relays.cpp's pure mechanism,
//       same split as ChargeController sits on top of BoostConverter. It's
//       also the one place that knows the load relays have an automatic
//       override and the solar/wind relays don't -- nothing else in the
//       firmware needs to know that distinction.
// ---------------------------------------------------------------------------

void loadControl_init();                                 // Resets all "desired" states to OFF
void loadControl_setDesired(RelayId relay, bool desiredOn);  // Records operator intent (from a touch button)
bool loadControl_getDesired(RelayId relay);               // What the operator last asked for
bool loadControl_getActual(RelayId relay);                // What's really energized right now
void loadControl_update();                                // Re-evaluates power availability and drives the relays

#endif
