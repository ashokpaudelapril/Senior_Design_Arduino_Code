// ---------------------------------------------------------------------------
// SENSORS MODULE (implementation)
//
// WHAT: Sets up I2C and the solar-array INA226 sensor object, and exposes
//       sensors_init() to bring it online.
//
// HOW:  INA226_WE measures the voltage across a known shunt resistor to
//       derive current (I = V_shunt / R_shunt), then combines that with bus
//       voltage for power. setResistorRange() tells the library our shunt
//       value (0.010 ohm) and the max current we expect to see (2.0 A) so
//       it sizes its internal calibration register correctly -- get this
//       wrong and current/power readings come out scaled incorrectly even
//       though voltage still looks fine.
//
// WHY:  This lives in its own .cpp file (not the main .ino) so the sensor
//       object (inaSolar) and its setup details are private to this module
//       -- nothing outside Sensors.cpp/.h needs to know an INA226_WE object
//       exists at all.
// ---------------------------------------------------------------------------

#include <Wire.h>          // I2C bus library
#include <INA226_WE.h>     // INA226 sensor driver library
#include "Sensors.h"       // This module's own public declarations (PowerReading, etc.)
#include "Config.h"        // Sensor address + calibration constants

INA226_WE inaSolar   = INA226_WE(INA226_SOLAR_ADDR);    // Sensor object bound to the solar sensor's I2C address
INA226_WE inaBattery = INA226_WE(INA226_BATTERY_ADDR);  // Sensor object bound to the battery/charge-output sensor's I2C address

void sensors_init() {
  Wire.begin();   // Join I2C bus as controller (Mega: SDA=pin20, SCL=pin21)

  inaSolar.init();                                              // Reset/configure the solar INA226 chip over I2C
  inaSolar.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Tell the library our shunt value + expected max current

  inaBattery.init();                                              // Reset/configure the battery INA226 chip over I2C
  inaBattery.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Same shunt/current-range values -- identical board

  Serial.println(F("Solar and battery INA226 sensors initialized."));   // Confirm setup completed, for debugging over Serial
}

// ---------------------------------------------------------------------------
// BLOCK 3: sensors_readSolar()
//
// WHAT: Takes one reading from the solar INA226 and returns it as a
//       PowerReading (bus voltage, current, power).
//
// HOW:  The INA226_WE library exposes getBusVoltage_V() directly in volts,
//       but getCurrent_mA() and getBusPower() return milliamps/milliwatts
//       -- so we divide each by 1000 to keep our whole project working in
//       consistent base units (volts, amps, watts).
//
// WHY:  Base units (V, A, W) everywhere means no code calling this function
//       has to remember "oh, this one's in milli-something" -- one
//       conversion here instead of scattered /1000s throughout the project.
// ---------------------------------------------------------------------------

PowerReading sensors_readSolar() {
  PowerReading reading;                                     // Create an empty struct to fill in below
  reading.busVoltage_V = inaSolar.getBusVoltage_V();         // Already in volts -- no conversion needed
  reading.current_A    = inaSolar.getCurrent_mA() / 1000.0;  // Library gives mA -- divide by 1000 for amps
  reading.power_W       = inaSolar.getBusPower() / 1000.0;   // Library gives mW -- divide by 1000 for watts
  return reading;                                            // Hand all three values back to the caller together
}

// ---------------------------------------------------------------------------
// BLOCK 6: sensors_readBattery()
//
// WHAT: Takes one reading from the battery/charge-output INA226 and returns
//       it as a PowerReading, the same shape as sensors_readSolar().
//
// HOW:  Identical logic to sensors_readSolar(), just against the inaBattery
//       object/address instead of inaSolar.
//
// WHY:  This is the measurement the CC/CV charging algorithm (Block 7) will
//       compare against our 1.0A CC / 14.4V CV targets.
// ---------------------------------------------------------------------------

PowerReading sensors_readBattery() {
  PowerReading reading;                                        // Create an empty struct to fill in below
  reading.busVoltage_V = inaBattery.getBusVoltage_V();         // Already in volts -- no conversion needed
  reading.current_A    = inaBattery.getCurrent_mA() / 1000.0;  // Library gives mA -- divide by 1000 for amps
  reading.power_W       = inaBattery.getBusPower() / 1000.0;   // Library gives mW -- divide by 1000 for watts
  return reading;                                              // Hand all three values back to the caller together
}
