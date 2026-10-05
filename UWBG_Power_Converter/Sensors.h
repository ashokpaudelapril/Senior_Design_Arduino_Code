#ifndef SENSORS_H
#define SENSORS_H

// ---------------------------------------------------------------------------
// SENSORS MODULE (public interface)
//
// WHAT: The functions other modules are allowed to call to work with the
//       power-monitoring sensors: solar, wind, common DC bus, battery/charge
//       output, and load (placeholder until the buck converter exists).
//
// HOW:  sensors_init() brings up I2C and calibrates each sensor -- call it
//       once from setup(). Other modules only see these function
//       signatures, not the INA226_WE objects themselves (those live in
//       Sensors.cpp).
//
// WHY:  Hiding the sensor objects behind functions means the boost-converter
//       control loop and the display code can ask "what's the solar power
//       right now?" without knowing anything about I2C addresses or the
//       INA226 library. If we ever swap sensor hardware, only Sensors.cpp
//       needs to change -- nothing else in the project does.
// ---------------------------------------------------------------------------

void sensors_init();

// ---------------------------------------------------------------------------
// BLOCK 3: reading a sensor
//
// WHAT: A struct bundling the three values one INA226 reading gives us
//       (bus voltage, current, power).
//
// HOW:  Each read function (sensors_readSolar(), sensors_readBattery(), ...)
//       takes one I2C snapshot from its sensor and returns all three
//       numbers together in a PowerReading.
//
// WHY:  A struct return keeps the three numbers from one sensor together as
//       a single reading, instead of three separate global variables (which
//       risks mixing values from different points in time) or the caller
//       juggling three separate function calls. Naming it generically
//       (PowerReading, not SolarReading) lets every sensor -- solar,
//       battery, and whichever ones follow -- share the same struct instead
//       of us defining a near-identical one per sensor.
// ---------------------------------------------------------------------------

struct PowerReading {
  float busVoltage_V;   // Bus voltage at this sensor's location, in volts
  float current_A;      // Current through this sensor's shunt, in amps
  float power_W;        // Power at this sensor's location, in watts
};

PowerReading sensors_readSolar();

// ---------------------------------------------------------------------------
// BLOCK 6: reading the battery/charge-output sensor
//
// WHAT: sensors_readBattery() takes a reading from the second INA226,
//       placed at the boost converter's output (battery/charging side).
//
// WHY:  This is what the CC/CV charging algorithm (ChargeController) regulates
//       against -- it needs the actual voltage/current reaching the battery,
//       not the solar array's output.
// ---------------------------------------------------------------------------

PowerReading sensors_readBattery();

// ---------------------------------------------------------------------------
// BLOCK 11: reading the wind branch sensor
//
// WHAT: sensors_readWind() takes a reading from the third INA226, placed on
//       the wind branch after its SB540 isolation diode, mirroring where
//       the solar sensor sits relative to the solar branch's diode.
//
// WHY:  Lets us monitor the wind generation path independently -- the
//       block diagram's wind system target is ~2-5W combined, and this is
//       how we'd actually measure it once the wind drive motor/generators
//       are wired up.
// ---------------------------------------------------------------------------

PowerReading sensors_readWind();

// ---------------------------------------------------------------------------
// BLOCK 15: reading the common DC bus sensor
//
// WHAT: sensors_readCommonBus() takes a reading from the fourth INA226,
//       placed after both isolation diodes combine, right before the boost
//       converter's actual input.
//
// WHY:  This is the true boost-converter input measurement. It sits after the
//       isolation diodes, so its reading already includes their forward-voltage
//       drop, which is what Efficiency.cpp uses as input power.
// ---------------------------------------------------------------------------

PowerReading sensors_readCommonBus();

// ---------------------------------------------------------------------------
// BLOCK 17: reading the load sensor -- PLACEHOLDER, see Config.h
//
// WHAT: sensors_readLoad() takes a reading from the fifth INA226, intended
//       for the buck converter's load-side output.
//
// WHY THIS IS A PLACEHOLDER: no buck converter hardware exists yet (see
//       Config.h's INA226_LOAD_ADDR comment) -- this function exists so the
//       pattern is ready, not because there's a real reading to trust yet.
// ---------------------------------------------------------------------------

PowerReading sensors_readLoad();

// ---------------------------------------------------------------------------
// BLOCK 10: sensor presence check
//
// WHAT: Checks whether the battery/charge-output sensor is actually
//       responding on the I2C bus right now, without reading its data.
//
// WHY:  sensors_readBattery() has no way to tell "no sensor here" apart
//       from "sensor here, reporting near-zero" -- both can come back
//       looking like 0V/0A. The CC/CV control loop (ChargeController) needs
//       to know the difference: a missing/disconnected sensor means it's
//       flying blind and should shut the output off, not keep ramping duty
//       cycle based on a reading that isn't real.
// ---------------------------------------------------------------------------

bool sensors_isBatterySensorConnected();

#endif
