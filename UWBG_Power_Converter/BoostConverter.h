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
// HOW:  boost_init() configures a hardware timer for ~100kHz PWM on the
//       gate-drive pin, starting at 0% duty cycle (switch fully off). On
//       the Giga R1 WiFi this is STM32's TIM8 Channel 2, configured
//       directly via STM32 HAL (the Mega's original AVR Timer1 code is
//       what this replaced when the project moved to the Giga).
//       boost_setDutyCycle() is the only way other code changes the duty
//       cycle, and it clamps the input so nothing outside the module can
//       accidentally command an out-of-range or unsafe value.
//
// WHY:  Other code (the CC/CV loop, or the manual test mode) should be able
//       to say "set duty cycle to X%" without knowing which pin or which
//       timer registers are involved underneath. The signature stays the
//       same no matter how the PWM is generated internally -- proven by the
//       Mega-to-Giga board change itself: only this module's internals
//       needed to change, not ChargeController or TestMode.
// ---------------------------------------------------------------------------

void boost_init();
void boost_setDutyCycle(float dutyPercent);

#endif
