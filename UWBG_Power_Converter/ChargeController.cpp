// ---------------------------------------------------------------------------
// CHARGE CONTROLLER MODULE (implementation)
//
// WHAT: Implements the CC/CV step-control algorithm described in
//       ChargeController.h.
//
// HOW:  Each call to chargeController_update() takes ONE battery reading
//       and moves the commanded duty cycle by one small step
//       (CHARGE_DUTY_STEP_PERCENT) in whichever direction moves us toward
//       the targets:
//         - If battery voltage has reached the 14.4V CV cap: step duty
//           cycle DOWN, regardless of current. Voltage is the constraint
//           now (CV phase) -- letting duty cycle keep rising would push
//           voltage past the cap.
//         - Else if battery current is still below the 1.0A CC target:
//           step duty cycle UP. We're in CC phase, still trying to reach
//           full charge current, and voltage has headroom below 14.4V.
//         - Else (current at/above target, voltage still below cap): step
//           duty cycle DOWN slightly to hold current near the 1.0A target
//           instead of overshooting it.
//       The result is clamped to [0, CHARGE_MAX_DUTY_PERCENT] before being
//       sent to boost_setDutyCycle() (which applies its own [0,100] clamp
//       as a second safety net).
//
// WHY:  This is a simple incremental ("integral-only") controller rather
//       than a full PID -- it has no proportional or derivative term, so it
//       responds slowly and smoothly rather than reacting hard to sudden
//       sensor changes. That's a deliberate simplification for a first
//       working version: it's easy to reason about and tune (one step-size
//       constant), and slow, cautious response is safer to test with real
//       hardware than a more aggressive controller we haven't validated
//       yet. It can be replaced with a proper PID controller later without
//       changing anything outside this file.
// ---------------------------------------------------------------------------

#include "ChargeController.h"
#include "BoostConverter.h"
#include "Sensors.h"
#include "Config.h"

static float dutyPercent = 0.0;   // Commanded duty cycle, remembered between update() calls

void chargeController_init() {
  dutyPercent = 0.0;             // Start at 0% -- switch off, same safe default as boost_init()
  boost_setDutyCycle(dutyPercent);   // Push that starting value out to the hardware immediately
}

void chargeController_update() {
  PowerReading battery = sensors_readBattery();   // One fresh snapshot of battery voltage/current

  if (battery.busVoltage_V >= CHARGE_TARGET_VOLTAGE_V) {
    // CV phase: at/above the voltage cap -- back off to avoid overvoltage
    dutyPercent -= CHARGE_DUTY_STEP_PERCENT;
  } else if (battery.current_A < CHARGE_TARGET_CURRENT_A) {
    // CC phase: below target current, and voltage still has headroom -- push harder
    dutyPercent += CHARGE_DUTY_STEP_PERCENT;
  } else {
    // At/above target current already, but voltage still under the cap -- ease back slightly
    dutyPercent -= CHARGE_DUTY_STEP_PERCENT;
  }

  if (dutyPercent < 0.0)                   dutyPercent = 0.0;                   // Clamp: never negative
  if (dutyPercent > CHARGE_MAX_DUTY_PERCENT) dutyPercent = CHARGE_MAX_DUTY_PERCENT; // Clamp: stay under our safety ceiling

  boost_setDutyCycle(dutyPercent);   // Apply the updated duty cycle to the gate drive
}

float chargeController_getDutyCycle() {
  return dutyPercent;   // Report the current commanded value, for debug printing
}
