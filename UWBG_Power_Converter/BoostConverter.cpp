// ---------------------------------------------------------------------------
// BOOST CONVERTER MODULE (implementation)
//
// WHAT: Drives the gate-driver PWM pin. boost_init() sets a safe starting
//       state; boost_setDutyCycle() is the only way to change it.
//
// HOW:  On the Giga R1 WiFi, pin 11 (D11) maps to STM32's TIM8 Channel 2.
//       This module configures that timer directly through STM32 HAL,
//       bypassing Arduino's normal analogWrite()/PwmOut layer entirely.
//       TIM8 is set to count from 0 up to PWM_ARR_VALUE (2399, from
//       Config.h) at its measured 240MHz kernel clock, then wrap back to 0
//       and repeat -- that repeat rate is our ~100kHz switching frequency.
//       The channel's compare register sets WHERE in that count the output
//       pin switches from high to low, which is what determines duty
//       cycle: a bigger compare value means the pin stays high longer
//       within each cycle.
//
//       Callers (ChargeController, TestMode) pass a 0-100% value and don't
//       need to know the internals are STM32 timer registers underneath --
//       this module replaced the Mega's AVR Timer1 code when the project
//       moved to the Giga, and nothing outside this file changed.
//
// WHY THIS APPROACH (not Mbed's PwmOut): PwmOut's write() and pulsewidth()
//       were both tried first, since that's the normal Arduino-level way to
//       do PWM on this board. Scope measurement showed both only produce
//       ~10% duty steps -- values like 25-29% all rounded down to 20%,
//       regardless of which entry point was used. That meant the
//       limitation was inside Mbed's own PWM implementation for this
//       timer, not something fixable by calling a different convenience
//       method. Driving TIM8 directly gives 2400 steps per period
//       (~0.042% resolution), confirmed correct on the scope at 35%, 55%,
//       and 58% -- values that specifically exposed the old rounding.
//
// WHY THE CLAMP: boost_setDutyCycle() clamps its input to 0-100% before
//       doing anything else. Without that, a bug anywhere else in the
//       project (e.g. a control-loop calculation going negative or
//       overflowing) could command an out-of-range duty cycle -- on a real
//       boost converter, a stuck-on or miscalculated switch can overheat
//       the GaN device or dump excess voltage onto the battery. Clamping
//       here, in the one function that actually touches the pin, means
//       every caller is protected without having to remember to check it
//       themselves.
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include "BoostConverter.h"
#include "Config.h"

extern "C" {
#include "stm32h7xx_hal.h"   // Direct timer/GPIO register access -- not available through Arduino's own API
}

static TIM_HandleTypeDef htim8;   // Kept private to this module -- nothing outside needs to know a timer handle exists

void boost_init() {
  // --- Enable the clocks for the GPIO port and the timer itself ---
  __HAL_RCC_GPIOJ_CLK_ENABLE();   // Pin 11 / D11 is physically pin PJ10 on the STM32H747
  __HAL_RCC_TIM8_CLK_ENABLE();    // Peripherals are clock-gated off by default on STM32 -- must enable before use

  // --- Configure PJ10 as TIM8_CH2's alternate-function output ---
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_10;
  gpio.Mode = GPIO_MODE_AF_PP;              // Alternate function, push-pull (the pin is driven by the timer, not by us directly)
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   // Needed so the pin can cleanly switch at 100kHz
  gpio.Alternate = GPIO_AF3_TIM8;           // TIM8's alternate-function number on this specific pin
  HAL_GPIO_Init(GPIOJ, &gpio);

  // --- Configure TIM8's base timing: 240MHz / 2400 ticks = 100kHz period ---
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;                              // No division -- use the full measured 240MHz
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = PWM_ARR_VALUE;                     // 2399 -- sets the 100kHz frequency
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;                      // Required field for advanced timers (TIM1/TIM8), unused here
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_PWM_Init(&htim8);

  // --- Configure Channel 2 for standard PWM (high while counter < compare value) ---
  TIM_OC_InitTypeDef channelConfig = {0};
  channelConfig.OCMode = TIM_OCMODE_PWM1;
  channelConfig.Pulse = 0;                               // Start at 0% duty -- safe default, switch off
  channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;      // Required field even though the complementary output is unused
  channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
  channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
  channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  HAL_TIM_PWM_ConfigChannel(&htim8, &channelConfig, TIM_CHANNEL_2);

  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);

  // TIM1/TIM8 are "advanced control" timers -- unlike general-purpose timers, they have a
  // master output-enable bit (BDTR.MOE) that must be set, or no signal reaches the pin at
  // all even though everything above is configured correctly. HAL_TIM_PWM_Start() normally
  // sets this automatically; it's explicitly re-asserted here as a safety net against this
  // specific, well-known gotcha for these two timers.
  __HAL_TIM_MOE_ENABLE(&htim8);

  boost_setDutyCycle(0.0);   // Redundant with Pulse=0 above, but explicit: confirm 0% as the starting state
}

void boost_setDutyCycle(float dutyPercent) {
  if (dutyPercent < 0.0)   dutyPercent = 0.0;    // Clamp: never allow negative duty cycle
  if (dutyPercent > 100.0) dutyPercent = 100.0;  // Clamp: never allow more than 100% duty cycle

  uint32_t compareValue = (uint32_t)(dutyPercent / 100.0 * PWM_PERIOD_TICKS);  // Convert 0-100% into a 0-2400 tick count
  __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, compareValue);                   // Timer compares its count to this value each cycle
}
