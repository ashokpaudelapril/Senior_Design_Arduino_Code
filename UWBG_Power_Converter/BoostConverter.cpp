// ---------------------------------------------------------------------------
// BOOST CONVERTER MODULE (implementation)
//
// WHAT: Drives the gate-driver PWM pin. boost_init() sets a safe starting
//       state; boost_setDutyCycle() is the only way to change it.
//
// HOW:  analogWrite() takes a 0-255 value, not a percentage, so
//       boost_setDutyCycle() converts the 0-100% input into that 0-255
//       range. This is a placeholder ~490-980Hz PWM (Arduino's default on
//       this pin) -- NOT the ~100kHz the real boost converter needs. It's
//       here so we can build and test the CC/CV control logic (which just
//       calls boost_setDutyCycle() with a percentage) before adding the
//       more complex hardware-timer code for true switching frequency.
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

#include <Arduino.h>       // Brings in pinMode(), analogWrite(), etc.
#include "BoostConverter.h" // This module's own public function declarations
#include "Config.h"         // GATE_DRIVE_PIN constant

void boost_init() {
  pinMode(GATE_DRIVE_PIN, OUTPUT);   // Configure the gate-drive pin as a digital output
  boost_setDutyCycle(0.0);           // Immediately force 0% duty -- switch off -- as the safe starting state
}

void boost_setDutyCycle(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;    // Clamp: never allow negative duty cycle
  if (dutyPercent > 100.0) dutyPercent = 100.0;  // Clamp: never allow more than 100% duty cycle

  int pwmValue = (int)(dutyPercent / 100.0 * 255.0);  // Convert 0-100% into analogWrite()'s 0-255 range
  analogWrite(GATE_DRIVE_PIN, pwmValue);              // Output the PWM signal on the gate-drive pin
}
