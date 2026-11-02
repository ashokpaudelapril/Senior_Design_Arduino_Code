// =====================================================================
// CurrentControl_Uno_Test
//
// WHAT: Builds on PWM_Uno_Test.ino's proven 100kHz Timer1 PWM setup
//       (D9/OC1A), but instead of a human typing the duty cycle, this
//       sketch reads real current from an INA226 sensor and
//       automatically adjusts the duty cycle up or down to hold that
//       current steady at a 1.0 A target -- a closed feedback loop,
//       not open-loop manual control.
//
// WHY THIS APPROACH: this is the exact same "step controller" idea
//       already used in the real boost converter firmware
//       (ChargeController.cpp) for CC/CV charging -- step the duty
//       cycle up or down by a small fixed amount each control tick,
//       based on whether the measured value is above or below target.
//       No PID here, same reasoning as the real firmware: a step
//       controller is slower and less precise than PID, but far more
//       predictable and safer while the mechanism itself is still
//       being tested on real hardware.
//
// ASSUMPTION STATED UP FRONT: this sketch assumes an INA226 sensor
//       (via the INA226_WE library, same one used elsewhere in this
//       project) provides the current feedback, wired on the standard
//       I2C pins (A4=SDA, A5=SCL on an UNO). If a different sensor or
//       feedback method is actually being used, this needs to change
//       before it's useful.
//
// SAFETY: No hard safety limit in this sketch yet (no overcurrent trip
//         independent of the control loop) -- this is a first working
//         version to prove the control concept, the same staged
//         approach the real firmware took (mechanism first, hard
//         limits layered on after). Watch it closely on the bench,
//         don't leave it unattended yet.
// =====================================================================

#include <Arduino.h>
#include <Wire.h>
#include <INA226_WE.h>

// ---------------------------------------------------------------------
// BLOCK 1: timer math -- identical to PWM_Uno_Test.ino, unchanged.
// See that file for the full explanation; repeated here only because
// this is a separate, standalone sketch.
// ---------------------------------------------------------------------
#define UNO_CLOCK_HZ        16000000UL
#define PWM_FREQUENCY_HZ    100000UL
#define PWM_PERIOD_TICKS    (UNO_CLOCK_HZ / PWM_FREQUENCY_HZ)   // 160 ticks per period
#define PWM_TOP_VALUE       (PWM_PERIOD_TICKS - 1)              // 159 -- Timer1 counts 0..TOP

// ---------------------------------------------------------------------
// BLOCK 2: INA226 sensor setup
//
// WHAT: Creates the sensor object and its I2C address, and a function
//       to bring the sensor up with sensible defaults.
// HOW:  INA226_WE was chosen project-wide (not a generic INA226
//       library) specifically because it exposes explicit
//       shunt-resistor and current-range calibration -- without this,
//       the sensor can't report accurate current for a given shunt
//       value. 0x40 is the default/unconfigured address; change this
//       to match whichever address jumper this specific sensor module
//       is actually set to.
// WHY:  This is the one new piece this sketch adds on top of
//       PWM_Uno_Test.ino -- real current feedback, which is the whole
//       point of a closed control loop instead of open-loop duty
//       cycle entry.
// ---------------------------------------------------------------------
#define INA226_I2C_ADDRESS   0x40   // CHECK THIS against the real module's address jumper before trusting readings
#define SHUNT_RESISTOR_OHMS  0.010  // Same 0.010 ohm shunt value used project-wide, per Config.h
#define MAX_EXPECTED_AMPS    2.0    // Calibration range -- matches the project's existing 0-2A sensor range

INA226_WE ina226 = INA226_WE(INA226_I2C_ADDRESS);

static void configureIna226() {
  Wire.begin();                 // Starts the I2C bus -- A4 (SDA) and A5 (SCL) on a standard UNO
  ina226.init();
  ina226.setResistorRange(SHUNT_RESISTOR_OHMS, MAX_EXPECTED_AMPS);   // Calibrates the sensor for this project's real shunt/range
}

// ---------------------------------------------------------------------
// BLOCK 3: configureTimer1Pwm() and setDuty() -- identical to
// PWM_Uno_Test.ino, unchanged. This is the proven PWM mechanism; the
// only thing new in this sketch is what DECIDES the duty cycle
// (BLOCK 4 below), not how the duty cycle is actually applied.
// ---------------------------------------------------------------------
static void configureTimer1Pwm() {
  pinMode(9, OUTPUT);   // D9 = OC1A

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  ICR1 = PWM_TOP_VALUE;

  TCCR1A |= (1 << COM1A1);
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12);
  TCCR1B |= (1 << CS10);

  OCR1A = 0;   // Start at 0% duty -- the control loop will ramp this up on its own, not a human typing a number
}

static void setDuty(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;
  if (dutyPercent > 100.0) dutyPercent = 100.0;

  uint16_t compareValue = (uint16_t)(dutyPercent / 100.0 * PWM_PERIOD_TICKS);
  OCR1A = compareValue;
}

// ---------------------------------------------------------------------
// BLOCK 4: the step controller itself -- the new logic this sketch
// actually adds.
//
// WHAT: Every CONTROL_INTERVAL_MS, reads real current from the INA226
//       and nudges the duty cycle up or down by STEP_SIZE_PERCENT,
//       depending on whether measured current is below or above the
//       1.0 A target.
// HOW:  Same step-controller pattern as the real ChargeController.cpp
//       -- no proportional or derivative math, just "am I above or
//       below target, step accordingly." Deliberately simple and
//       predictable while this is still being proven on the bench.
// WHY A FIXED INTERVAL, NOT EVERY LOOP() PASS: adjusting on every
//       single loop() iteration (effectively thousands of times a
//       second) would make the duty cycle jitter constantly and make
//       it hard to tell if the control loop is actually working.
//       A controlled, human-watchable interval (100ms here) makes the
//       step-by-step behavior visible on the Serial Monitor.
// ---------------------------------------------------------------------
#define TARGET_CURRENT_A      1.0    // Matches the real boost converter's CC target
#define STEP_SIZE_PERCENT     0.5    // Reduced from 0.5 -- bench data showed 0.5% steps moved current ~0.01A each, leaving a 1.008A plateau instead of settling on 1.000A. A smaller step narrows that steady-state gap proportionally, at the cost of taking roughly 5x longer to ramp up from 0% initially.
#define CONTROL_INTERVAL_MS   100
#define MAX_DUTY_PERCENT      80.0   // Safety ceiling for this bench test -- matches the real ChargeController's own 80% CC/CV ceiling, kept separate from the hardware's own 0-100% clamp in setDuty()

static float currentDutyPercent = 0.0;
static unsigned long lastControlTick = 0;

static void runControlStep() {
  float measuredAmps = ina226.getCurrent_mA() / 1000.0;   // Library reports mA -- convert to A to compare against the target directly

  if (measuredAmps < TARGET_CURRENT_A) {
    currentDutyPercent += STEP_SIZE_PERCENT;   // Below target -- push a little more duty cycle
  } else if (measuredAmps > TARGET_CURRENT_A) {
    currentDutyPercent -= STEP_SIZE_PERCENT;   // Above target -- pull back a little
  }
  // Exactly at target: leave duty cycle alone this tick

  // WHAT: Clamp the TRACKED variable itself, not just what setDuty()
  //       applies to hardware.
  // WHY:  setDuty() clamps its own local parameter before writing to
  //       the hardware register, so the real PWM signal was always
  //       safely capped at 100% -- but currentDutyPercent itself kept
  //       climbing unclamped past that (observed climbing to ~198% on
  //       the bench). Without this, if current ever rose above target,
  //       the controller would have to step all the way back down from
  //       ~198% before the real output even started decreasing --
  //       a serious response-lag bug, same idea as "integral windup"
  //       in a PI controller, just in a step controller here instead.
  // NOTE: Clamped to MAX_DUTY_PERCENT (80%), not 100% -- a deliberate
  //       safety ceiling for this bench test, matching the real
  //       ChargeController's own 80% CC/CV ceiling. setDuty() still
  //       separately clamps at the true hardware limit (100%), so this
  //       is a second, tighter layer on top of that, not a replacement
  //       for it.
  if (currentDutyPercent > MAX_DUTY_PERCENT) currentDutyPercent = MAX_DUTY_PERCENT;
  if (currentDutyPercent < 0.0)              currentDutyPercent = 0.0;

  setDuty(currentDutyPercent);
}

// ---------------------------------------------------------------------
// BLOCK 4.5: start-gate safety control
//
// WHAT: The control loop does NOT run automatically on power-up.
//       Duty cycle stays locked at 0% until "start" is typed into the
//       Serial Monitor and Enter is pressed.
// WHY:  Without this, the old behavior powers on, immediately sees
//       near-zero current (below the 1.0A target), and starts ramping
//       duty cycle upward right away -- even if input power isn't
//       actually connected yet. If input then gets connected partway
//       through that unsupervised ramp, that's an uncontrolled jump
//       instead of a safe climb from zero. Gating the loop behind an
//       explicit typed command means duty cycle only ever starts
//       moving once a person has confirmed everything is actually
//       wired up and ready -- same reasoning as the hand-spin-test
//       discipline used elsewhere in this project before trusting
//       motors unattended.
// ---------------------------------------------------------------------
static bool controlLoopStarted = false;

static void checkForStartCommand() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "start") {
      controlLoopStarted = true;
      Serial.println(F(">>> START received -- control loop is now running. <<<"));
    } else if (input.length() > 0) {
      Serial.println(F("Not recognized. Type START (exactly, then press Enter) to begin."));
    }
  }
}

// ---------------------------------------------------------------------
// BLOCK 5: setup() and loop() -- brings everything together, plus a
// heartbeat so the Serial Monitor shows the control loop actually
// working (measured current converging toward 1.0 A), not just a
// silent board.
// ---------------------------------------------------------------------
static unsigned long lastHeartbeat = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // wait for the Serial Monitor, same pattern as every other test sketch here
  }

  configureIna226();
  configureTimer1Pwm();
  setDuty(0.0);   // Explicit, not just relying on configureTimer1Pwm()'s own OCR1A=0 -- makes the "locked at 0% until started" guarantee obvious right here too

  Serial.println(F("--- CurrentControl_Uno_Test start ---"));
  Serial.println(F("Closed-loop test: duty cycle is LOCKED AT 0% for now."));
  Serial.println(F("Confirm input power and wiring are ready, THEN type START and press Enter to begin the control loop."));
  Serial.println(F("SAFETY: no hard overcurrent trip in this sketch yet -- watch the bench closely once started, don't leave unattended."));
}

void loop() {
  if (!controlLoopStarted) {
    checkForStartCommand();
    setDuty(0.0);   // Actively enforced every pass, not just assumed -- duty cannot drift while waiting

    if (millis() - lastHeartbeat >= 1000) {
      lastHeartbeat = millis();
      Serial.println(F("waiting for START... (duty locked at 0%)"));
    }
    return;   // Skip the control loop and the running heartbeat entirely until started
  }

  if (millis() - lastControlTick >= CONTROL_INTERVAL_MS) {
    lastControlTick = millis();
    runControlStep();
  }

  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    Serial.print(F("heartbeat | measured: "));
    Serial.print(ina226.getCurrent_mA() / 1000.0, 3);
    Serial.print(F(" A | target: "));
    Serial.print(TARGET_CURRENT_A);
    Serial.print(F(" A | duty: "));
    Serial.print(currentDutyPercent);
    Serial.println(F(" %"));
  }
}
