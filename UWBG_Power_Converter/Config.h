#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// CONFIG: shared hardware addresses and calibration constants
//
// WHAT: One place for hardware values that multiple modules need (sensors,
//       boost converter, charge controller, display), instead of repeating
//       magic numbers in every file.
//
// HOW:  Plain #define constants. No RAM cost, and any .cpp file that
//       #includes "Config.h" can see them.
//
// WHY:  If a sensor address or the shunt resistor value changes, we edit it
//       here once instead of hunting through every module that uses it.
// ---------------------------------------------------------------------------

#define INA226_SOLAR_ADDR   0x40    // Solar array output sensor, A0=A1=GND
#define INA226_SHUNT_OHMS   0.010   // CJMCU-226 board shunt resistor value
#define INA226_MAX_AMPS     2.0     // Expected major-path current, per block diagram (0-2A)

// ---------------------------------------------------------------------------
// BLOCK 6: battery/charge-output sensor address
//
// WHAT: I2C address of the second INA226, placed at the boost converter's
//       output (battery/charging side) instead of the solar array.
//
// HOW:  Each INA226 board's address is set by two solder-jumper pins (A0,
//       A1). The default (both tied to GND) is 0x40 -- already used by the
//       solar sensor -- so this second board needs a DIFFERENT address or
//       both sensors would collide on the I2C bus. Per the INA226 address
//       table, bridging the A0 jumper to VS (instead of GND) gives 0x41.
//       Verify this against your specific board's silkscreen/datasheet once
//       it's in hand -- the jumper labeling can vary slightly by supplier.
//
// WHY:  The CC/CV charging algorithm regulates the actual current/voltage
//       going INTO the battery, not the solar array's output -- those two
//       are different once the boost converter is stepping voltage/current
//       up between them. This second, independent measurement point is what
//       it controls against.
// ---------------------------------------------------------------------------
#define INA226_BATTERY_ADDR 0x41   // Battery/charge-output sensor, A0 jumper set to VS

// ---------------------------------------------------------------------------
// BLOCK 11: wind branch sensor address
//
// WHAT: I2C address of the third INA226, placed on the wind branch (after
//       the wind SB540 isolation diode, before the common DC bus) --
//       mirrors the solar branch's own isolation-diode/sensor arrangement.
//
// WHY:  Same reasoning as the battery sensor: this needs its own unique
//       address so it doesn't collide with the solar (0x40) or battery
//       (0x41) sensors on the shared I2C bus. Per the INA226 address table,
//       bridging the A0 jumper to SDA (instead of GND or VS) gives 0x42.
//       Verify against your specific board once it's in hand.
// ---------------------------------------------------------------------------
#define INA226_WIND_ADDR 0x42   // Wind branch sensor, A0 jumper set to SDA

// ---------------------------------------------------------------------------
// BLOCK 15: common DC bus sensor address
//
// WHAT: I2C address of the fourth INA226, placed on the common DC bus --
//       AFTER both isolation diodes combine the solar and wind branches,
//       right before the boost converter's actual input.
//
// WHY:  Same reasoning as the other added sensors: needs its own unique
//       address (0x40/0x41/0x42 already taken). Per the INA226 address
//       table, bridging the A0 jumper to SCL gives 0x43. Verify against
//       your specific board once it's in hand.
//
//       This sensor also feeds Efficiency.cpp as the boost converter's
//       input power.
// ---------------------------------------------------------------------------
#define INA226_COMMONBUS_ADDR 0x43   // Common DC bus sensor, A0 jumper set to SCL

// ---------------------------------------------------------------------------
// BLOCK 17: load sensor address -- PLACEHOLDER, no hardware location yet
//
// WHAT: I2C address reserved for a fifth INA226, intended to measure power
//       delivered to the "model loads" (home/EV/data-center demo loads per
//       the block diagram) downstream of the buck converter.
//
// HOW:  Per the INA226 address table, A1=VS / A0=GND gives 0x44 -- the next
//       unused combination after 0x40/0x41/0x42/0x43. Verify against your
//       specific board once wired.
//
// WHY THIS IS A PLACEHOLDER: the buck converter (12.8V battery -> 5V bus)
//       is still an undecided hardware choice in the block diagram --
//       switch, inductor, diode, and driver are all still blank. That means
//       there is currently no physical "buck converter output" node to
//       attach this sensor to. This address and the read function below
//       exist so the code pattern is ready -- wiring it up later, once the
//       buck converter exists, should be a quick repeat of the same steps
//       used for the other four sensors, not a new design exercise. Until
//       then, sensors_readLoad() will return whatever a disconnected/
//       nonexistent sensor returns (see sensors_isBatterySensorConnected()
//       for how to tell "not really there" apart from "reading zero") --
//       don't trust these numbers for anything yet.
// ---------------------------------------------------------------------------
#define INA226_LOAD_ADDR 0x44   // Load sensor (PLACEHOLDER), A1 jumper set to VS

// ---------------------------------------------------------------------------
// BLOCK 4: loop timing
//
// WHAT: How often (in milliseconds) we print a solar reading to Serial.
//
// WHY:  Named here instead of a bare "1000" in the .ino so it reads as
//       "print interval" wherever it's used, and so it's easy to speed up
//       later (e.g. for tighter debugging) without hunting through loop().
// ---------------------------------------------------------------------------
#define SENSOR_PRINT_INTERVAL_MS 1000

// ---------------------------------------------------------------------------
// BLOCK 5 (upgraded in Block 8): boost converter gate drive pin
//
// WHAT: The Mega pin that carries the PWM signal into the TC4427 gate
//       driver, which switches the GaN device.
//
// WHY:  Pin 11 is Timer1's OC1A output -- a 16-bit hardware timer. Block 5
//       used plain analogWrite() here as a placeholder; Block 8 reconfigures
//       Timer1's registers directly for true ~100kHz output on this SAME
//       pin, so nothing about the physical wiring changes between the two.
// ---------------------------------------------------------------------------
#define GATE_DRIVE_PIN 11

// ---------------------------------------------------------------------------
// BLOCK 8: real hardware-timer PWM frequency
//
// WHAT: The actual boost-converter switching frequency (per the block
//       diagram, ~100kHz) and the Timer1 TOP value that produces it.
//
// HOW:  In Fast PWM mode with no prescaling, Timer1 counts up from 0 to
//       PWM_TOP_VALUE at the Mega's 16MHz clock rate, then resets --that's
//       one PWM period. So: frequency = F_CPU / (TOP + 1).
//       Solving for TOP at our target frequency:
//         TOP = F_CPU / PWM_FREQUENCY_HZ - 1
//             = 16,000,000 / 100,000 - 1 = 159
//       F_CPU is defined automatically by the Arduino core (16000000 for
//       the Mega), so this is computed here rather than hardcoding "159",
//       in case the target frequency ever changes.
//
// WHY:  This TOP value also sets our duty-cycle resolution: OCR1A can be
//       any whole number from 0 to PWM_TOP_VALUE, so we get (TOP+1) = 160
//       possible duty-cycle steps (~0.625% each) -- coarser than the 256
//       steps analogWrite() gave us, because running a 16-bit timer at a
//       much higher frequency means fewer clock ticks fit in each period.
//       This is an inherent trade-off between switching frequency and duty
//       cycle resolution on this hardware, not a bug -- worth knowing when
//       interpreting the boost converter's behavior on the scope.
// ---------------------------------------------------------------------------
#define PWM_FREQUENCY_HZ 100000UL
#define PWM_TOP_VALUE    ((F_CPU / PWM_FREQUENCY_HZ) - 1)

// ---------------------------------------------------------------------------
// BLOCK 7: CC/CV charge control targets and tuning
//
// WHAT: The charging targets from the block diagram (1.0A constant current,
//       14.4V constant voltage cap), plus tuning values for how the control
//       loop steps the duty cycle toward them.
//
// HOW:  CHARGE_DUTY_STEP_PERCENT is how much the duty cycle changes on each
//       control tick -- small enough to avoid big current/voltage swings,
//       large enough to actually respond within a reasonable time.
//       CHARGE_MAX_DUTY_PERCENT is a safety ceiling well below 100%: a
//       boost converter approaching 100% duty cycle stops boosting
//       properly and can pull excessive input current, so we cap it here
//       independently of BoostConverter's own hard 0-100% clamp.
//       CONTROL_LOOP_INTERVAL_MS sets how often the control loop re-checks
//       sensors and adjusts duty cycle.
//
// WHY:  These are named constants (not magic numbers buried in
// ChargeController.cpp) so the charging behavior can be tuned/retuned in
//       one place, and so the 1.0A / 14.4V figures visibly match the
//       project spec from the block diagram rather than looking arbitrary.
// ---------------------------------------------------------------------------
#define CHARGE_TARGET_CURRENT_A   1.0    // CC target: 1.0 A into the battery
#define CHARGE_TARGET_VOLTAGE_V   14.4   // CV target: 14.4 V cap at the battery
#define CHARGE_DUTY_STEP_PERCENT  0.5    // Duty cycle change per control tick
#define CHARGE_MAX_DUTY_PERCENT   80.0   // Safety ceiling, well below 100%
#define CONTROL_LOOP_INTERVAL_MS  50     // Run the control loop 20 times/sec

// ---------------------------------------------------------------------------
// BLOCK 16: hard safety cutoff limits
//
// WHAT: Absolute voltage/current limits at the battery sensor that trigger
//       an immediate fault, completely independent of the CC/CV targets
//       above or which operating mode (manual or automatic) is active.
//
// HOW:  Modest headroom above the CC/CV targets: 15.0V vs. the 14.4V CV
//       cap, 1.5A vs. the 1.0A CC target. Enough margin that normal
//       operation (including the step-controller's small overshoot as it
//       approaches its targets) won't false-trip this, but tight enough to
//       catch a real malfunction quickly. Both limits sit well inside the
//       battery's own rated maximums (14.4V max charge voltage, 4A max
//       charge current per its datasheet), so tripping here protects the
//       battery even if the CC/CV logic itself is misbehaving.
//
// WHY:  The CC/CV targets and the open-circuit fault detector (Block 10)
//       only protect the AUTOMATIC mode's control loop. Manual PWM test
//       mode (Block 9) has no protection at all otherwise -- a person
//       typing an unsafe duty cycle value has nothing stopping them. This
//       hard limit check runs regardless of mode, specifically to close
//       that gap.
// ---------------------------------------------------------------------------
#define HARD_LIMIT_MAX_VOLTAGE_V 15.0
#define HARD_LIMIT_MAX_CURRENT_A 1.5

// ---------------------------------------------------------------------------
// BLOCK 10: fault detection timeout
//
// WHAT: How long (in milliseconds) duty cycle is allowed to sit pinned at
//       CHARGE_MAX_DUTY_PERCENT without reaching the CC target current
//       before ChargeController treats it as a fault and shuts the output
//       off.
//
// WHY:  A few seconds is enough time to rule out normal startup transients
//       (e.g. briefly ramping up from 0%) while still catching a genuine
//       open-circuit/no-load condition quickly, rather than leaving the
//       boost converter running near its duty-cycle ceiling indefinitely.
// ---------------------------------------------------------------------------
#define CHARGE_FAULT_TIMEOUT_MS   3000   // 3 seconds pinned at ceiling = fault

// ---------------------------------------------------------------------------
// BLOCK 9: manual PWM test mode switch
//
// WHAT: A compile-time switch between two ways of driving the boost
//       converter's duty cycle:
//         true  = MANUAL mode -- you type a duty-cycle percentage into the
//                 Serial Monitor and it's applied directly, ignoring
//                 sensors and the CC/CV algorithm entirely.
//         false = AUTOMATIC mode -- ChargeController's CC/CV loop drives
//                 duty cycle based on the battery sensor, as built in
//                 Block 7.
//
// HOW:  Since this is a plain #define (not a variable), the choice is made
//       at compile time -- flip it and re-upload to switch modes.
//
// WHY:  Lets us verify the ~100kHz PWM signal on the oscilloscope in
//       isolation -- confirming frequency and that duty cycle actually
//       changes -- WITHOUT needing the battery sensor wired up, and without
//       the CC/CV loop's tendency to ramp toward its safety ceiling when it
//       can never reach its current target (e.g. nothing connected to
//       charge). Set this back to false once ready to test the full closed
//       loop with real sensors.
// ---------------------------------------------------------------------------
#define MANUAL_PWM_TEST_MODE true

// ---------------------------------------------------------------------------
// BLOCK 12: display + touch pin assignments
//
// WHAT: Mega pin connections for the Hosyond 3.5" ST7796U display and its
//       FT6336U capacitive touch controller, per the module's own pin
//       function table (VCC, GND, LCD_CS, LCD_RST, LCD_RS, SDI/MOSI, SCK,
//       LED, SDO/MISO, CTP_SCL, CTP_RST, CTP_SDA, CTP_INT, SD_CS).
//
// HOW:  SDI(MOSI), SCK, and SDO(MISO) are NOT defined here -- on the Mega,
//       hardware SPI is fixed to pins 51, 52, and 50 respectively, so the
//       display library uses those automatically rather than us choosing
//       them. CTP_SDA/CTP_SCL aren't defined here either -- they're the
//       same physical I2C bus (pins 20/21) that Sensors.cpp already uses
//       for the INA226s, via the same Wire library. SD_CS isn't used at all
//       since this project doesn't use the display's microSD slot.
//       The remaining signals (LCD_CS, LCD_RST, LCD_RS, LED, CTP_RST,
//       CTP_INT) each need their own free digital pin, chosen here to avoid
//       colliding with GATE_DRIVE_PIN (11) or the hardware SPI/I2C pins.
//
// WHY:  Named here (not hardcoded in a Display.cpp file) for the same
//       reason as every other pin constant in this project -- one place to
//       check or change the wiring plan.
// ---------------------------------------------------------------------------
#define LCD_CS_PIN    10
#define LCD_RST_PIN   9
#define LCD_RS_PIN    8    // AKA "DC" (data/command select) in most display libraries
#define LCD_LED_PIN   7    // Backlight; PWM-capable pin in case dimming is added later
#define TOUCH_RST_PIN 6
#define TOUCH_INT_PIN 3    // Interrupt-capable pin on the Mega

// ---------------------------------------------------------------------------
// BLOCK 18: display refresh interval
//
// WHAT: How often (in milliseconds) the live screen redraws its values.
//
// WHY:  Redrawing every loop() pass would waste SPI time and make the
//       screen flicker. Half a second is fast enough to follow the
//       readings while leaving time for the control loop and sensor reads.
// ---------------------------------------------------------------------------
#define DISPLAY_UPDATE_INTERVAL_MS 500

// ---------------------------------------------------------------------------
// BLOCK 19: touch polling interval
//
// WHAT: How often (in milliseconds) the touch controller is checked for a tap
//        on the tab bar.
//
// WHY:  Checking every loop() pass would flood the I2C bus. 100 ms still
//       feels instant to a person tapping the screen.
// ---------------------------------------------------------------------------
#define DISPLAY_TOUCH_POLL_INTERVAL_MS 100

// ---------------------------------------------------------------------------
// BLOCK 14: efficiency calculation threshold
//
// WHAT: The minimum common DC bus input power (in watts) required before we
//       bother computing an efficiency percentage.
//
// WHY:  Efficiency = output power / input power. When input power is very
//       small (near 0W, e.g. at dawn/dusk or with no wind), sensor noise in
//       the denominator can produce wildly misleading percentages (even
//       >100% or huge swings) that don't reflect anything real about the
//       converter. Below this threshold we just report 0% instead of a
//       noisy, meaningless ratio.
// ---------------------------------------------------------------------------
#define EFFICIENCY_MIN_INPUT_POWER_W 0.1

#endif
