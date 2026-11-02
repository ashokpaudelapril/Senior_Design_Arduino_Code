// =====================================================================
// I2C_Scanner_Test
//
// WHAT: Checks every possible I2C address (1-127) and reports back
//       which ones actually have a device responding -- answers "what
//       address is this INA226 module really set to?" for certain,
//       instead of guessing from the A0/A1 jumper pads.
//
// WHY:  CurrentControl_Uno_Test.ino assumes the INA226 is at 0x40, but
//       that's only the DEFAULT -- if this specific module's jumpers
//       are set differently, that assumption is wrong, and a wrong
//       I2C address gives silent garbage readings rather than an
//       obvious error (explained earlier). This sketch removes the
//       guesswork entirely.
//
// HOW:  For each possible address, the Arduino sends a tiny "are you
//       there?" message (Wire.beginTransmission + Wire.endTransmission)
//       and checks whether anything acknowledges it. No actual data is
//       read -- this only checks presence, not what the device is.
//
// USE: Upload this to the UNO with ONLY the sensor's I2C wires
//      connected (SDA/SCL/VCC/GND) -- the PWM/motor side doesn't need
//      to be connected for this test at all. Open the Serial Monitor
//      at 9600 baud.
// =====================================================================

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial) {
    // wait for the Serial Monitor, same pattern as every other test sketch here
  }

  Serial.println(F("--- I2C_Scanner_Test start ---"));
  Serial.println(F("Scanning all possible addresses (1-127)..."));
}

void loop() {
  int devicesFound = 0;

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t result = Wire.endTransmission();   // 0 means something acknowledged -- a real device is there

    if (result == 0) {
      Serial.print(F("Device found at address 0x"));
      if (address < 16) {
        Serial.print(F("0"));   // Pads single-digit hex (e.g. 0x04) so addresses line up neatly when printed
      }
      Serial.println(address, HEX);
      devicesFound++;
    }
  }

  if (devicesFound == 0) {
    Serial.println(F("No devices found -- check wiring (SDA/SCL swapped is a common mistake) before assuming the sensor is broken."));
  } else {
    Serial.print(F("Scan complete. "));
    Serial.print(devicesFound);
    Serial.println(F(" device(s) found. Compare the address(es) above against what CurrentControl_Uno_Test.ino assumes (0x40)."));
  }

  Serial.println(F("Scanning again in 5 seconds..."));
  delay(5000);   // A plain delay is fine here -- this is a one-purpose diagnostic sketch, not something that needs to stay responsive to anything else
}
