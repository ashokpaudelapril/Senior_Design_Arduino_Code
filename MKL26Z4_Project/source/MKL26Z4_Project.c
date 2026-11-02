/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MKL26Z4_Project.c
 * @brief   Buck converter PWM stopgap test on the FRDM-KL26Z (MKL26Z128),
 *          while the project's damaged Giga R1 WiFi is being replaced.
 *
 * WHAT: Produces a PWM signal on pin D3 (PTA12, TPM1 Channel 0) and lets
 *       you set the duty cycle by typing a number (0-100) over the
 *       debug console (115200 baud, already wired to PTA1/PTA2 by the
 *       generated board files) -- same idea as every other PWM test
 *       sketch in this project, on yet another board.
 *
 * WHY THIS BOARD, RIGHT NOW: same situation as Buck_PWM_Nano_Test --
 *       this is a stopgap to check the buck converter's basic PWM
 *       mechanism while waiting for Giga hardware. "Then use Giga to do
 *       everything" -- this board is for an early mechanism check, not
 *       the final verified implementation.
 *
 * TWO THINGS HERE ARE UNVERIFIED -- STATED UP FRONT, NOT GLOSSED OVER:
 *
 *   1. PIN MAPPING: D3 = PTA12 = TPM1_CH0 is inferred from the sibling
 *      FRDM-KL25Z board's schematic (same silicon family, same
 *      Arduino-compatible header layout), NOT confirmed directly
 *      against the FRDM-KL26Z's own schematic. The two boards are
 *      widely known to share this header design, but "widely known to"
 *      is not "confirmed for this exact board" -- check the actual
 *      FRDM-KL26Z user manual or probe the pin directly before fully
 *      trusting it, same discipline as the Giga's D1/D5 pin guesses.
 *
 *   2. TPM CLOCK SOURCE: TPM modules on this chip do NOT automatically
 *      run at the 48MHz core clock -- they have their own separate
 *      clock-source mux (SIM->SOPT2 TPMSRC). This code selects the
 *      MCGFLLCLK/MCGPLLCLK source (TPMSRC=1, the SDK's CLOCK_SetTpmClock(1))
 *      and then QUERIES the actual resulting frequency at runtime via
 *      CLOCK_GetFreq() rather than assuming a number -- same "measure,
 *      don't assume" lesson the Giga's TIM8 clock taught this project.
 *
 * HOW (DUTY CYCLE RESOLUTION): NXP's SDK convenience function
 *       TPM_UpdatePwmDutycycle() only accepts a whole-percent integer
 *       (0-100) -- a coarse convenience wrapper, the same category of
 *       resolution trap as Mbed's PwmOut on the Giga. Instead, this
 *       code writes directly to the channel's compare register
 *       (TPM1->CONTROLS[0].CnV) relative to the period register
 *       (TPM1->MOD), giving the hardware's real resolution instead of
 *       being limited to 101 discrete steps.
 *
 * SAFETY: No safety limits in this code. Keep the gate driver and GaN
 *         switch disconnected while probing, same as every other PWM
 *         test in this project.
 */
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_tpm.h"
#include "fsl_clock.h"

/* ---------------------------------------------------------------------
 * BLOCK 1: PWM target frequency and the pin/channel this drives.
 *
 * WHAT: Same ~100kHz target as every other PWM test in this project,
 *       for consistency with the real buck converter design.
 * --------------------------------------------------------------------- */
#define PWM_FREQUENCY_HZ 100000U
#define TPM_PWM_CHANNEL  kTPM_Chnl_0   /* PTA12 / D3, per the header note above -- UNVERIFIED for KL26Z specifically */

static uint32_t pwmPeriodTicks = 0;   /* filled in at runtime once the real TPM clock is known -- see configurePwm() */

/* ---------------------------------------------------------------------
 * BLOCK 2: configurePwm() -- pin mux, TPM clock source, and PWM setup.
 *
 * WHAT: Routes PTA12 to its TPM1_CH0 alternate function, selects and
 *       measures the TPM clock source, then configures TPM1 for edge-
 *       aligned PWM at PWM_FREQUENCY_HZ.
 * HOW:  Uses the SDK's own TPM_Init()/TPM_SetupPwm() for the one-time
 *       setup (these are NXP's own supported driver calls, not bypassed
 *       -- unlike Mbed's PwmOut on the Giga, there's no established
 *       reason to distrust them for INITIAL configuration; the
 *       resolution problem only shows up in the per-update convenience
 *       function, handled separately in setDutyCycle() below).
 * --------------------------------------------------------------------- */
static void configurePwm(void) {
    PRINTF("Step 1: enabling PORTA clock and routing PTA12 to TPM1_CH0...\r\n");

    CLOCK_EnableClock(kCLOCK_PortA);
    /* GUESS: ALT3 is the typical TPM alternate-function number on many
     * Kinetis ports, but this has NOT been individually confirmed for
     * PTA12's TPM1_CH0 function on this exact chip -- verify against
     * the MKL26Z128 reference manual's pin mux table, or the scope,
     * before trusting it. */
    PORT_SetPinMux(PORTA, 12U, kPORT_MuxAlt3);

    PRINTF("Step 2: selecting and measuring the TPM clock source...\r\n");
    CLOCK_SetTpmClock(1U);   /* TPMSRC = 1 -- selects MCGFLLCLK/MCGPLLCLK per the active MCG mode */

    uint32_t tpmClockHz = CLOCK_GetFreq(kCLOCK_McgFllClk);
    PRINTF("Measured TPM clock source: %lu Hz -- this is what period math below is based on, not an assumed number.\r\n", (unsigned long)tpmClockHz);

    pwmPeriodTicks = tpmClockHz / PWM_FREQUENCY_HZ;
    PRINTF("Computed PWM period: %lu ticks for a %u Hz target.\r\n", (unsigned long)pwmPeriodTicks, PWM_FREQUENCY_HZ);

    PRINTF("Step 3: configuring TPM1 for edge-aligned PWM...\r\n");
    tpm_config_t tpmInfo;
    TPM_GetDefaultConfig(&tpmInfo);
    TPM_Init(TPM1, &tpmInfo);

    tpm_chnl_pwm_signal_param_t tpmParam;
    tpmParam.chnlNumber = TPM_PWM_CHANNEL;
    tpmParam.level = kTPM_HighTrue;   /* non-inverting: higher compare value = more time high, same convention as every other PWM sketch here */
    tpmParam.dutyCyclePercent = 0;    /* start at 0% -- safe default */

    TPM_SetupPwm(TPM1, &tpmParam, 1U, kTPM_EdgeAlignedPwm, PWM_FREQUENCY_HZ, tpmClockHz);
    TPM_StartTimer(TPM1, kTPM_SystemClock);

    PRINTF("Step 4: PWM configuration complete.\r\n");
}

/* ---------------------------------------------------------------------
 * BLOCK 3: setDutyCycle() -- direct register write, full resolution.
 *
 * WHAT: Sets the channel's compare register directly, as a fraction of
 *       pwmPeriodTicks, instead of calling TPM_UpdatePwmDutycycle()
 *       (which only accepts a whole-percent integer -- see the file
 *       header note on why that's not used here).
 * --------------------------------------------------------------------- */
static void setDutyCycle(float dutyPercent) {
    if (dutyPercent < 0.0f)   dutyPercent = 0.0f;
    if (dutyPercent > 100.0f) dutyPercent = 100.0f;

    uint32_t compareValue = (uint32_t)((dutyPercent / 100.0f) * (float)pwmPeriodTicks);
    TPM1->CONTROLS[TPM_PWM_CHANNEL].CnV = compareValue;   /* applied directly to the hardware channel register */
}

/* ---------------------------------------------------------------------
 * BLOCK 4: a small non-blocking input buffer for typed duty values.
 *
 * WHAT: Builds up typed digits into a line buffer and parses it once
 *       Enter is pressed -- the same "type a number, press Enter"
 *       interaction as the other PWM test sketches, implemented with
 *       this SDK's own non-blocking character read instead of
 *       Arduino's Serial.available()/parseFloat().
 * HOW:  DbgConsole_TryGetchar() is the SDK's documented non-blocking
 *       read -- returns immediately whether or not a character is
 *       waiting, so this never stalls the heartbeat below, same
 *       non-blocking discipline as every other sketch in this project.
 * --------------------------------------------------------------------- */
#define INPUT_BUFFER_SIZE 16
static char inputBuffer[INPUT_BUFFER_SIZE];
static uint8_t inputLength = 0;

static void pollSerialInput(void) {
    char c;
    if (DbgConsole_TryGetchar(&c) != kStatus_Success) {
        return;   /* nothing waiting -- same "empty means nobody's typed anything" idea as server.available() elsewhere in this project */
    }

    if (c == '\r' || c == '\n') {
        if (inputLength > 0) {
            inputBuffer[inputLength] = '\0';
            float requested = (float)atof(inputBuffer);
            setDutyCycle(requested);
            PRINTF("Duty set to: %d%% (compare=%lu/%lu)\r\n", (int)requested, (unsigned long)TPM1->CONTROLS[TPM_PWM_CHANNEL].CnV, (unsigned long)pwmPeriodTicks);
            inputLength = 0;
        }
    } else if (inputLength < INPUT_BUFFER_SIZE - 1) {
        inputBuffer[inputLength] = c;
        inputLength++;
    }
    /* characters beyond the buffer size are silently dropped -- fine for a short "0-100" test input, not meant to be a general-purpose console */
}

/* ---------------------------------------------------------------------
 * BLOCK 5: heartbeat -- same pattern as every other test sketch in this
 * project. There's no millis() in bare-metal SDK code, so this uses a
 * simple loop counter instead of real elapsed time -- coarser than the
 * other sketches' millis()-based heartbeats, but serves the same
 * purpose: confirm the board is still alive and looping, not frozen.
 * --------------------------------------------------------------------- */
#define HEARTBEAT_LOOP_COUNT 2000000U   /* rough tuning, not a calibrated time interval -- adjust if it prints too fast/slow */

int main(void) {
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    PRINTF("--- Buck_PWM_NXP_Test (MKL26Z128 / FRDM-KL26Z) start ---\r\n");
    PRINTF("Stopgap buck converter PWM check while the Giga replacement is on order.\r\n");

    configurePwm();

    PRINTF("Type a duty cycle 0-100 and press Enter. Pin D3 (PTA12, TPM1_CH0) is the gate-drive output.\r\n");
    PRINTF("Pin/clock mapping are UNVERIFIED for this exact board -- check on the scope before trusting.\r\n");

    uint32_t heartbeatCounter = 0;

    while (1) {
        pollSerialInput();

        heartbeatCounter++;
        if (heartbeatCounter >= HEARTBEAT_LOOP_COUNT) {
            heartbeatCounter = 0;
            PRINTF("heartbeat | duty compare: %lu/%lu\r\n", (unsigned long)TPM1->CONTROLS[TPM_PWM_CHANNEL].CnV, (unsigned long)pwmPeriodTicks);
        }
    }

    return 0;
}
