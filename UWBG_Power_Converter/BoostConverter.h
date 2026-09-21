#ifndef BOOSTCONVERTER_H
#define BOOSTCONVERTER_H

// ---------------------------------------------------------------------------
// BOOST CONVERTER MODULE (public interface)
//
// WHAT: Controls the PWM signal driving the TC4427 gate driver, which in
//       turn switches the GaN device (Renesas TP65H150G4PS) in the boost
//       converter. This block only sets up the PWM output itself; the
//       CC/CV feedback logic that decides WHAT duty cycle to use comes in
//       a later block.
//
// HOW:  boost_init() configures the gate-drive pin as an output and starts
//       it at 0% duty cycle (switch fully off) as a safe default.
//       boost_setDutyCycle() is the only way other code changes the duty
//       cycle, and it clamps the input so nothing outside the module can
//       accidentally command an out-of-range or unsafe value.
//
// WHY:  Same reasoning as the Sensors module: other code (the future CC/CV
//       control loop, or a test routine) should be able to say "set duty
//       cycle to X%" without knowing which pin, which timer, or which
//       analogWrite() range is involved underneath. That also means when we
//       swap this from analogWrite() to a true ~100kHz hardware-timer PWM
//       later, boost_setDutyCycle()'s signature doesn't have to change --
//       only BoostConverter.cpp's internals do.
// ---------------------------------------------------------------------------

void boost_init();
void boost_setDutyCycle(float dutyPercent);

#endif
