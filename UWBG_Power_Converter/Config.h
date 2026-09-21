#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// CONFIG: shared hardware addresses and calibration constants
//
// WHAT: One place for hardware values that multiple modules need (right now
//       just Sensors.cpp, later BoostConverter.cpp and Display.cpp too),
//       instead of repeating magic numbers in every file.
//
// HOW:  Plain #define constants. No RAM cost, and any .cpp file that
//       #includes "Config.h" can see them.
//
// WHY:  If a sensor address or the shunt resistor value changes (e.g. when
//       we wire in the second INA226 for the wind branch), we edit it here
//       once instead of hunting through every module that uses it.
// ---------------------------------------------------------------------------

#define INA226_SOLAR_ADDR   0x40    // Solar array output sensor, A0=A1=GND
#define INA226_SHUNT_OHMS   0.010   // CJMCU-226 board shunt resistor value
#define INA226_MAX_AMPS     2.0     // Expected major-path current, per block diagram (0-2A)

// ---------------------------------------------------------------------------
// BLOCK 6: battery/charge-output sensor address
//
// WHAT: I2C address of the second INA226, placed at the boost converter's
//       output (battery/charging side) instead of the solar array.
//
// HOW:  Each INA226 board's address is set by two solder-jumper pins (A0,
//       A1). The default (both tied to GND) is 0x40 -- already used by the
//       solar sensor -- so this second board needs a DIFFERENT address or
//       both sensors would collide on the I2C bus. Per the INA226 address
//       table, bridging the A0 jumper to VS (instead of GND) gives 0x41.
//       Verify this against your specific board's silkscreen/datasheet once
//       it's in hand -- the jumper labeling can vary slightly by supplier.
//
// WHY:  The CC/CV charging algorithm (Block 7) needs to regulate the actual
//       current/voltage going INTO the battery, not the solar array's
//       output -- those two are different once the boost converter is
//       stepping voltage/current up between them. We need this second,
//       independent measurement point to control against.
// ---------------------------------------------------------------------------
#define INA226_BATTERY_ADDR 0x41   // Battery/charge-output sensor, A0 jumper set to VS

// ---------------------------------------------------------------------------
// BLOCK 4: loop timing
//
// WHAT: How often (in milliseconds) we print a solar reading to Serial.
//
// WHY:  Named here instead of a bare "1000" in the .ino so it reads as
//       "print interval" wherever it's used, and so it's easy to speed up
//       later (e.g. for tighter debugging) without hunting through loop().
// ---------------------------------------------------------------------------
#define SENSOR_PRINT_INTERVAL_MS 1000

// ---------------------------------------------------------------------------
// BLOCK 5: boost converter gate drive pin
//
// WHAT: The Mega pin that carries the PWM signal into the TC4427 gate
//       driver, which switches the GaN device.
//
// WHY:  Pin 11 is one of the Mega's 16-bit timer pins (Timer1). We're using
//       analogWrite() on it for now (Block 5), but choosing a 16-bit-timer
//       pin now means when we upgrade to true ~100kHz Fast PWM in a later
//       block, we reconfigure Timer1's registers on this SAME pin instead
//       of needing to move the wire to the gate driver.
// ---------------------------------------------------------------------------
#define GATE_DRIVE_PIN 11

// ---------------------------------------------------------------------------
// BLOCK 7: CC/CV charge control targets and tuning
//
// WHAT: The charging targets from the block diagram (1.0A constant current,
//       14.4V constant voltage cap), plus tuning values for how the control
//       loop steps the duty cycle toward them.
//
// HOW:  CHARGE_DUTY_STEP_PERCENT is how much the duty cycle changes on each
//       control tick -- small enough to avoid big current/voltage swings,
//       large enough to actually respond within a reasonable time.
//       CHARGE_MAX_DUTY_PERCENT is a safety ceiling well below 100%: a
//       boost converter approaching 100% duty cycle stops boosting
//       properly and can pull excessive input current, so we cap it here
//       independently of BoostConverter's own hard 0-100% clamp.
//       CONTROL_LOOP_INTERVAL_MS sets how often the control loop re-checks
//       sensors and adjusts duty cycle.
//
// WHY:  These are named constants (not magic numbers buried in
// ChargeController.cpp) so the charging behavior can be tuned/retuned in
//       one place, and so the 1.0A / 14.4V figures visibly match the
//       project spec from the block diagram rather than looking arbitrary.
// ---------------------------------------------------------------------------
#define CHARGE_TARGET_CURRENT_A   1.0    // CC target: 1.0 A into the battery
#define CHARGE_TARGET_VOLTAGE_V   14.4   // CV target: 14.4 V cap at the battery
#define CHARGE_DUTY_STEP_PERCENT  0.5    // Duty cycle change per control tick
#define CHARGE_MAX_DUTY_PERCENT   80.0   // Safety ceiling, well below 100%
#define CONTROL_LOOP_INTERVAL_MS  50     // Run the control loop 20 times/sec

#endif
