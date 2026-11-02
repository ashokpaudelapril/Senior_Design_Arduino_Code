// =====================================================================
// CurrentControl_Giga_Test
//
// WHAT: The same closed-loop current-control concept already proven on
//       the UNO (CurrentControl_Uno_Test.ino), now on the real Giga
//       with the real touchscreen -- a Start button begins the
//       control loop (holding 1.0A via the step controller), a Stop
//       button halts it and forces duty cycle back to 0% immediately.
//
// WHY THIS APPROACH: the PWM mechanism (TIM8/D11) is copied verbatim
//       from the already-validated PWM_Giga_Test.ino -- not
//       re-derived, since that register configuration was already
//       confirmed correct on the scope. The display/touch pin
//       assignments and button hit-test pattern are copied from the
//       real firmware's Display.cpp (its relay-button layout), for
//       the same reason -- reuse what's already proven rather than
//       re-guess pin numbers or touch-handling logic.
//
// ASSUMPTION STATED UP FRONT: INA226 current feedback, same as the
//       UNO version, now on the Giga's I2C bus (pins 20/21, shared
//       with the touch controller -- same bus, per Config.h).
//
// SAFETY: Duty cycle starts at 0% and stays there until Start is
//         tapped (same start-gate reasoning as the UNO version, now a
//         touch button instead of a typed Serial command). Stop
//         forces an immediate return to 0% at any time. No hard
//         overcurrent trip independent of the control loop yet --
//         watch the bench closely once started.
// =====================================================================

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7796S_kbv.h>
#include <FT6336U.h>
#include <INA226_WE.h>

extern "C" {
#include "stm32h7xx_hal.h"
}

// WHAT: SimpleButton is defined here, immediately after the includes,
//       instead of down in BLOCK 4 where it's actually used.
// WHY:  Arduino IDE auto-generates forward declarations for every
//       function in the sketch and inserts them near the top of the
//       file, before anything else -- including before a struct
//       that's defined further down but used in a function signature.
//       That auto-generated prototype for drawButton() then references
//       SimpleButton before the compiler has seen what that type is,
//       causing "SimpleButton does not name a type" even though the
//       struct and its use are in the correct order relative to each
//       other in this file. Moving the struct itself to the top avoids
//       the problem entirely.
struct SimpleButton {
  const char* label;
  int16_t x, y, w, h;
};

// ---------------------------------------------------------------------
// BLOCK 1: TIM8 PWM setup -- copied verbatim from the already-validated
// PWM_Giga_Test.ino. See that file for the full explanation of why
// direct TIM8 register access is used instead of Mbed's PwmOut.
// ---------------------------------------------------------------------
static const uint32_t TIM8_CLOCK_HZ = 240000000UL;     // Measured on real hardware
static const uint32_t PWM_PERIOD_TICKS = 2400;         // 240MHz / 2400 = exactly 100kHz
static const uint32_t PWM_ARR = PWM_PERIOD_TICKS - 1;

static TIM_HandleTypeDef htim8;

static void configureTim8Pwm() {
  __HAL_RCC_GPIOJ_CLK_ENABLE();   // D11 is physically pin PJ10 on the STM32H747
  __HAL_RCC_TIM8_CLK_ENABLE();

  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_10;
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  gpio.Alternate = GPIO_AF3_TIM8;
  HAL_GPIO_Init(GPIOJ, &gpio);

  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = PWM_ARR;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_PWM_Init(&htim8);

  TIM_OC_InitTypeDef channelConfig = {0};
  channelConfig.OCMode = TIM_OCMODE_PWM1;
  channelConfig.Pulse = 0;
  channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
  channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
  channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
  channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  HAL_TIM_PWM_ConfigChannel(&htim8, &channelConfig, TIM_CHANNEL_2);

  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
  __HAL_TIM_MOE_ENABLE(&htim8);   // Advanced-control timer -- no signal without this
}

static void setDuty(float dutyPercent) {
  if (dutyPercent < 0.0f)   dutyPercent = 0.0f;
  if (dutyPercent > 100.0f) dutyPercent = 100.0f;
  uint32_t compareValue = (uint32_t)((dutyPercent / 100.0f) * PWM_PERIOD_TICKS);
  __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_2, compareValue);
}

// ---------------------------------------------------------------------
// BLOCK 2: INA226 sensor setup -- same INA226_WE library and shunt
// values as CurrentControl_Uno_Test.ino, now on the Giga's I2C bus.
// ---------------------------------------------------------------------
#define INA226_I2C_ADDRESS   0x40   // CHECK against the real module's address jumper -- confirmed 0x40 on the bench already for this sensor
#define SHUNT_RESISTOR_OHMS  0.010
#define MAX_EXPECTED_AMPS    2.0

INA226_WE ina226 = INA226_WE(INA226_I2C_ADDRESS);

static void configureIna226() {
  ina226.init();
  ina226.setResistorRange(SHUNT_RESISTOR_OHMS, MAX_EXPECTED_AMPS);
}

// ---------------------------------------------------------------------
// BLOCK 3: display + touch setup -- same pins and library calls as the
// real firmware's Display.cpp (LCD_CS=10, LCD_RST=9, LCD_RS=8,
// LCD_LED=7, TOUCH_RST=6, TOUCH_INT=3, per Config.h). A much simpler
// screen than the real tabbed UI -- just a title, live readings, and
// two buttons -- since this sketch only needs to prove Start/Stop.
// ---------------------------------------------------------------------
#define LCD_CS_PIN    10
#define LCD_RST_PIN   9
#define LCD_RS_PIN    8
#define LCD_LED_PIN   7

// UPDATED: CTP_RST and CTP_INT are now actually wired (D6 and D3,
// matching the real firmware's Config.h) -- the earlier 4/5 placeholder
// pins were from when these were thought to be unconnected, which was
// the real cause of the touch hang: the code was controlling pins that
// weren't wired to anything, while the actual RST/INT lines never got
// toggled or read at all. Reverting touch polling to enabled now that
// the pins match where the wires actually are -- see DISABLE_TOUCH_POLLING
// further down.
#define TOUCH_RST_PIN 6
#define TOUCH_INT_PIN 3

Adafruit_ST7796S_kbv tft(LCD_CS_PIN, LCD_RS_PIN, LCD_RST_PIN);
FT6336U touchCtrl(TOUCH_RST_PIN, TOUCH_INT_PIN);

static const uint16_t COLOR_BLACK  = 0x0000;
static const uint16_t COLOR_WHITE  = 0xFFFF;
static const uint16_t COLOR_GREEN  = 0x07E0;
static const uint16_t COLOR_RED    = 0xF800;
static const uint16_t COLOR_GREY   = 0x8410;

static const uint16_t SCREEN_W = 480;   // Landscape panel dimensions, matching the real firmware's Display.cpp -- used to mirror touch coordinates for the 180-degree rotation flip
static const uint16_t SCREEN_H = 320;

static void configureDisplay() {
  pinMode(LCD_LED_PIN, OUTPUT);
  digitalWrite(LCD_LED_PIN, HIGH);   // Backlight on

  tft.begin();
  // WHAT: Rotation 3, not 1 -- confirmed on the bench that rotation 1
  //       displayed everything upside down for how this panel is
  //       physically mounted. Rotation values step through 0-3 in 90
  //       degree increments, so 3 = 1 + 180 degrees, same landscape
  //       orientation just flipped the right way for this mounting.
  tft.setRotation(3);

  tft.fillScreen(COLOR_BLACK);   // DIAGNOSTIC red fill removed now that SPI communication is confirmed working

  touchCtrl.begin();   // Shares the same I2C bus as the INA226 -- Wire.begin() already called once in setup() below, not repeated here
}

// ---------------------------------------------------------------------
// BLOCK 4: Start/Stop buttons -- same layout-struct-plus-hit-test
// pattern as the real firmware's relay buttons (Display.cpp's
// RELAY_BUTTONS/drawRelayButton/handleRelayTabTouch), just two buttons
// instead of six and a different action when tapped. (SimpleButton
// itself is defined at the top of the file -- see the note there.)
// ---------------------------------------------------------------------
static const SimpleButton START_BUTTON = { "START", 40,  120, 180, 80 };
static const SimpleButton STOP_BUTTON  = { "STOP",  260, 120, 180, 80 };

static void drawButton(const SimpleButton& btn, uint16_t bgColor) {
  tft.fillRect(btn.x, btn.y, btn.w, btn.h, bgColor);
  tft.drawRect(btn.x, btn.y, btn.w, btn.h, COLOR_WHITE);
  tft.setTextSize(3);
  tft.setTextColor(COLOR_BLACK, bgColor);
  tft.setCursor(btn.x + 30, btn.y + 30);
  tft.print(btn.label);
}

static bool touchIsInsideButton(uint16_t x, uint16_t y, const SimpleButton& btn) {
  bool withinX = (x >= btn.x) && (x < btn.x + btn.w);
  bool withinY = (y >= btn.y) && (y < btn.y + btn.h);
  return withinX && withinY;
}

// ---------------------------------------------------------------------
// BLOCK 5: the step controller -- identical logic to
// CurrentControl_Uno_Test.ino, including the 80% safety ceiling and
// the fix for the "tracked duty climbs past 100%" bug found on the
// UNO bench. Same step size (0.5%) that was already found to behave
// better than a smaller one against this sensor's real noise.
// ---------------------------------------------------------------------
#define TARGET_CURRENT_A      1.0
#define STEP_SIZE_PERCENT     0.5
#define CONTROL_INTERVAL_MS   100
#define MAX_DUTY_PERCENT      80.0

static float currentDutyPercent = 0.0;
static unsigned long lastControlTick = 0;
static bool controlLoopRunning = false;   // Touch-button equivalent of the UNO version's Serial start-gate

// WHAT: DEAD_BAND_A -- how far ABOVE TARGET_CURRENT_A measured current
//       is allowed to drift before the controller corrects it back
//       down. There is deliberately NO lower-side margin anymore.
// WHY:  A symmetric band (stop anywhere within target +/- band) was
//       found to lock in as soon as current first crossed the LOWER
//       edge while climbing -- e.g. settling around 0.98A instead of
//       actually reaching 1.0A, since it stopped the instant it
//       entered the band rather than continuing to the real target.
//       Making the lower side strict (must reach at least
//       TARGET_CURRENT_A exactly before climbing stops) guarantees it
//       always reaches the real 1.0A target first. The upper-side
//       band still exists so small noise just above target doesn't
//       trigger constant step-down corrections -- it only corrects
//       back down if it drifts meaningfully past the top of the band.
#define DEAD_BAND_A   0.03

static void runControlStep() {
  float measuredAmps = ina226.getCurrent_mA() / 1000.0;

  if (measuredAmps < TARGET_CURRENT_A) {
    currentDutyPercent += STEP_SIZE_PERCENT;   // Below target, no tolerance -- keep climbing until actually at/above 1.0A
  } else if (measuredAmps > TARGET_CURRENT_A + DEAD_BAND_A) {
    currentDutyPercent -= STEP_SIZE_PERCENT;   // Meaningfully above target -- step down
  }
  // Between TARGET_CURRENT_A and TARGET_CURRENT_A + DEAD_BAND_A: hold -- duty cycle stays exactly where it is

  if (currentDutyPercent > MAX_DUTY_PERCENT) currentDutyPercent = MAX_DUTY_PERCENT;
  if (currentDutyPercent < 0.0)              currentDutyPercent = 0.0;

  setDuty(currentDutyPercent);
}

// ---------------------------------------------------------------------
// BLOCK 5.5: independent hard overcurrent trip -- the "brakes," not
// the "cruise control." This does NOT depend on the step controller's
// own logic being correct -- it reads current directly and acts on
// its own, the same role Feature 3's Hard Safety Limit Check plays in
// the real firmware.
//
// WHAT: If measured current ever exceeds HARD_CURRENT_LIMIT_A, force
//       duty to 0% immediately and LATCH a fault -- the control loop
//       will not run again, and START taps are ignored, until the
//       board is reset.
// HOW:  1.5A was chosen to match the real boost converter's own hard
//       safety limit (per the Product Specification: 1.0A CC target,
//       1.5A hard limit -- same ~50% headroom pattern used
//       throughout this project's other safety margins).
// WHY LATCHED, NOT AUTO-CLEARING: matches the real firmware's own
//       documented behavior ("faults latch... clearing requires a
//       manual reset") -- a fault should force a person to look at
//       what happened, not silently retry and potentially trip again
//       and again unattended.
// WHY CHECKED EVERY loop() PASS, NOT ON THE 100ms CONTROL INTERVAL:
//       this needs to react faster than the control loop's own
//       pace -- the whole point is catching a problem the control
//       loop itself might be causing or failing to catch in time.
// ---------------------------------------------------------------------
#define HARD_CURRENT_LIMIT_A   1.5

static bool faulted = false;

static void checkHardLimit() {
  if (faulted) {
    return;   // Already tripped -- nothing more to check until reset
  }

  float measuredAmps = ina226.getCurrent_mA() / 1000.0;

  if (measuredAmps > HARD_CURRENT_LIMIT_A) {
    faulted = true;
    controlLoopRunning = false;
    currentDutyPercent = 0.0;
    setDuty(0.0);   // Applied immediately, bypassing the control loop entirely
    Serial.print(F(">>> FAULT: measured current "));
    Serial.print(measuredAmps, 3);
    Serial.print(F(" A exceeded the "));
    Serial.print(HARD_CURRENT_LIMIT_A);
    Serial.println(F(" A hard limit. Duty forced to 0% and LATCHED. Reset the board to clear. <<<"));
  }
}

// ---------------------------------------------------------------------
// BLOCK 6: the screen's live readout, Start/Stop handling, and
// setup()/loop() tying everything together.
//
// WHAT: Draws the title, Start/Stop buttons, and live measured
//       current / duty cycle. Tapping START begins runControlStep()
//       on its normal interval; tapping STOP immediately forces duty
//       back to 0% and halts the loop, same "stop means stop right
//       now" behavior as the real firmware's safety trips.
// ---------------------------------------------------------------------
static void drawStaticScreen() {
  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setCursor(60, 20);
  tft.print(F("Boost Current Control Test"));

  drawButton(START_BUTTON, COLOR_GREEN);
  drawButton(STOP_BUTTON, COLOR_RED);
}

static void drawLiveReadout(float measuredAmps) {
  tft.fillRect(40, 230, 400, 70, COLOR_BLACK);   // Clear previous readout before redrawing

  tft.setTextSize(2);
  tft.setTextColor(COLOR_WHITE, COLOR_BLACK);
  tft.setCursor(40, 230);
  tft.print(F("Measured: "));
  tft.print(measuredAmps, 3);
  tft.print(F(" A"));

  tft.setCursor(40, 260);
  tft.print(F("Duty: "));
  tft.print(currentDutyPercent, 1);
  tft.print(F(" %"));

  tft.setCursor(40, 290);
  if (faulted) {
    tft.setTextColor(COLOR_RED, COLOR_BLACK);
    tft.print(F("FAULT -- reset board to clear"));
  } else {
    tft.setTextColor(controlLoopRunning ? COLOR_GREEN : COLOR_GREY, COLOR_BLACK);
    tft.print(controlLoopRunning ? F("RUNNING") : F("STOPPED (duty locked at 0%)"));
  }
}

// WHAT: wasTouching remembers whether the screen was being touched on
//       the PREVIOUS poll, so checkTouch() can act only on the moment
//       a touch begins (the rising edge), not on every single poll
//       where a finger happens to still be resting on the screen.
// WHY:  Without this, holding a tap for longer than one 100ms poll
//       interval fires the button's action once per poll for as long
//       as contact lasts -- exactly what caused "START tapped" to
//       print 3 times and duty to climb well past where one tap
//       should have taken it, seen on the bench.
static bool wasTouching = false;

static void checkTouch() {
  bool isTouchingNow = (touchCtrl.read_td_status() != 0);

  if (!isTouchingNow) {
    wasTouching = false;   // Contact ended -- next touch will be treated as a fresh tap
    return;
  }

  if (wasTouching) {
    return;   // Still the same ongoing touch as last poll -- already acted on this one, ignore
  }
  wasTouching = true;   // This is a NEW touch starting right now -- act on it, just this once

  // WHAT: x = SCREEN_W - rawY, y = rawX -- a 90-degree-style axis
  //       swap, not a simple mirror.
  // WHY:  Two earlier guesses (no transform, then mirroring both axes)
  //       were both wrong. This transform was derived from two real
  //       bench measurements -- an intended START tap at raw(178,394)
  //       and an intended STOP tap at raw(146,144) -- and verified to
  //       land both correctly in their respective button zones. Makes
  //       physical sense too: the touch controller appears to report
  //       coordinates in the panel's native portrait orientation,
  //       independent of the TFT driver's landscape rotation(3)
  //       setting, which is exactly the kind of mismatch that produces
  //       a 90-degree axis swap rather than a plain mirror.
  uint16_t rawX = touchCtrl.read_touch1_x();
  uint16_t rawY = touchCtrl.read_touch1_y();
  uint16_t x = SCREEN_W - rawY;
  uint16_t y = rawX;

  if (touchIsInsideButton(x, y, START_BUTTON)) {
    if (faulted) {
      Serial.println(F("START ignored -- board is latched in FAULT. Reset to clear before starting again."));
      return;
    }
    controlLoopRunning = true;
    Serial.println(F(">>> START tapped -- control loop running. <<<"));
  } else if (touchIsInsideButton(x, y, STOP_BUTTON)) {
    controlLoopRunning = false;
    currentDutyPercent = 0.0;
    setDuty(0.0);   // Applied immediately, not waiting for the next control tick -- "stop means stop right now"
    Serial.println(F(">>> STOP tapped -- duty forced to 0%, control loop halted. <<<"));
  }
}

// ---------------------------------------------------------------------
// BLOCK 6.5: Serial start/stop -- a fallback control path in case the
// display/touch hardware doesn't work. Does the exact same thing as
// tapping the on-screen buttons (checkTouch() above), just triggered
// by typed text instead -- both paths share the same state variables
// (controlLoopRunning, currentDutyPercent, faulted), so using one
// doesn't conflict with the other; whichever actually works still
// controls the same real PWM output.
// ---------------------------------------------------------------------
static void checkSerialCommand() {
  if (Serial.available() == 0) {
    return;
  }

  String input = Serial.readStringUntil('\n');
  input.trim();
  input.toLowerCase();

  if (input == "start") {
    if (faulted) {
      Serial.println(F("START ignored -- board is latched in FAULT. Reset to clear before starting again."));
      return;
    }
    controlLoopRunning = true;
    Serial.println(F(">>> START (Serial) -- control loop running. <<<"));
  } else if (input == "stop") {
    controlLoopRunning = false;
    currentDutyPercent = 0.0;
    setDuty(0.0);   // Same immediate "stop means stop right now" behavior as the touch STOP button
    Serial.println(F(">>> STOP (Serial) -- duty forced to 0%, control loop halted. <<<"));
  } else if (input.length() > 0) {
    Serial.println(F("Not recognized. Type START or STOP and press Enter."));
  }
}

static unsigned long lastTouchPoll = 0;
static unsigned long lastDisplayUpdate = 0;
static unsigned long lastHeartbeat = 0;
#define TOUCH_POLL_INTERVAL_MS   100
#define DISPLAY_UPDATE_INTERVAL_MS 500

void setup() {
  Serial.begin(9600);

  Wire.begin();   // Joins the I2C bus once -- shared by the INA226 and the touch controller, same as the real firmware's sensors_init()

  configureTim8Pwm();
  configureIna226();
  configureDisplay();
  drawStaticScreen();

  Serial.println(F("--- CurrentControl_Giga_Test start ---"));
  Serial.println(F("Duty cycle locked at 0% until START (screen tap OR typed Serial command) is given. STOP (either path) forces an immediate return to 0% at any time."));
  Serial.println(F("Serial fallback: type START or STOP and press Enter -- works the same as tapping the screen, in case the display/touch doesn't come up."));
  Serial.print(F("Independent hard overcurrent trip active: "));
  Serial.print(HARD_CURRENT_LIMIT_A);
  Serial.println(F(" A. Tripping latches a FAULT -- reset the board to clear it."));
}

// WHAT: Touch polling disabled for now (see DISABLE_TOUCH_POLLING below).
// WHY:  Setup() was completing and printing its full banner, then the
//       board was resetting again before the first heartbeat -- a
//       silent hang/reset in loop(), not a crash in setup(). The
//       strongest suspect is touchCtrl.read_td_status(): the touch
//       controller's RST/INT pins aren't actually wired to anything
//       (see the note in BLOCK 3), so if this specific module really
//       does need a hardware reset pulse to respond, an I2C call to it
//       can hang indefinitely and trigger a watchdog reset -- exactly
//       matching the symptom seen. Disabling touch polling isolates
//       whether that's really the cause: if heartbeats start showing
//       up reliably with this off, that confirms it. The Serial
//       fallback (START/STOP typed commands) still works fully with
//       touch disabled, so this doesn't block testing.
// UPDATED: the reset loop traced back to TOUCH_RST_PIN/TOUCH_INT_PIN
// being set to unconnected placeholder pins (4/5) while the real wires
// were actually on D6/D3 -- the code was never touching the pins that
// were really wired. Now that BLOCK 3's pin numbers match the real
// wiring, re-enabling touch to test again.
#define DISABLE_TOUCH_POLLING 0

void loop() {
  checkHardLimit();   // Runs every single pass, not gated by any interval -- this is the independent check, it doesn't wait its turn

  checkSerialCommand();   // Checked every pass -- typed commands shouldn't feel laggy, and this is the safety fallback

#if !DISABLE_TOUCH_POLLING
  if (millis() - lastTouchPoll >= TOUCH_POLL_INTERVAL_MS) {
    lastTouchPoll = millis();
    checkTouch();
  }
#endif

  if (controlLoopRunning && (millis() - lastControlTick >= CONTROL_INTERVAL_MS)) {
    lastControlTick = millis();
    runControlStep();
  }

  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
    lastDisplayUpdate = millis();
    drawLiveReadout(ina226.getCurrent_mA() / 1000.0);
  }

  // WHAT: Serial heartbeat, same once-a-second pattern as every other
  //       test sketch in this project (and the UNO version of this
  //       exact sketch).
  // WHY:  Without this, Serial only ever prints on start/stop/fault
  //       events -- if the display never comes up, there would be no
  //       ongoing visibility into measured current or duty cycle at
  //       all, which defeats the point of having a Serial fallback in
  //       the first place. This makes Serial a genuinely complete
  //       fallback, not just half of one.
  if (millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    Serial.print(F("heartbeat | measured: "));
    Serial.print(ina226.getCurrent_mA() / 1000.0, 3);
    Serial.print(F(" A | duty: "));
    Serial.print(currentDutyPercent, 1);
    Serial.print(F(" % | "));
    if (faulted) {
      Serial.println(F("FAULT"));
    } else {
      Serial.println(controlLoopRunning ? F("RUNNING") : F("STOPPED"));
    }
  }
}
