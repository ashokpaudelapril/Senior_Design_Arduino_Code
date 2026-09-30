// ---------------------------------------------------------------------------
// UWBG_Power_Converter.ino -- main sketch file
//
// WHAT: The top-level file Arduino IDE compiles first. Its job is just to
//       call into the modules (Sensors, and later BoostConverter, Display)
//       from setup()/loop() -- it shouldn't contain sensor/control logic
//       itself.
//
// HOW:  #include "Sensors.h" pulls in that module's public functions.
//       Because Sensors.cpp is in the same sketch folder, the Arduino IDE
//       compiles and links it automatically -- no extra setup needed.
//
// WHY:  Keeping this file thin makes the overall program flow readable at a
//       glance: open this one file to see *what* the system does at
//       startup and every loop, then open the relevant module to see *how*.
// ---------------------------------------------------------------------------

#include "Config.h"           // Shared pin/calibration constants
#include "Sensors.h"          // Sensor module (INA226 readings)
#include "BoostConverter.h"   // Boost converter gate-drive module
#include "ChargeController.h" // CC/CV control logic module
#include "TestMode.h"         // Manual duty-cycle override for bench testing
#include "Display.h"          // Display + touch module

void setup() {
  Serial.begin(9600);       // Start Serial at 9600 baud for debug output over USB
  sensors_init();           // Bring up I2C and calibrate the solar + battery + wind INA226 sensors
  boost_init();             // Gate drive starts at 0% duty -- switch off
  chargeController_init();  // Reset the CC/CV controller's duty cycle state to 0%
  display_init();           // Bring up the ST7796U display + FT6336U touch controller

  if (MANUAL_PWM_TEST_MODE) {   // Print usage instructions only when the test mode is actually active
    Serial.println(F("MANUAL PWM TEST MODE: type a duty cycle 0-100 and press Enter."));
  }
}

// ---------------------------------------------------------------------------
// BLOCK 3 (updated in Block 4): printing solar readings in loop()
//
// WHAT: Every SENSOR_PRINT_INTERVAL_MS, take one solar sensor reading and
//       print it to Serial as volts / amps / watts.
//
// HOW:  Calls sensors_readSolar() (declared in Sensors.h) to get a
//       PowerReading struct, then prints its three fields.
//
// WHY:  This is our first real feedback from hardware -- once the board and
//       sensor are wired up, we compare these numbers against a multimeter
//       on the solar array to confirm the shunt calibration from Block 2 is
//       correct before trusting this sensor for anything else (like
//       boost-converter control later).
// ---------------------------------------------------------------------------

void printSolarReading() {
  PowerReading solar = sensors_readSolar();   // Take one snapshot reading from the solar sensor

  Serial.print(F("Solar: "));      // Label, so the Serial Monitor line is readable
  Serial.print(solar.busVoltage_V);
  Serial.print(F(" V, "));         // Unit + separator before the next value
  Serial.print(solar.current_A);
  Serial.print(F(" A, "));
  Serial.print(solar.power_W);
  Serial.println(F(" W"));         // println (not print) ends the line so the next reading starts fresh
}

// ---------------------------------------------------------------------------
// BLOCK 6: printing battery readings in loop()
//
// WHAT: Same idea as printSolarReading(), but for the new battery/
//       charge-output sensor.
//
// WHY:  Lets us verify the battery sensor's readings (against a multimeter,
//       once hardware is wired up) before Block 7's CC/CV logic starts
//       trusting this sensor to control the charger.
// ---------------------------------------------------------------------------

void printBatteryReading() {
  PowerReading battery = sensors_readBattery();   // Take one snapshot reading from the battery sensor

  Serial.print(F("Battery: "));
  Serial.print(battery.busVoltage_V);
  Serial.print(F(" V, "));
  Serial.print(battery.current_A);
  Serial.print(F(" A, "));
  Serial.print(battery.power_W);
  Serial.print(F(" W | Duty: "));
  Serial.print(chargeController_getDutyCycle());   // Report what the control loop is currently commanding
  Serial.print(F(" %"));

  if (chargeController_isFaulted()) {   // Make a latched fault impossible to miss in the Serial Monitor
    Serial.println(F(" | FAULT -- output disabled, reset required"));
  } else {
    Serial.println();
  }
}

// ---------------------------------------------------------------------------
// BLOCK 11: printing wind readings in loop()
//
// WHAT: Same idea as printSolarReading()/printBatteryReading(), but for the
//       new wind branch sensor.
//
// WHY:  Lets us verify the wind sensor independently once the wind drive
//       motor and generators are wired up, matching how we brought the
//       solar and battery sensors online one at a time.
// ---------------------------------------------------------------------------

void printWindReading() {
  PowerReading wind = sensors_readWind();   // Take one snapshot reading from the wind branch sensor

  Serial.print(F("Wind: "));
  Serial.print(wind.busVoltage_V);
  Serial.print(F(" V, "));
  Serial.print(wind.current_A);
  Serial.print(F(" A, "));
  Serial.print(wind.power_W);
  Serial.println(F(" W"));
}

// ---------------------------------------------------------------------------
// BLOCK 4: non-blocking timing in loop()
//
// WHAT: Replaces delay(1000) with a millis()-based check, so printing still
//       happens about once a second but WITHOUT freezing the rest of
//       loop() while it waits.
//
// HOW:  millis() returns the number of milliseconds since the board booted.
//       We remember the last time we printed (lastPrintTime) and compare
//       how much time has passed. When enough has passed, we print and
//       update lastPrintTime. Using subtraction (now - lastPrintTime)
//       instead of comparing raw millis() values directly also handles the
//       millis() counter overflowing and wrapping back to 0 (which happens
//       after about 49 days) correctly, because unsigned integer
//       subtraction wraps the same way.
//
// WHY:  delay() blocks the entire program -- nothing else can run while
//       it's waiting. The boost-converter CC/CV control loop (Block 7,
//       below) needs to check sensor readings and adjust PWM duty cycle
//       many times per second to regulate charging current/voltage
//       properly. If we kept delay(1000), that control loop would only get
//       to run once a second, which is far too slow and would make the
//       charger unstable. Switching to millis() means loop() runs
//       continuously, and BOTH the sensor printing AND the control logic
//       can each run at their own appropriate pace within it.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// BLOCK 7 (branched in Block 9): running the CC/CV control loop in loop()
//
// WHAT: Calls chargeController_update() on its own timer, separate from
//       (and faster than) the once-a-second Serial printing -- but ONLY
//       when MANUAL_PWM_TEST_MODE is false. When it's true, testMode_update()
//       runs instead, so typed Serial input controls duty cycle directly.
//
// HOW:  Same millis()-based pattern as the print timer, just with its own
//       lastControlTime variable and its own interval
//       (CONTROL_LOOP_INTERVAL_MS = 50ms -- 20 times a second).
//
// WHY:  The control loop needs to run much more often than we print debug
//       output -- printing 20 times a second would flood the Serial
//       Monitor and be unreadable, but the charger still needs to react
//       that quickly to stay stable. Two independent timers let each task
//       run at the pace it actually needs. Branching on MANUAL_PWM_TEST_MODE
//       here means the two modes can never both try to command duty cycle
//       at the same time.
// ---------------------------------------------------------------------------

void loop() {
  static unsigned long lastPrintTime = 0;     // Remembers the millis() value at the last print, kept between loop() calls
  static unsigned long lastControlTime = 0;   // Same idea, for the control loop's own timer
  unsigned long now = millis();               // Current time (ms since board booted), read fresh each pass

  if (MANUAL_PWM_TEST_MODE) {
    testMode_update();   // Bench-test mode: duty cycle comes from typed Serial input, checked every loop() pass
  } else if (now - lastControlTime >= CONTROL_LOOP_INTERVAL_MS) {   // Has a full control interval elapsed?
    lastControlTime = now;          // Reset this timer's reference point
    chargeController_update();      // Read battery sensor, adjust duty cycle one step
  }

  if (now - lastPrintTime >= SENSOR_PRINT_INTERVAL_MS) {   // Has a full print interval elapsed?
    lastPrintTime = now;        // Reset the timer reference point
    printSolarReading();        // Take and print a new solar reading
    printWindReading();         // Take and print a new wind branch reading
    printBatteryReading();      // Take and print a new battery reading + current duty cycle
  }
}
