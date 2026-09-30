#ifndef CHARGECONTROLLER_H
#define CHARGECONTROLLER_H

// ---------------------------------------------------------------------------
// CHARGE CONTROLLER MODULE (public interface)
//
// WHAT: The CC/CV logic that decides what duty cycle the boost converter
//       should run at, based on the battery sensor's readings vs. our
//       1.0A CC / 14.4V CV targets.
//
// HOW:  chargeController_init() resets to a safe starting state.
//       chargeController_update() runs one control step -- read the
//       battery sensor, decide whether to raise or lower duty cycle, apply
//       it via BoostConverter. It's meant to be called repeatedly (about
//       every CONTROL_LOOP_INTERVAL_MS) from the main loop().
//       chargeController_getDutyCycle() just reports the current commanded
//       duty cycle, for debug printing.
//
// WHY:  This module is the "policy" layer -- it decides WHAT duty cycle to
//       use. BoostConverter is the "mechanism" layer -- it just safely
//       applies whatever duty cycle it's told. Splitting them means we
//       could swap this simple step-based algorithm for a proper PID
//       controller later without touching BoostConverter.cpp at all, and
//       we can test BoostConverter's PWM output independently of whether
//       the control logic is correct.
// ---------------------------------------------------------------------------

void chargeController_init();
void chargeController_update();
float chargeController_getDutyCycle();

// ---------------------------------------------------------------------------
// BLOCK 10: fault latching
//
// WHAT: chargeController_isFaulted() reports whether the controller has
//       tripped into its fault state (output forced off) because either the
//       battery sensor stopped responding, or duty cycle got pinned at its
//       ceiling for too long without ever reaching the CC target current
//       (a sign of an open circuit / disconnected battery).
//
// WHY:  Once faulted, chargeController_update() keeps forcing 0% duty cycle
//       every call and will NOT recover on its own -- this is deliberate: a
//       fault that clears itself and retries automatically could just
//       ramp back up into the same hazard. Exposing this getter lets other
//       code (Serial printing, later the display) show the fault state to
//       whoever's watching, so it's obvious a reset is needed rather than
//       the system silently sitting there doing nothing.
// ---------------------------------------------------------------------------

bool chargeController_isFaulted();

#endif
