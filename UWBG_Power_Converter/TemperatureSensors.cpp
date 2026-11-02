// ---------------------------------------------------------------------------
// TEMPERATURE SENSORS MODULE (implementation)
//
// WHAT: Reads the GaN switch and inductor DS18B20 probes over 1-Wire,
//       without ever blocking loop() while waiting on a conversion.
//
// HOW:  Both probes share one 1-Wire bus (TEMP_SENSOR_PIN). Each DS18B20
//       has its own unique, factory-burned 64-bit ID, and the
//       DallasTemperature library finds all devices on the bus during
//       begin() and lets you read them back by index (0, 1, ...) in the
//       order its search found them -- index[0] is wired as the GaN switch
//       probe, index[1] is the inductor probe (see VERIFY note below).
//
//       probes.setWaitForConversion(false) is the key call: it tells the
//       library NOT to block when requestTemperatures() is called. Instead
//       the DS18B20 chips do their conversion in the background, and
//       temperatureSensors_update() implements a small state machine:
//         1. Every TEMP_REQUEST_INTERVAL_MS, if not already mid-conversion,
//            call requestTemperatures() and start timing.
//         2. Once TEMP_CONVERSION_TIME_MS has passed since that request,
//            read the results and cache them.
//       temperatureSensors_getLatest() just returns that cache -- it never
//       waits on anything, so it's safe to call from anywhere, as often as
//       needed.
//
// WHY:  See TemperatureSensors.h -- the straightforward blocking API would
//       freeze the whole firmware for up to 750ms per reading. This
//       two-step request/collect pattern is the same non-blocking
//       philosophy used everywhere else in this project (millis()-based
//       timers instead of delay()).
//
// VERIFY ON HARDWARE: after wiring both probes, confirm index 0 really is
//       the GaN probe and index 1 really is the inductor probe -- warm one
//       probe with a finger or hair dryer and confirm the Serial output
//       shows the change on the expected reading, not the other one. Don't
//       just trust the wiring order. The search order is determined by
//       comparing each device's unique ID bit-by-bit, not by which one was
//       plugged in "first," so it will be consistent run-to-run once
//       confirmed -- but it still needs confirming once.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "TemperatureSensors.h"
#include "Config.h"

static OneWire oneWireBus(TEMP_SENSOR_PIN);       // The 1-Wire bus itself, on one pin
static DallasTemperature probes(&oneWireBus);     // The library that talks DS18B20's protocol over that bus

static TemperatureReading latestReading = {0.0, 0.0, false};  // Cached result; starts "invalid" until the first read completes
static unsigned long lastRequestTime = 0;                      // millis() timestamp of the last requestTemperatures() call
static bool conversionPending = false;                         // True while waiting out TEMP_CONVERSION_TIME_MS

void temperatureSensors_init() {
  probes.begin();                        // Scans the bus and finds every DS18B20 present -- this is what assigns index 0, 1, ...
  probes.setWaitForConversion(false);    // Critical: without this, requestTemperatures() blocks for ~750ms by default
}

void temperatureSensors_update() {
  unsigned long now = millis();

  if (!conversionPending && (now - lastRequestTime >= TEMP_REQUEST_INTERVAL_MS)) {
    // Time to start a new reading. This call returns immediately (non-blocking mode)
    // while the DS18B20s convert in the background.
    probes.requestTemperatures();
    lastRequestTime = now;
    conversionPending = true;
    return;   // Nothing to collect yet -- the conversion just started
  }

  if (conversionPending && (now - lastRequestTime >= TEMP_CONVERSION_TIME_MS)) {
    // Enough time has passed that both probes should be done converting -- safe to read now.
    latestReading.ganSwitchTemp_C = probes.getTempCByIndex(0);   // Index 0 -- GaN switch probe
    latestReading.inductorTemp_C  = probes.getTempCByIndex(1);   // Index 1 -- inductor probe

    // DEVICE_DISCONNECTED_C (-127.0) is the library's way of saying "this index didn't
    // respond" -- treat that the same way Sensors.cpp treats a missing I2C sensor: don't
    // trust the number, flag it instead.
    latestReading.valid = (latestReading.ganSwitchTemp_C != DEVICE_DISCONNECTED_C) &&
                           (latestReading.inductorTemp_C  != DEVICE_DISCONNECTED_C);

    conversionPending = false;   // Ready to start the next cycle once TEMP_REQUEST_INTERVAL_MS passes again
  }
}

TemperatureReading temperatureSensors_getLatest() {
  return latestReading;   // Always instant -- just returns whatever was cached by the last completed update()
}
