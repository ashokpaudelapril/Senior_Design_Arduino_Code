// ---------------------------------------------------------------------------
// DISPLAY MODULE (implementation)
//
// WHAT: Initializes the ST7796U display (via Adafruit_ST7796S_kbv, which
//       extends Adafruit_GFX -- the same drawPixel/setCursor/print-style API
//       as most Adafruit display libraries) and the FT6336U touch
//       controller (via the Arduino-FT6336U library), then draws one test
//       message to confirm both are alive.
//
// HOW:  The display constructor takes (CS, DC, RST) for hardware-SPI mode
//       -- MOSI/MISO/SCK are NOT passed in because on the Mega, hardware
//       SPI is fixed to pins 50/51/52 and the library uses those
//       automatically. The touch constructor takes (interruptPin,
//       resetPin) -- its actual I2C communication goes over the same
//       Wire bus (pins 20/21) that Sensors.cpp already initializes with
//       Wire.begin(), so this module does NOT call Wire.begin() again.
//
// WHY:  VERIFY BEFORE RELYING ON THIS: the exact library method/constant
//       names below (tft.begin(), color constants, touch.begin() and its
//       read methods) are written from how these libraries are
//       conventionally structured, not a confirmed check against the
//       installed library's own header/examples. Once the libraries are
//       actually installed, open their bundled example sketches first --
//       if a name here doesn't match (e.g. a different color constant
//       naming scheme), fix it here to match the real library, the same
//       way we'd double check an INA226 I2C address against real hardware.
// ---------------------------------------------------------------------------

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>
#include <FT6336U.h>
#include "Display.h"
#include "Config.h"

Adafruit_ST7796S_kbv tft(LCD_CS_PIN, LCD_RS_PIN, LCD_RST_PIN);   // Hardware-SPI display object
FT6336U touch(TOUCH_INT_PIN, TOUCH_RST_PIN);                     // I2C touch controller object

void display_init() {
  pinMode(LCD_LED_PIN, OUTPUT);     // Backlight pin is just a plain digital output
  digitalWrite(LCD_LED_PIN, HIGH);  // Backlight full on -- no dimming yet, that's a future refinement

  tft.begin();          // Bring up the ST7796U over SPI (resets it, sends its startup command sequence)
  tft.setRotation(1);   // Landscape orientation -- adjust (0-3) once we see how the panel is physically mounted
  tft.fillScreen(BLACK);   // Clear the screen to a known state before drawing anything

  tft.setTextColor(WHITE);   // White text on the black background we just filled
  tft.setTextSize(2);        // Large enough to read from a short distance
  tft.setCursor(10, 10);     // Start near the top-left corner
  tft.print(F("UWBG Power Converter"));   // One-time test message -- confirms the display itself works

  touch.begin();   // Bring up the FT6336U touch controller over the shared I2C bus

  Serial.println(F("Display and touch controller initialized."));   // Confirm setup completed, for debugging over Serial
}
