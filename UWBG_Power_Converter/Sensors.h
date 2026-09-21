#ifndef SENSORS_H
#define SENSORS_H

// ---------------------------------------------------------------------------
// SENSORS MODULE (public interface)
//
// WHAT: The functions other modules are allowed to call to work with the
//       power-monitoring sensors. Right now that's just the solar-array
//       INA226; more sensors (wind branch, common bus, battery, etc.) will
//       get their own init/read entries here as we bring each one online.
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
// WHY:  This is what the CC/CV charging algorithm (Block 7) will regulate
//       against -- it needs to know the actual voltage/current reaching the
//       battery, not the solar array's output.
// ---------------------------------------------------------------------------

PowerReading sensors_readBattery();

#endif
