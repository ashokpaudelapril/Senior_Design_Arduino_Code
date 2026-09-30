// ---------------------------------------------------------------------------
// BOOST CONVERTER MODULE (implementation)
//
// WHAT: Drives the gate-driver PWM pin. boost_init() sets a safe starting
//       state; boost_setDutyCycle() is the only way to change it.
//
// HOW:  Block 5 used analogWrite() as a placeholder (~490-980Hz). Block 8
//       replaces that with direct Timer1 register configuration, because
//       analogWrite() can't go anywhere near the ~100kHz the real boost
//       converter needs. Timer1 is set to "Fast PWM, TOP = ICR1" mode: it
//       counts from 0 up to PWM_TOP_VALUE (159, from Config.h) at the
//       Mega's full 16MHz clock rate, then resets to 0 and repeats -- that
//       repeat rate is our ~100kHz switching frequency. OCR1A sets WHERE
//       in that count the output pin (pin 11 / OC1A) switches from high to
//       low, which is what determines duty cycle: a bigger OCR1A means the
//       pin stays high longer within each cycle.
//
//       Note that boost_setDutyCycle()'s signature hasn't changed at all
//       from Block 5 -- callers (ChargeController) still just pass a 0-100%
//       value and don't know or care that the internals switched from
//       analogWrite() to raw timer registers.
//
// WHY:  boost_setDutyCycle() clamps its input to 0-100% before doing
//       anything else. Without that, a bug anywhere else in the project
//       (e.g. a control-loop calculation going negative or overflowing)
//       could command an out-of-range duty cycle -- on a real boost
//       converter, a stuck-on or miscalculated switch can overheat the
//       GaN device or dump excess voltage onto the battery. Clamping here,
//       in the one function that actually touches the pin, means every
//       caller is protected without having to remember to check it
//       themselves.
// ---------------------------------------------------------------------------

#include <Arduino.h>        // Brings in pinMode() and the AVR register names (TCCR1A, ICR1, OCR1A, etc.)
#include "BoostConverter.h" // This module's own public function declarations
#include "Config.h"         // GATE_DRIVE_PIN, PWM_TOP_VALUE constants

void boost_init() {
  pinMode(GATE_DRIVE_PIN, OUTPUT);   // Configure pin 11 as a digital output (hardware PWM still needs this)

  // --- Configure Timer1 for Fast PWM, non-inverting, TOP = ICR1 ---
  TCCR1A = 0;   // Clear the control register before setting bits (known starting state)
  TCCR1B = 0;   // Same for the second control register

  TCCR1A |= (1 << COM1A1);               // Non-inverting mode on OC1A (pin 11): pin goes LOW at compare match, HIGH at BOTTOM
  TCCR1A |= (1 << WGM11);                // WGM11 + WGM13 + WGM12 together select "Fast PWM, TOP = ICR1" (mode 14)
  TCCR1B |= (1 << WGM13) | (1 << WGM12); // (the remaining two mode-select bits)

  ICR1 = PWM_TOP_VALUE;   // Sets the PWM period -- this is what fixes the frequency at ~100kHz

  TCCR1B |= (1 << CS10);  // Prescaler = 1 (no division): timer counts at the full 16MHz clock rate

  boost_setDutyCycle(0.0);   // Immediately force 0% duty -- switch off -- as the safe starting state
}

void boost_setDutyCycle(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;    // Clamp: never allow negative duty cycle
  if (dutyPercent > 100.0) dutyPercent = 100.0;  // Clamp: never allow more than 100% duty cycle

  uint16_t compareValue = (uint16_t)(dutyPercent / 100.0 * PWM_TOP_VALUE);  // Convert 0-100% into a 0-PWM_TOP_VALUE count
  OCR1A = compareValue;   // Timer1 compares its count to this value each cycle to decide when the pin switches low
}
