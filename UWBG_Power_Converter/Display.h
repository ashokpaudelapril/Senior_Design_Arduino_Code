#ifndef DISPLAY_H
#define DISPLAY_H

// ---------------------------------------------------------------------------
// DISPLAY MODULE (public interface)
//
// WHAT: Brings up the Hosyond 3.5" ST7796U SPI display and its FT6336U
//       capacitive touch controller. This first version only handles
//       initialization and a one-time test message -- actual live data
//       rendering (solar/wind/battery readings, duty cycle, fault status)
//       and touch handling come in a later block, same as how Sensors.cpp
//       got an init step (Block 2) before real readings (Block 3).
//
// HOW:  display_init() sets up the display and touch chip; call it once
//       from setup(), after sensors_init()/boost_init() so Serial is
//       already available for debug output.
//
// WHY:  Same reasoning as every other module here: other code shouldn't
//       need to know this is an Adafruit_ST7796S_kbv object talking SPI, or
//       an FT6336U object talking I2C -- just that "the display module" has
//       a function to bring it online.
// ---------------------------------------------------------------------------

void display_init();

#endif
