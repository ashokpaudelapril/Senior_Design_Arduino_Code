#ifndef TEMPERATURESENSORS_H
#define TEMPERATURESENSORS_H

// ---------------------------------------------------------------------------
// TEMPERATURE SENSORS MODULE (public interface)
//
// WHAT: Reads the two DS18B20 probes monitoring the GaN switch and the
//       boost inductor -- the two hottest components in the power stage.
//
// HOW:  temperatureSensors_init() brings up the 1-Wire bus. Call it once
//       from setup(). temperatureSensors_update() must be called every
//       loop() pass -- it advances an internal non-blocking state machine
//       (see WHY) and does nothing most of the time it's called.
//       temperatureSensors_getLatest() returns whatever the most recent
//       completed reading was -- it's instant and safe to call as often as
//       needed (every loop() pass, from multiple places), since it never
//       waits on anything.
//
// WHY:  Separate module, not folded into Sensors.h/.cpp, because DS18B20
//       uses a completely different protocol (1-Wire, not I2C), has a
//       fundamentally different timing requirement (a DS18B20 conversion
//       takes up to 750ms -- INA226 I2C reads are near-instant by
//       comparison), and returns a different shape of data (one
//       temperature, not voltage/current/power). Folding a 750ms-latency
//       sensor into the same pattern as the near-instant ones would be
//       misleading.
//
//       The update()/getLatest() split exists specifically because of that
//       750ms conversion time: calling the straightforward blocking
//       DallasTemperature API (request a reading, then immediately read it
//       back) would freeze the ENTIRE firmware for 750ms every time --
//       breaking manual-mode responsiveness, the control loop, touch
//       polling, everything. update() instead kicks off a conversion, then
//       (on a LATER call, once enough time has passed) collects the
//       result -- all without ever blocking loop().
// ---------------------------------------------------------------------------

struct TemperatureReading {
  float ganSwitchTemp_C;    // GaN switch probe reading, degrees Celsius
  float inductorTemp_C;     // Boost inductor probe reading, degrees Celsius
  bool  valid;              // False if either probe didn't respond -- don't trust the values if so
};

void temperatureSensors_init();
void temperatureSensors_update();
TemperatureReading temperatureSensors_getLatest();

#endif
