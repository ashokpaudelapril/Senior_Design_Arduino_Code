#ifndef BOOSTCONVERTER_H
#define BOOSTCONVERTER_H

// ---------------------------------------------------------------------------
// BOOST CONVERTER MODULE (public interface)
//
// WHAT: Controls the PWM signal driving the TC4427 gate driver, which in
//       turn switches the GaN device (Renesas TP65H150G4PS) in the boost
//       converter. This module only sets up the PWM output itself; the
//       CC/CV logic that decides WHAT duty cycle to use lives in
//       ChargeController.
//
// HOW:  boost_init() configures Timer1 for ~100kHz hardware PWM on the
//       gate-drive pin, starting at 0% duty cycle (switch fully off).
//       boost_setDutyCycle() is the only way other code changes the duty
//       cycle, and it clamps the input so nothing outside the module can
//       accidentally command an out-of-range or unsafe value.
//
// WHY:  Other code (the CC/CV loop, or the manual test mode) should be able
//       to say "set duty cycle to X%" without knowing which pin or which
//       timer registers are involved underneath. The signature stays the
//       same no matter how the PWM is generated internally.
// ---------------------------------------------------------------------------

void boost_init();
void boost_setDutyCycle(float dutyPercent);

#endif
