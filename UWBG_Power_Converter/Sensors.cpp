// ---------------------------------------------------------------------------
// SENSORS MODULE (implementation)
//
// WHAT: Sets up I2C and the five INA226 sensor objects (solar, battery,
//       wind, common bus, load), and exposes sensors_init() to bring them
//       online.
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

INA226_WE inaSolar     = INA226_WE(INA226_SOLAR_ADDR);      // Sensor object bound to the solar sensor's I2C address
INA226_WE inaBattery   = INA226_WE(INA226_BATTERY_ADDR);    // Sensor object bound to the battery/charge-output sensor's I2C address
INA226_WE inaWind      = INA226_WE(INA226_WIND_ADDR);       // Sensor object bound to the wind branch sensor's I2C address
INA226_WE inaCommonBus = INA226_WE(INA226_COMMONBUS_ADDR);  // Sensor object bound to the common DC bus sensor's I2C address
INA226_WE inaLoad      = INA226_WE(INA226_LOAD_ADDR);       // Sensor object bound to the load sensor's I2C address (PLACEHOLDER -- see Config.h)

void sensors_init() {
  Wire.begin();   // Join I2C bus as controller (SDA=pin20, SCL=pin21 on both Mega and Giga)

  inaSolar.init();                                              // Reset/configure the solar INA226 chip over I2C
  inaSolar.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Tell the library our shunt value + expected max current

  inaBattery.init();                                              // Reset/configure the battery INA226 chip over I2C
  inaBattery.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Same shunt/current-range values -- identical board

  inaWind.init();                                              // Reset/configure the wind branch INA226 chip over I2C
  inaWind.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Same shunt/current-range values -- identical board

  inaCommonBus.init();                                              // Reset/configure the common bus INA226 chip over I2C
  inaCommonBus.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Same shunt/current-range values -- identical board

  inaLoad.init();                                              // Harmless if not physically wired yet -- I2C just won't get ACKed
  inaLoad.setResistorRange(INA226_SHUNT_OHMS, INA226_MAX_AMPS); // Same placeholder shunt/current-range values as the others

  Serial.println(F("Solar, battery, wind, common bus, and load INA226 sensors initialized."));   // Confirm setup completed, for debugging over Serial
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
// WHY:  This is the measurement the CC/CV charging algorithm compares against
//       our 1.0A CC / 14.4V CV targets.
// ---------------------------------------------------------------------------

PowerReading sensors_readBattery() {
  PowerReading reading;                                        // Create an empty struct to fill in below
  reading.busVoltage_V = inaBattery.getBusVoltage_V();         // Already in volts -- no conversion needed
  reading.current_A    = inaBattery.getCurrent_mA() / 1000.0;  // Library gives mA -- divide by 1000 for amps
  reading.power_W       = inaBattery.getBusPower() / 1000.0;   // Library gives mW -- divide by 1000 for watts
  return reading;                                              // Hand all three values back to the caller together
}

// ---------------------------------------------------------------------------
// BLOCK 11: sensors_readWind()
//
// WHAT: Takes one reading from the wind branch INA226 and returns it as a
//       PowerReading, the same shape as sensors_readSolar()/readBattery().
//
// HOW:  Identical logic to the other two read functions, just against the
//       inaWind object/address.
//
// WHY:  Lets us monitor and log the wind generation path independently of
//       solar, matching the block diagram's separate solar/wind branches
//       feeding into one common DC bus.
// ---------------------------------------------------------------------------

PowerReading sensors_readWind() {
  PowerReading reading;                                     // Create an empty struct to fill in below
  reading.busVoltage_V = inaWind.getBusVoltage_V();         // Already in volts -- no conversion needed
  reading.current_A    = inaWind.getCurrent_mA() / 1000.0;  // Library gives mA -- divide by 1000 for amps
  reading.power_W       = inaWind.getBusPower() / 1000.0;   // Library gives mW -- divide by 1000 for watts
  return reading;                                            // Hand all three values back to the caller together
}

// ---------------------------------------------------------------------------
// BLOCK 15: sensors_readCommonBus()
//
// WHAT: Takes one reading from the common DC bus INA226 and returns it as a
//       PowerReading, the same shape as the other three read functions.
//
// HOW:  Identical logic to sensors_readSolar()/readWind()/readBattery(),
//       just against the inaCommonBus object/address.
//
// WHY:  This is the true boost-converter input measurement -- placed after
//       both isolation diodes combine, so it captures their forward-voltage
//       drop. Efficiency.cpp uses it as the input power.
// ---------------------------------------------------------------------------

PowerReading sensors_readCommonBus() {
  PowerReading reading;                                          // Create an empty struct to fill in below
  reading.busVoltage_V = inaCommonBus.getBusVoltage_V();         // Already in volts -- no conversion needed
  reading.current_A    = inaCommonBus.getCurrent_mA() / 1000.0;  // Library gives mA -- divide by 1000 for amps
  reading.power_W       = inaCommonBus.getBusPower() / 1000.0;   // Library gives mW -- divide by 1000 for watts
  return reading;                                                 // Hand all three values back to the caller together
}

// ---------------------------------------------------------------------------
// BLOCK 17: sensors_readLoad() -- PLACEHOLDER, see Config.h
//
// WHAT: Takes one reading from the load INA226 and returns it as a
//       PowerReading, the same shape as the other four read functions.
//
// HOW:  Identical logic to the others, just against the inaLoad
//       object/address.
//
// WHY THIS IS A PLACEHOLDER: with no buck converter built yet, this sensor
//       isn't physically attached to anything meaningful. Calling this
//       function right now will behave like reading any other disconnected
//       INA226 -- likely near-zero or stale values, NOT an error, and NOT
//       trustworthy data. This exists so wiring it up later is a five-
//       minute repeat of this exact pattern, not a new design exercise.
// ---------------------------------------------------------------------------

PowerReading sensors_readLoad() {
  PowerReading reading;                                     // Create an empty struct to fill in below
  reading.busVoltage_V = inaLoad.getBusVoltage_V();         // Already in volts -- no conversion needed
  reading.current_A    = inaLoad.getCurrent_mA() / 1000.0;  // Library gives mA -- divide by 1000 for amps
  reading.power_W       = inaLoad.getBusPower() / 1000.0;   // Library gives mW -- divide by 1000 for watts
  return reading;                                            // Hand all three values back to the caller together
}

// ---------------------------------------------------------------------------
// BLOCK 10: sensors_isBatterySensorConnected()
//
// WHAT: Probes the battery sensor's I2C address directly, without asking it
//       for any data, just to see if a device answers at all.
//
// HOW:  Wire.beginTransmission()/endTransmission() is the raw I2C
//       "is anyone listening at this address?" check -- it sends just the
//       address byte and checks for an ACK. endTransmission() returns 0
//       only if a device acknowledged; any other value means nothing
//       responded (not wired, wrong address, powered off, etc.).
//
// WHY:  This is deliberately separate from sensors_readBattery(), which
//       always returns SOME numbers whether or not a real sensor is there
//       -- the INA226_WE library doesn't distinguish "sensor missing" from
//       "sensor present and reading near zero." This function is the only
//       way calling code (ChargeController) can tell those two cases apart.
// ---------------------------------------------------------------------------

bool sensors_isBatterySensorConnected() {
  Wire.beginTransmission(INA226_BATTERY_ADDR);   // Start addressing the battery sensor on the I2C bus
  byte error = Wire.endTransmission();           // Actually send it; 0 = a device ACKed, anything else = no response
  return (error == 0);                            // true only if the sensor is actually present and responding
}
