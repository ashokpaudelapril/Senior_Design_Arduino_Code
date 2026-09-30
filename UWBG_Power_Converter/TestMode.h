#ifndef TESTMODE_H
#define TESTMODE_H

// ---------------------------------------------------------------------------
// TEST MODE MODULE (public interface)
//
// WHAT: A manual override that lets you set the boost converter's duty
//       cycle directly by typing a number into the Serial Monitor, instead
//       of letting ChargeController's CC/CV algorithm decide it.
//
// HOW:  testMode_update() checks for and handles Serial input; call it
//       repeatedly from loop() while MANUAL_PWM_TEST_MODE is true.
//
// WHY:  This exists purely for bench testing -- verifying the PWM
//       frequency/duty cycle on an oscilloscope without depending on the
//       battery sensor or the CC/CV logic being correct yet. It's a
//       diagnostic tool, not part of the normal operating firmware.
// ---------------------------------------------------------------------------

void testMode_update();

#endif
