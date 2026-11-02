// ---------------------------------------------------------------------------
// PWM_Giga_Test.ino -- bench test for the boost gate-drive PWM on the Giga
//
// WHAT: Produces a 100 kHz PWM on pin 11 (D11) and lets you set the duty
//       cycle by typing a number (0-100) in the Serial Monitor. Nothing
//       else is used: no sensors, no display, no charge controller, no
//       fault logic -- just the PWM signal itself, for checking on a scope
//       before connecting the gate driver and GaN switch.
//
// HOW:  Pin 11 (D11) maps to STM32's TIM8 Channel 2 on the Giga R1 WiFi.
//       This configures that timer directly through STM32 HAL: 240MHz
//       kernel clock (measured on real hardware, not assumed), no
//       prescaling, 2400 ticks per period -- 240,000,000 / 2400 = exactly
//       100 kHz, with ~0.042% duty resolution per step.
//
// WHY THIS APPROACH: Arduino's normal PWM API (Mbed's PwmOut, both write()
//       and pulsewidth()) was tried first, since that's the standard way
//       to do PWM on this board. Scope measurement showed it only produces
//       ~10% duty steps regardless of which entry point was used -- values
//       like 25-29% all rounded down to 20%. That's a limitation inside
//       Mbed's own PWM implementation for this timer, not something
//       fixable from the sketch side with the standard API. Driving TIM8
//       directly bypasses that entirely and was confirmed correct on the
//       scope at 35%, 55%, and 58% -- the exact values that exposed the
//       original problem.
//
//       This is the same code now used in the real firmware's
//       BoostConverter.cpp -- this sketch exists purely so the PWM signal
//       can be checked on the bench in isolation, without the rest of the
//       project (sensors, display, control loop) also needing to compile
//       and run.
//
// SAFETY: No safety limits in this sketch. Keep the gate driver and GaN
//         switch disconnected while probing.
// ---------------------------------------------------------------------------

#include <Arduino.h>

extern "C" {
#include "stm32h7xx_hal.h"
}

static const uint32_t TIM8_CLOCK_HZ = 240000000UL;     // Measured on real hardware -- see Config.h in the main firmware
static const uint32_t PWM_PERIOD_TICKS = 2400;         // 240MHz / 2400 = exactly 100kHz
static const uint32_t PWM_ARR = PWM_PERIOD_TICKS - 1;  // Timer counts 0..ARR, so ARR = period - 1

static TIM_HandleTypeDef htim8;

// ---------------------------------------------------------------------------
// SECOND PWM CHANNEL: pin 5 (D5), via TIM3 Channel 2 -- added for a second,
// independent PWM output to test alongside pin 11.
//
// WHAT TWO THINGS ARE UNVERIFIED HERE, SPELLED OUT HONESTLY:
//   1. D5 = PA7 -- this comes from a third-party pin reference, not
//      Arduino's own official pinout PDF (same caveat as the buck
//      converter's D1/PA9 guess). GPIO_AF2_TIM3 is a much more confident
//      standard STM32 convention (TIM3 is AF2 on every F4/F7/H7 part),
//      so the alternate-function number is the less risky part of this.
//   2. TIM3's actual clock speed -- TIM3 sits on APB1, a DIFFERENT bus
//      than TIM8 (APB2), so the 240MHz measured for TIM8 does NOT
//      automatically apply here. TIM3_CLOCK_HZ_ASSUMED below is a
//      placeholder (120MHz, the common STM32H7 APB1/APB2 ratio), not a
//      measurement -- setup() prints the REAL measured APB1 clock so
//      this assumption can be checked against it, same verification
//      discipline that caught TIM8's real clock before trusting it.
// ---------------------------------------------------------------------------
#define TIM3_CLOCK_HZ_ASSUMED   120000000UL                                 // UNVERIFIED -- compare against the printed measurement in setup()
#define PWM3_PERIOD_TICKS       (TIM3_CLOCK_HZ_ASSUMED / 100000UL)          // target 100kHz, same as the TIM8 channel
#define PWM3_ARR_VALUE          (PWM3_PERIOD_TICKS - 1)

static TIM_HandleTypeDef htim3;

static void configureTim3Pwm() {
  Serial.println(F("Step P1: enabling GPIOA and TIM3 clocks..."));
  Serial.flush();
  __HAL_RCC_GPIOA_CLK_ENABLE();   // GUESS: D5 is physically PA7 -- see header comment above
  __HAL_RCC_TIM3_CLK_ENABLE();

  Serial.println(F("Step P2: configuring PA7 as TIM3_CH2 alternate function..."));
  Serial.flush();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_7;                     // GUESS -- see header comment above
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF2_TIM3;            // Standard STM32 AF number for TIM3 -- the more confident part of this guess
  HAL_GPIO_Init(GPIOA, &gpio);

  Serial.println(F("Step P3: configuring TIM3 base timing..."));
  Serial.flush();
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = PWM3_ARR_VALUE;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  // NOTE: no RepetitionCounter field here -- that's specific to the
  // "advanced control" timers (TIM1/TIM8). TIM3 is a general-purpose
  // timer and doesn't have one.
  HAL_TIM_PWM_Init(&htim3);

  Serial.println(F("Step P4: configuring Channel 2 for PWM..."));
  Serial.flush();
  TIM_OC_InitTypeDef channelConfig = {0};
  channelConfig.OCMode = TIM_OCMODE_PWM1;
  channelConfig.Pulse = 0;                   // start at 0% -- safe default
  channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim3, &channelConfig, TIM_CHANNEL_2);

  Serial.println(F("Step P5: starting PWM..."));
  Serial.flush();
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  // NOTE: no __HAL_TIM_MOE_ENABLE() call here -- that master-output-enable
  // gotcha is specific to TIM1/TIM8 (advanced control timers). General-
  // purpose timers like TIM3 start outputting as soon as HAL_TIM_PWM_Start()
  // runs, with no separate enable step.

  Serial.println(F("Step P6: TIM3 PWM configuration complete."));
  Serial.flush();
}

static void setTim3Duty(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;
  if (dutyPercent > 100.0) dutyPercent = 100.0;
  uint32_t compareValue = (uint32_t)(dutyPercent / 100.0 * PWM3_PERIOD_TICKS);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, compareValue);
}

static void configureTim8Pwm() {
  // --- Enable the clocks for the GPIO port and the timer itself ---
  __HAL_RCC_GPIOJ_CLK_ENABLE();   // D11 is physically pin PJ10 on the STM32H747
  __HAL_RCC_TIM8_CLK_ENABLE();    // Peripherals are clock-gated off by default on STM32

  // --- Configure PJ10 as TIM8_CH2's alternate-function output ---
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_10;
  gpio.Mode = GPIO_MODE_AF_PP;              // Alternate function, push-pull
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   // Needed to cleanly switch at 100kHz
  gpio.Alternate = GPIO_AF3_TIM8;           // TIM8's alternate-function number on this pin
  HAL_GPIO_Init(GPIOJ, &gpio);

  // --- Configure TIM8's base timing: 240MHz / 2400 ticks = 100kHz period ---
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;                              // No division -- use the full 240MHz
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = PWM_ARR;                           // 2399 -- sets the 100kHz frequency
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;                      // Required field for advanced timers (TIM1/TIM8)
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_PWM_Init(&htim8);

  // --- Configure Channel 2 for standard PWM (high while counter < compare value) ---
  TIM_OC_InitTypeDef channelConfig = {0};
  channelConfig.OCMode = TIM_OCMODE_PWM1;
  channelConfig.Pulse = 0;                               // Start at 0% duty -- safe default
  channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;      // Required field even though we don't use the complementary output
  channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
  channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
  channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  HAL_TIM_PWM_ConfigChannel(&htim8, &channelConfig, TIM_CHANNEL_2);

  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);

  // TIM1/TIM8 are "advanced control" timers -- unlike general-purpose timers, they have a
  // master output-enable bit (BDTR.MOE) that must be set, or no signal reaches the pin at
  // all even though everything above is configured correctly.
  __HAL_TIM_MOE_ENABLE(&htim8);
}

void setup() {
  Serial.begin(9600);

  configureTim8Pwm();

  // Breadcrumb: print the REAL measured APB1 clock before trusting
  // TIM3_CLOCK_HZ_ASSUMED above -- same verification step that caught
  // TIM8's real 240MHz (vs. a naive 120MHz datasheet assumption).
  uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
  Serial.print(F("Measured APB1 (PCLK1) clock: "));
  Serial.print(pclk1);
  Serial.println(F(" Hz -- if TIM3's real kernel clock (check for a possible x2 multiplier, same rule as TIM8) doesn't match the 120MHz assumed above, PWM3_PERIOD_TICKS needs correcting and pin 5 won't actually be at 100kHz."));
  Serial.flush();

  configureTim3Pwm();

  // WHAT: fix pin 5 at 100% duty (always on) ONCE here, instead of
  //       following the typed duty value like pin 11 does.
  // WHY:  a steady, unchanging reference signal on a second scope channel
  //       makes it much easier to confirm pin 11's duty cycle is actually
  //       changing correctly -- you can see both traces at once and
  //       compare "this one never moves" against "this one should move
  //       exactly as much as I typed." Same reasoning as the earlier
  //       request for an always-on reference pin during the original
  //       duty-cycle debugging.
  setTim3Duty(100.0f);

  Serial.println(F("Giga PWM test (TIM8 on pin 11, variable duty via Serial; TIM3 on pin 5, fixed at 100% as a scope reference)."));
  Serial.println(F("Type a duty cycle 0-100 and press Enter -- only pin 11 changes. Pin 5 stays always on."));
  Serial.println(F("Pin 5's frequency/pin mapping are UNVERIFIED -- check it on the scope before trusting it as a reference."));
}

// ---------------------------------------------------------------------------
// HEARTBEAT: prints once a second so we can tell the sketch is still alive
// (not frozen) even when nobody's typing a new duty value -- same
// breadcrumb-style pattern already used in WiFi_Giga_Test.ino and
// Buck_PWM_Giga_Test.ino. Worth having here now too, since this sketch
// just grew a second timer/pin config that could silently fail to produce
// a signal while the board itself is still running fine.
// ---------------------------------------------------------------------------
static unsigned long lastHeartbeat = 0;
static float currentDuty = 0.0f;   // remembers the last commanded duty, so the heartbeat can report it

void loop() {
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    Serial.print(F("heartbeat | pin 11 duty: "));
    Serial.print(currentDuty);
    Serial.println(F(" % | pin 5: always on (100%, fixed reference)"));
  }

  if (Serial.available() > 0) {
    float requested = Serial.parseFloat();
    while (Serial.available() > 0) {
      Serial.read();
    }

    if (requested < 0.0f)   requested = 0.0f;   // Clamp to 0-100
    if (requested > 100.0f) requested = 100.0f;
    currentDuty = requested;   // remember it for the heartbeat above

    uint32_t compareValue = (uint32_t)((requested / 100.0f) * PWM_PERIOD_TICKS);   // Full-resolution tick count
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, compareValue);                     // Apply directly to the timer (pin 11)

    // NOTE: pin 5 (TIM3) is deliberately NOT touched here anymore -- it's
    // fixed at 100% in setup() as an always-on scope reference, and stays
    // that way regardless of what's typed. Only pin 11 responds to input.

    Serial.print(F("Duty set to: "));
    Serial.print(requested);
    Serial.print(F(" % (pin 11 compare="));
    Serial.print(compareValue);
    Serial.print(F("/"));
    Serial.print(PWM_PERIOD_TICKS);
    Serial.println(F(") -- pin 5 stays fixed at 100% (reference), unaffected by this"));
  }
}
