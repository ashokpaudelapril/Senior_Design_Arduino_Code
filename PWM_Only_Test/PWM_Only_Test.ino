// ---------------------------------------------------------------------------
// PWM_Only_Test.ino -- bench test for the boost gate-drive PWM (pin 11)
//
// WHAT: Drives the 100 kHz Timer1 PWM from BoostConverter and lets you set
//       the duty cycle by typing a number in the Serial Monitor. Nothing else
//       is used: no sensors, no display, no charge controller, no fault logic.
//
// HOW:  boost_init() sets up Timer1 and starts at 0%. Each typed number is
//       passed to boost_setDutyCycle(), which clamps it to 0-100%.
//
// WHY:  This avoids the display and INA226 libraries, which aren't installed
//       yet, so the PWM can be checked on the scope now. There are no safety
//       limits in this sketch, so keep the gate driver and GaN switch
//       disconnected while probing.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include "BoostConverter.h"

void setup() {
  Serial.begin(9600);   // 9600 baud to match the main firmware
  boost_init();         // Timer1 configured for ~100 kHz, output at 0%

  Serial.println(F("PWM-only test. Type a duty cycle 0-100 and press Enter."));
}

void loop() {
  if (Serial.available() > 0) {               // A line has been typed
    float requested = Serial.parseFloat();    // Read the number, e.g. 25
    while (Serial.available() > 0) {          // Discard the newline
      Serial.read();
    }
    boost_setDutyCycle(requested);            // Applies the duty, clamped to 0-100
    Serial.print(F("Duty set to: "));
    Serial.print(requested);
    Serial.println(F(" %"));
  }
}
