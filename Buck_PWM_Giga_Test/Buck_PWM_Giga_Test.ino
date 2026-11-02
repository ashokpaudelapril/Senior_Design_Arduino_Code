// =====================================================================
// Buck_PWM_Giga_Test
//
// WHAT: A standalone, throwaway sketch that tests ONE thing: whether
//       pin D1 on the Giga actually carries TIM1 Channel 2's PWM output
//       when configured via direct STM32 HAL, the same way PWM_Giga_Test
//       verified pin 11 / TIM8 Channel 2 before BoostConverter.cpp ever
//       used it for real.
//
// HOW:  Same overall approach as PWM_Giga_Test -- configure the timer
//       directly through STM32 HAL (bypassing Mbed's PwmOut, already
//       proven too coarse for this project), then let you type a duty
//       cycle 0-100 into Serial Monitor and check it on a scope.
//
// WHY THIS EXISTS BEFORE BuckConverter.cpp: the GPIO port and alternate-
//       function number for "D1 = TIM1 CH2" is NOT a confirmed fact --
//       it's inferred from common STM32 pin conventions (PA9 very often
//       serves both USART1_TX and TIM1_CH2 via AF1 across STM32 parts
//       generally), sourced from a third-party pin reference (OpenMV's
//       docs), not Arduino's own pinout page or the STM32H747 datasheet
//       directly. The exact values below (GPIOA, PIN_9, AF1) are a
//       best-reasoned GUESS, not a verified fact -- this sketch exists
//       to find out if that guess is right BEFORE it goes anywhere near
//       the real buck converter firmware. This is exactly the lesson
//       learned from the boost converter's PWM resolution problem:
//       something that looks right from documentation/convention isn't
//       the same as something confirmed on a scope.
//
// WHAT TO CHECK ON THE SCOPE:
//   1. Is there a signal on D1 at all? (confirms the pin/AF guess)
//   2. Is the frequency actually ~100kHz? (confirms the timer clock
//      assumption -- TIM1 should behave the same as TIM8 here, since
//      both are "advanced control" timers on the same APB2 bus, but
//      this has NOT been separately measured the way TIM8's 240MHz
//      kernel clock was)
//   3. Does duty cycle actually change smoothly, not in coarse ~10%
//      jumps? (confirms this isn't hitting the same Mbed-style rounding
//      problem -- though since this bypasses Mbed entirely via direct
//      HAL, it shouldn't, but confirm instead of assuming)
// =====================================================================

#include <Arduino.h>
#include "Config.h"   // BUCK_GATE_DRIVE_PIN, BUCK_PWM_FREQUENCY_HZ -- shares the same constants the real module will use

extern "C" {
#include "stm32h7xx_hal.h"
}

// ---------------------------------------------------------------------
// BLOCK 1: timer math -- same approach as Config.h's PWM_PERIOD_TICKS,
// but using TIM1_CLOCK_HZ as a SEPARATE, UNVERIFIED assumption.
//
// WHAT: TIM8's kernel clock was measured directly on real hardware
//       (240MHz, see Config.h BLOCK 21) -- NOT assumed from a datasheet.
//       TIM1 is also an "advanced control" timer on the same APB2 bus,
//       so it SHOULD have the same 240MHz kernel clock by the same
//       2x-multiplier reasoning -- but "should" is not "measured". This
//       sketch prints the result of HAL_RCC_GetPCLK2Freq() at startup so
//       we can confirm this assumption the same way TIM8's was
//       confirmed, instead of quietly inheriting an unverified number.
// ---------------------------------------------------------------------
#define TIM1_CLOCK_HZ_ASSUMED   240000000UL                                  // UNVERIFIED -- see breadcrumb print in setup()
#define BUCK_PWM_PERIOD_TICKS   (TIM1_CLOCK_HZ_ASSUMED / BUCK_PWM_FREQUENCY_HZ)
#define BUCK_PWM_ARR_VALUE      (BUCK_PWM_PERIOD_TICKS - 1)

static TIM_HandleTypeDef htim1;

// ---------------------------------------------------------------------
// BLOCK 2: configureTim1Pwm() -- mirrors BoostConverter.cpp's TIM8
// setup almost exactly, swapped to TIM1/GPIOA/PIN_9/AF1.
//
// WHAT: Configures PA9 (the GUESSED physical pin for D1) as TIM1
//       Channel 2's alternate-function output, then sets up the timer
//       for ~100kHz PWM, same structure as the already-proven TIM8 code.
// ---------------------------------------------------------------------
void configureTim1Pwm() {
  Serial.println(F("Step 1: enabling GPIOA and TIM1 clocks..."));
  Serial.flush();
  __HAL_RCC_GPIOA_CLK_ENABLE();   // GUESS: D1 is physically PA9 -- to be confirmed by this sketch
  __HAL_RCC_TIM1_CLK_ENABLE();

  Serial.println(F("Step 2: configuring PA9 as TIM1_CH2 alternate function..."));
  Serial.flush();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_9;                     // GUESS -- see header comment
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF1_TIM1;            // GUESS -- AF1 is TIM1's usual alternate-function number on STM32 parts generally
  HAL_GPIO_Init(GPIOA, &gpio);

  Serial.println(F("Step 3: configuring TIM1 base timing..."));
  Serial.flush();
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = BUCK_PWM_ARR_VALUE;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;          // Required field for advanced timers (TIM1/TIM8), same as BoostConverter.cpp
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_PWM_Init(&htim1);

  Serial.println(F("Step 4: configuring Channel 2 for PWM..."));
  Serial.flush();
  TIM_OC_InitTypeDef channelConfig = {0};
  channelConfig.OCMode = TIM_OCMODE_PWM1;
  channelConfig.Pulse = 0;                   // start at 0% -- safe default
  channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
  channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
  channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  HAL_TIM_PWM_ConfigChannel(&htim1, &channelConfig, TIM_CHANNEL_2);

  Serial.println(F("Step 5: starting PWM + enabling master output..."));
  Serial.flush();
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  __HAL_TIM_MOE_ENABLE(&htim1);   // same advanced-timer gotcha as TIM8 -- required or no signal reaches the pin

  Serial.println(F("Step 6: TIM1 PWM configuration complete."));
  Serial.flush();
}

void setTim1Duty(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;
  if (dutyPercent > 100.0) dutyPercent = 100.0;
  uint32_t compareValue = (uint32_t)(dutyPercent / 100.0 * BUCK_PWM_PERIOD_TICKS);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, compareValue);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // wait for Serial Monitor -- same reasoning as every other test sketch in this project
  }

  Serial.println(F("--- Buck_PWM_Giga_Test start ---"));

  // Breadcrumb: confirm the APB2 clock assumption BEFORE trusting the
  // 240MHz-derived period math above, same verification step that
  // caught the real number for TIM8.
  uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();
  Serial.print(F("Measured APB2 (PCLK2) clock: "));
  Serial.print(pclk2);
  Serial.println(F(" Hz -- compare against the 120MHz seen for TIM8's bus; if this differs, TIM1_CLOCK_HZ_ASSUMED above needs correcting."));
  Serial.flush();

  configureTim1Pwm();

  Serial.println(F("MANUAL PWM TEST: type a duty cycle 0-100 and press Enter."));
  Serial.println(F("Check pin D1 (PA9, if the guess is right) on the scope."));
  Serial.flush();
}

void loop() {
  if (Serial.available() > 0) {
    float duty = Serial.parseFloat();
    while (Serial.available() > 0) {
      Serial.read();   // clear the trailing newline/extra characters
    }
    setTim1Duty(duty);
    Serial.print(F("Duty set to: "));
    Serial.print(duty, 2);
    Serial.println(F(" %"));
  }
}
