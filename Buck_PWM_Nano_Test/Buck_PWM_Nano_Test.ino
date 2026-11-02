// =====================================================================
// Buck_PWM_Nano_Test
//
// WHAT: A standalone, throwaway sketch for a stock Arduino Nano
//       (ATmega328P). Produces a PWM signal on pin D9 and lets you set
//       the duty cycle by typing a number (0-100) in the Serial
//       Monitor -- same idea as PWM_Giga_Test.ino and
//       Buck_PWM_Giga_Test.ino, just on completely different hardware.
//
// WHY THIS BOARD, RIGHT NOW: the project's Giga R1 WiFi got damaged and
//       a replacement is on order. This Nano is a stopgap -- it lets the
//       buck converter's basic PWM mechanism (gate-drive signal, duty
//       cycle control, the overall approach) get checked on the bench
//       while waiting for the Giga replacement, rather than everything
//       stopping until new hardware arrives.
//
// HONEST LIMITATION, STATED UP FRONT: this is NOT the same resolution as
//       the Giga-based firmware. The Nano's ATmega328P runs at 16MHz;
//       for a 100kHz switching frequency, that gives only 160 timer
//       ticks per period (16,000,000 / 100,000 = 160), so the finest
//       possible duty-cycle step is 1/160 = 0.625% -- about 15x coarser
//       than the Giga's ~0.042% (2400 steps). This is the exact same
//       math/limitation the original Mega 2560 would have hit before
//       the project moved to the Giga -- same AVR family, same timer
//       architecture. This sketch is for checking that the buck
//       converter mechanism works at all (signal present, duty cycle
//       responds to input, gate driver switches correctly) -- not a
//       substitute for the real resolution check, which still needs to
//       happen on the actual Giga once it's back.
//
// HOW:  Arduino's normal analogWrite() was not tried here at all --
//       it's already an established fact from this project's own
//       history that analogWrite() tops out around 490-980 Hz on AVR
//       boards, nowhere close to the ~100kHz target. Going straight to
//       direct AVR Timer1 register configuration, same reasoning as
//       going straight to STM32 HAL for the Giga instead of trying
//       Mbed's PwmOut again.
//
//       Pin D9 is ATmega328P's OC1A (Timer1, Compare Output A) --
//       Timer1 is the 16-bit timer, which matters here: an 8-bit timer
//       (Timer0, Timer2) can only count to 255, nowhere near enough for
//       a custom 160-tick period: ICR1 (Timer1's 16-bit "Input Capture"
//       register) is used as the counter's TOP value in Fast PWM Mode
//       14, giving a period of exactly whatever we set it to, not a
//       fixed 255 or 1023.
//
// SAFETY: No safety limits in this sketch. Keep the gate driver and GaN
//         switch disconnected while probing, same as the other PWM test
//         sketches.
// =====================================================================

#include <Arduino.h>

// ---------------------------------------------------------------------
// BLOCK 1: timer math
//
// WHAT: How many Timer1 ticks make up one 100kHz period at the Nano's
//       16MHz clock, with no prescaling.
// HOW:  16,000,000 Hz / 100,000 Hz = 160 ticks per period. Timer1 counts
//       0..TOP then wraps, so TOP = 160 - 1 = 159.
// WHY:  Unlike the Giga's TIM8 clock (which had to be MEASURED because
//       STM32 timers can get an unexpected 2x multiplier over their bus
//       clock), the Nano's 16MHz is the crystal frequency directly --
//       AVR timers don't have that multiplier quirk, so this number is
//       trustworthy from the datasheet, not something that needs a
//       breadcrumb measurement first.
// ---------------------------------------------------------------------
#define NANO_CLOCK_HZ       16000000UL
#define PWM_FREQUENCY_HZ    100000UL
#define PWM_PERIOD_TICKS    (NANO_CLOCK_HZ / PWM_FREQUENCY_HZ)   // 160 ticks per period
#define PWM_TOP_VALUE       (PWM_PERIOD_TICKS - 1)               // 159 -- Timer1 counts 0..TOP

// ---------------------------------------------------------------------
// BLOCK 2: configureTimer1Pwm() -- direct AVR register setup
//
// WHAT: Configures pin D9 (OC1A) for Fast PWM using Timer1, with ICR1
//       as the TOP value so the period is exactly 160 ticks (100kHz),
//       not a fixed power-of-two default.
// HOW:  This is "Fast PWM, Mode 14" in the ATmega328P datasheet's timer
//       mode table -- selected by setting WGM13:WGM10 = 1110 across the
//       TCCR1A and TCCR1B registers. COM1A1=1, COM1A0=0 sets
//       non-inverting mode on OC1A (pin high at the start of each
//       period, goes low when the counter reaches OCR1A -- so a bigger
//       OCR1A value means the pin stays high longer, same "bigger
//       compare value = more duty cycle" relationship as the Giga code).
//       CS10=1 alone (no other CS1x bits) selects no prescaling, using
//       the full 16MHz clock directly.
// WHY:  Direct register access is used instead of any Arduino timer
//       library, same reasoning as the Giga's direct HAL approach --
//       full control over the exact period and resolution, instead of
//       being limited to whatever analogWrite()'s fixed assumptions
//       allow.
// ---------------------------------------------------------------------
static void configureTimer1Pwm() {
  pinMode(9, OUTPUT);   // D9 = OC1A -- must be an output for the timer's PWM signal to actually reach the pin

  // --- Stop the timer and clear its control registers before configuring, so no leftover bits from Arduino's own init code interfere ---
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;            // Reset the counter to 0

  ICR1 = PWM_TOP_VALUE;  // 159 -- this is what makes the period exactly 160 ticks instead of a fixed 255/1023

  // --- Fast PWM, Mode 14 (TOP = ICR1), non-inverting output on OC1A (D9) ---
  TCCR1A |= (1 << COM1A1);                 // Non-inverting mode: clear OC1A on compare match, set at BOTTOM
  TCCR1A |= (1 << WGM11);                  // WGM11 (part of Mode 14, see TCCR1B below for the rest)
  TCCR1B |= (1 << WGM13) | (1 << WGM12);   // WGM13+WGM12 complete Mode 14 (Fast PWM, TOP=ICR1)
  TCCR1B |= (1 << CS10);                   // No prescaling -- run Timer1 at the full 16MHz

  OCR1A = 0;   // Start at 0% duty -- safe default, same as every other PWM sketch in this project
}

static void setDuty(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;    // Clamp, same safety habit as the other PWM test sketches
  if (dutyPercent > 100.0) dutyPercent = 100.0;

  uint16_t compareValue = (uint16_t)(dutyPercent / 100.0 * PWM_PERIOD_TICKS);
  OCR1A = compareValue;   // Applying this directly to the hardware register takes effect on the next timer cycle
}

// ---------------------------------------------------------------------
// BLOCK 3: heartbeat -- same pattern as every other test sketch in this
// project, so a silent Serial Monitor is a clear "something's actually
// wrong" signal rather than just "nobody typed anything recently."
// ---------------------------------------------------------------------
static unsigned long lastHeartbeat = 0;
static float currentDuty = 0.0;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // wait for the Serial Monitor to actually be listening, same reasoning as every other test sketch here
  }

  configureTimer1Pwm();

  // WHAT: D8 is a plain always-on reference pin, not PWM at all -- just
  //       a steady digital HIGH, set once and never changed.
  // WHY:  Same reasoning as pin 5 on PWM_Giga_Test.ino -- a second scope
  //       channel that never moves makes it much easier to confirm D9's
  //       duty cycle is actually changing correctly, by comparing "this
  //       one never moves" against "this one should move exactly as
  //       much as I typed."
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  Serial.println(F("--- Buck_PWM_Nano_Test start ---"));
  Serial.println(F("Pin D8 is a fixed always-on reference (steady HIGH, not PWM) -- compare it on a second scope channel against D9."));
  Serial.println(F("Stopgap buck converter PWM check on Arduino Nano, while waiting for the Giga replacement."));
  Serial.print(F("Period: "));
  Serial.print(PWM_PERIOD_TICKS);
  Serial.println(F(" ticks (160) -- resolution is 1/160 = 0.625% per step, NOT the Giga's ~0.042%. This is a mechanism check, not the final resolution check."));
  Serial.println(F("Type a duty cycle 0-100 and press Enter. Pin D9 is the gate-drive output."));
}

void loop() {
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    Serial.print(F("heartbeat | duty: "));
    Serial.print(currentDuty);
    Serial.println(F(" %"));
  }

  if (Serial.available() > 0) {
    float requested = Serial.parseFloat();
    while (Serial.available() > 0) {
      Serial.read();   // discard any trailing characters, same pattern as the other PWM test sketches
    }

    if (requested < 0.0f)   requested = 0.0f;
    if (requested > 100.0f) requested = 100.0f;
    currentDuty = requested;

    setDuty(requested);

    Serial.print(F("Duty set to: "));
    Serial.print(requested);
    Serial.print(F(" % (OCR1A="));
    Serial.print(OCR1A);
    Serial.print(F("/"));
    Serial.print(PWM_TOP_VALUE);
    Serial.println(F(")"));
  }
}
