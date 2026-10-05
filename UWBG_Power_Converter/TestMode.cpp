// ---------------------------------------------------------------------------
// TEST MODE MODULE (implementation)
//
// WHAT: Reads a duty-cycle percentage typed into the Serial Monitor and
//       applies it directly via boost_setDutyCycle().
//
// HOW:  Serial.available() is nonzero once at least one byte has arrived
//       from the Serial Monitor. Serial.parseFloat() reads and converts the
//       digits it finds into a float, skipping any leading non-numeric
//       characters -- this is what lets you just type "25" and press Enter
//       in the Serial Monitor. Anything left over afterward (like the
//       newline character the Serial Monitor sends) is drained in the
//       while loop so it doesn't get misread as the start of the next
//       number.
//
// WHY:  boost_setDutyCycle() already clamps its input to 0-100%, so this
//       function doesn't need to validate the typed value itself -- typing
//       a bad number (negative, too large, or garbage that parses as 0)
//       can't command anything unsafe to the gate drive.
//
// BLOCK 16 addition: refuses to apply any typed duty cycle once
//       ChargeController has latched a fault (e.g. a hard overvoltage/
//       overcurrent trip from chargeController_checkHardLimits()).
//       Without this check, a fault could still be overridden for one
//       loop() pass by whatever the user last typed, before the hard-limit
//       check (which runs after this function each pass) forces it back to
//       0% -- a brief, avoidable unsafe pulse. Refusing here instead closes
//       that gap outright: once faulted, typed input has no effect until a
//       reset.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include "TestMode.h"
#include "BoostConverter.h"
#include "ChargeController.h"

void testMode_update() {
  if (Serial.available() > 0) {               // Only act once the user has typed something and pressed Enter
    float requestedDuty = Serial.parseFloat(); // Read the number they typed, e.g. 25.0 for 25%

    while (Serial.available() > 0) {   // Clear out any leftover characters (like the trailing newline)
      Serial.read();
    }

    if (chargeController_isFaulted()) {   // Refuse to apply -- a fault (e.g. hard limit trip) requires a reset first
      Serial.println(F("Ignored: system is in a FAULT state. Reset the board to clear it before setting duty cycle."));
      return;
    }

    boost_setDutyCycle(requestedDuty);   // Apply it directly -- still clamped to 0-100% inside BoostConverter

    Serial.print(F("Manual duty cycle set to: "));   // Confirm back so you know it was received correctly
    Serial.print(requestedDuty);
    Serial.println(F(" %"));
  }
}
