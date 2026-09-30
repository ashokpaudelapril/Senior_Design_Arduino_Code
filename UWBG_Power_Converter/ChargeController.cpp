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

static float dutyPercent = 0.0;          // Commanded duty cycle, remembered between update() calls
static bool  faulted = false;            // Once true, stays true until chargeController_init() runs again (e.g. reset)
static unsigned long timeAtMaxDuty = 0;  // millis() timestamp when we FIRST got pinned at the ceiling below target; 0 = not currently pinned

void chargeController_init() {
  dutyPercent  = 0.0;   // Start at 0% -- switch off, same safe default as boost_init()
  faulted      = false; // Clear any previous fault -- this only runs at power-up/reset, so this IS the reset
  timeAtMaxDuty = 0;    // Not pinned at the ceiling yet

  boost_setDutyCycle(dutyPercent);   // Push that starting value out to the hardware immediately
}

// ---------------------------------------------------------------------------
// BLOCK 10: fault detection inside chargeController_update()
//
// WHAT: Two checks added ahead of the normal CC/CV stepping logic:
//         1. Is the battery sensor even responding on I2C right now? If
//            not, we have no real feedback at all -- fault immediately.
//         2. Has duty cycle been sitting at CHARGE_MAX_DUTY_PERCENT for
//            longer than CHARGE_FAULT_TIMEOUT_MS while current still
//            hasn't reached the CC target? That pattern -- pinned at max,
//            current never rising -- is what an open circuit or
//            disconnected battery looks like from the control loop's
//            point of view, since normal charging current rises well
//            before duty cycle maxes out.
//
// HOW:  Once faulted is true, every subsequent call just forces 0% duty and
//       returns immediately -- it does NOT retry or self-clear. The only
//       way out is chargeController_init() running again, which happens on
//       power-up or a manual reset. timeAtMaxDuty works like a mini version
//       of the millis()-based timers in the main .ino: it records WHEN we
//       first got stuck, then a later call checks how long that's been true.
//
// WHY:  A control loop that can't tell "no feedback" apart from "everything
//       reads zero" will happily keep raising duty cycle into an unloaded
//       boost converter, which is exactly the overvoltage hazard flagged
//       back when this control loop was first written. Latching (not
//       auto-retrying) means a real fault can't repeatedly re-trigger the
//       same hazard while nobody's watching.
// ---------------------------------------------------------------------------

void chargeController_update() {
  if (faulted) {
    boost_setDutyCycle(0.0);   // Already faulted -- keep the output off, do nothing else
    return;
  }

  if (!sensors_isBatterySensorConnected()) {
    faulted = true;
    dutyPercent = 0.0;
    boost_setDutyCycle(0.0);
    Serial.println(F("FAULT: battery sensor not responding -- output disabled."));
    return;
  }

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

  bool pinnedBelowTarget = (dutyPercent >= CHARGE_MAX_DUTY_PERCENT) && (battery.current_A < CHARGE_TARGET_CURRENT_A);

  if (pinnedBelowTarget) {
    if (timeAtMaxDuty == 0) {
      timeAtMaxDuty = millis();   // First time we've seen this -- start the clock
    } else if (millis() - timeAtMaxDuty >= CHARGE_FAULT_TIMEOUT_MS) {
      faulted = true;
      dutyPercent = 0.0;
      boost_setDutyCycle(0.0);
      Serial.println(F("FAULT: duty cycle pinned at ceiling without reaching CC target -- possible open circuit. Output disabled."));
      return;
    }
  } else {
    timeAtMaxDuty = 0;   // Not pinned right now -- reset the clock so a later brief pin starts fresh
  }

  boost_setDutyCycle(dutyPercent);   // Apply the updated duty cycle to the gate drive
}

float chargeController_getDutyCycle() {
  return dutyPercent;   // Report the current commanded value, for debug printing
}

bool chargeController_isFaulted() {
  return faulted;   // Report whether the controller has latched into its fault state
}
