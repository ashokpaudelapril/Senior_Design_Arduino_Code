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
// BLOCK 5 (upgraded in Block 8, ported to Giga in Block 21): boost converter
// gate drive pin
//
// WHAT: The pin that carries the PWM signal into the TC4427 gate driver,
//       which switches the GaN device.
//
// WHY:  Pin 11 was originally chosen on the Mega because it's Timer1's OC1A
//       output -- a 16-bit hardware timer, needed to generate true ~100kHz
//       PWM. On the Giga, pin 11 (D11) happens to map to TIM8 Channel 2,
//       also independently suitable for this -- confirmed by direct scope
//       measurement in Block 21. Nothing about the physical wiring needed
//       to change when the board did.
// ---------------------------------------------------------------------------
#define GATE_DRIVE_PIN 11

// ---------------------------------------------------------------------------
// BLOCK 21: Giga TIM8 hardware-timer PWM (replaces the Mega's Timer1 code)
//
// WHAT: The boost-converter switching frequency (~100kHz, per the block
//       diagram) and the timer tick values that produce it on the Giga's
//       STM32H7 chip, via TIM8 Channel 2.
//
// HOW:  Unlike the Mega's AVR Timer1 (a simple 16MHz clock with a
//       straightforward TOP calculation), the Giga's TIM8 clock speed isn't
//       something to assume -- STM32 timers get a 2x multiplier over their
//       APB bus clock whenever that bus's prescaler divides by more than 1.
//       TIM8_CLOCK_HZ below is NOT calculated from a datasheet number; it
//       was measured directly on real hardware using a small diagnostic
//       sketch (HAL_RCC_GetPCLK2Freq() read 120MHz, and HAL_RCC_GetClockConfig()
//       confirmed the APB2 prescaler divides by more than 1, so the x2 rule
//       applies: 120MHz x 2 = 240MHz actual TIM8 kernel clock).
//
//       With no prescaling, the timer counts 0..PWM_PERIOD_TICKS-1 and
//       wraps -- that's one PWM period:
//         PWM_PERIOD_TICKS = TIM8_CLOCK_HZ / PWM_FREQUENCY_HZ
//                           = 240,000,000 / 100,000 = 2400
//
// WHY:  This was NOT the first thing tried. Mbed's standard PwmOut API
//       (both write() and pulsewidth()) was tested first, since it's the
//       normal Arduino-level way to do PWM -- but scope measurement showed
//       it only produces ~10% duty steps (values like 25-29% all rounded
//       down to 20%), regardless of which float-based entry point was
//       used. That meant the limitation was inside Mbed's own PWM
//       implementation for this timer, not fixable from the sketch side
//       with the standard API. Configuring TIM8 directly via STM32 HAL
//       (done in BoostConverter.cpp) was confirmed by scope measurement to
//       give correct duty cycles at values like 35%, 55%, and 58% -- a
//       resolution of 1/2400 = ~0.042% per step, far finer than the
//       0.5% step size ChargeController actually needs.
// ---------------------------------------------------------------------------
#define PWM_FREQUENCY_HZ   100000UL
#define TIM8_CLOCK_HZ      240000000UL                         // Measured, not assumed -- see HOW above
#define PWM_PERIOD_TICKS   (TIM8_CLOCK_HZ / PWM_FREQUENCY_HZ)  // 2400 ticks per period
#define PWM_ARR_VALUE      (PWM_PERIOD_TICKS - 1)              // Timer counts 0..ARR, so ARR = period - 1

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
// BLOCK 22: temperature sensor pin
//
// WHAT: The Giga pin carrying the 1-Wire bus for the two DS18B20
//       temperature probes (GaN switch and boost inductor).
//
// HOW:  DS18B20 uses 1-Wire, not I2C -- unlike the INA226 sensors, both
//       probes share this ONE pin (plus GND and 3.3V), distinguished by
//       their own unique factory-burned IDs rather than a configurable
//       address. A single pull-up resistor (4.7k ohm, included with the
//       DROK sensor kit) goes between this pin and 3.3V -- NOT 5V, since
//       the Giga's logic is 3.3V and pulling the line to 5V would
//       overvoltage the GPIO when a sensor releases it high.
//
// WHY:  Pin 2 was chosen because it's free -- not used by any other module
//       in this project (gate drive is 11; display/touch use 3, 6, 7, 8,
//       9, 10; I2C is 20/21; SPI is the Giga's fixed hardware pins).
// ---------------------------------------------------------------------------
#define TEMP_SENSOR_PIN 2

// ---------------------------------------------------------------------------
// BLOCK 23: thermal safety limits
//
// WHAT: Maximum safe temperatures (Celsius) for the GaN switch and boost
//       inductor before ChargeController trips a fault, same pattern as
//       the voltage/current hard limits above.
//
// HOW:  These are conservative starting values, NOT taken from a specific
//       datasheet thermal-derating calculation -- the GaN switch
//       (TP65H150G4PS) and inductor (TDK SPM10065VT) both have junction/
//       core ratings well above 85C, but we're measuring board/case
//       temperature near each part with an external probe, not the actual
//       junction or core temperature, so a lower trip point gives real
//       margin against thermal damage. TUNE THESE once you have real bench
//       data on how hot each part actually runs at normal operating duty
//       cycles -- the same spirit as CHARGE_DUTY_STEP_PERCENT being an
//       untested starting guess until proven on hardware.
//
// WHY:  This extends chargeController_checkHardLimits() (Block 16) to also
//       cover thermal faults, using the exact same fault-latch mechanism
//       and the same "runs in both manual and automatic mode" reasoning --
//       a person in manual test mode has no other protection against
//       driving the GaN switch into an overtemperature condition either.
// ---------------------------------------------------------------------------
#define HARD_LIMIT_MAX_GAN_TEMP_C       85.0
#define HARD_LIMIT_MAX_INDUCTOR_TEMP_C  85.0

// ---------------------------------------------------------------------------
// BLOCK 24: temperature sensor timing
//
// WHAT: Timing constants for the DS18B20 non-blocking read cycle.
//
// HOW:  TEMP_REQUEST_INTERVAL_MS is how often a new conversion is started.
//       TEMP_CONVERSION_TIME_MS is how long a DS18B20 needs to finish one
//       conversion at its default 12-bit resolution -- 750ms is the
//       datasheet's worst-case figure; waiting slightly longer (800ms)
//       leaves a small safety margin.
//
// WHY:  Temperature changes slowly compared to electrical faults (a GaN
//       switch doesn't go from cool to dangerous in milliseconds the way
//       voltage can), so reading once a second is more than fast enough --
//       no need to push these sensors as hard as the 50ms CC/CV control
//       loop runs.
// ---------------------------------------------------------------------------
#define TEMP_REQUEST_INTERVAL_MS  1000
#define TEMP_CONVERSION_TIME_MS   800

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
// WHAT: Pin connections for the Hosyond 3.5" ST7796U display and its
//       FT6336U capacitive touch controller, per the module's own pin
//       function table (VCC, GND, LCD_CS, LCD_RST, LCD_RS, SDI/MOSI, SCK,
//       LED, SDO/MISO, CTP_SCL, CTP_RST, CTP_SDA, CTP_INT, SD_CS).
//
// HOW:  SDI(MOSI), SCK, and SDO(MISO) are NOT defined here -- the display
//       library uses the board's default SPI bus automatically. On the Mega
//       that bus is pins 51, 52, and 50. On the Giga R1 WiFi it is the main
//       SPI header (CIPO/COPI/SCK at D89/D90/D91), so the display's MOSI,
//       SCK, and MISO wires move to that header when switching boards.
//       CTP_SDA/CTP_SCL aren't defined here either -- they're the same I2C
//       bus (pins 20/21 on both boards) that Sensors.cpp uses for the
//       INA226s, via the same Wire library. SD_CS isn't used at all since
//       this project doesn't use the display's microSD slot.
//       The remaining signals (LCD_CS, LCD_RST, LCD_RS, LED, CTP_RST,
//       CTP_INT) each need their own free digital pin, chosen here to avoid
//       colliding with GATE_DRIVE_PIN (11) or the SPI/I2C pins. On the Giga,
//       pins 8 and 9 are the second I2C bus, which this project doesn't use.
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
#define TOUCH_INT_PIN 3    // Interrupt-capable pin

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

// ---------------------------------------------------------------------------
// BLOCK 25: relay control pins (solar / wind / load switching)
//
// WHAT: Six digital output pins, each driving one channel of the ELEGOO
//       8-channel relay module -- these replace what was originally planned
//       as 6 manually-flipped toggle switches. Instead of a person flipping
//       a physical switch, Arduino drives these pins HIGH/LOW, and the
//       relay module's optocoupler + transistor on that channel does the
//       actual switching of the real circuit.
//
// HOW:  Each pin feeds one relay module channel's logic input (the "VCC"
//       side of the module, NOT "JD-VCC" -- the module's two power rails
//       are split so the Giga's 3.3V GPIO only ever talks to the
//       optocoupler, never the 5V relay coil current itself; see
//       01-Wiring-and-Assembly.md for the full jumper-removal wiring note).
//       Picked pins 4, 5, 12, 22, 23, 24 -- plain digital pins, no PWM or
//       hardware-timer requirement, and none of them collide with anything
//       already in use: gate drive is 11, temp probes are 2, I2C is 20/21,
//       display/touch use 3, 6, 7, 8, 9, 10, and the display's SPI uses the
//       Giga's fixed main-header pins (D89-D91).
//
// WHY:  Named here, one per switching location, for the same reason as
//       every other pin constant in this file -- one place to check or
//       change the wiring plan. The grouping (solar / wind / 4 loads)
//       matches the final switch-location decision: the original 14-switch
//       manual-toggle plan (wind drive motor + 4 individual generators +
//       8 other disconnect points) was replaced by just these 6 Arduino-
//       controlled points, with everything else (battery disconnect, master
//       power, etc.) staying a manual physical switch since those are
//       either bench-test disconnects or things the charge controller needs
//       to stay connected to function.
// ---------------------------------------------------------------------------
#define RELAY_SOLAR_PIN              4    // Solar array -> common bus
#define RELAY_WIND_PIN               5    // Whole wind subsystem (drive motor + generators) -> common bus
#define RELAY_LOAD_DATACENTER_PIN    12   // Data center demo load
#define RELAY_LOAD_EV_PIN            22   // EV charging demo load
#define RELAY_LOAD_LED_LAMP_PIN      23   // LED lamp demo load
#define RELAY_LOAD_OTHER_PIN         24   // Other / general demonstration load

// ---------------------------------------------------------------------------
// BLOCK 26: relay logic sense
//
// WHAT: Whether this specific relay module energizes its relay (closes the
//       real-world circuit) on a HIGH signal or a LOW signal.
//
// HOW:  Most of these common "blue board" opto-isolated relay modules are
//       ACTIVE-LOW by default -- pulling the channel's input LOW turns the
//       relay ON, HIGH turns it OFF -- which is the opposite of what most
//       people assume at a glance. CONFIRM THIS ON THE BENCH once the
//       module arrives (listen/watch for the relay's click while toggling
//       the pin both ways) before trusting it -- don't just assume the
//       datasheet default matches your specific board revision.
//
// WHY:  Keeping this as one named constant, used everywhere a relay is
//       switched, means if the bench test shows the opposite sense, fixing
//       it is a one-line change here instead of hunting through every
//       relay-driving call to flip its logic individually.
// ---------------------------------------------------------------------------
#define RELAY_ACTIVE_LOW true   // true = LOW turns the relay ON; verify on the bench

// ---------------------------------------------------------------------------
// BLOCK 27: load-shedding power threshold
//
// WHAT: The minimum battery/charge-output power (in watts) required before
//       any of the 4 demo load relays (data center, EV, LED lamp, other)
//       is allowed to actually energize, even if its touch button says ON.
//
// HOW:  Checked against sensors_readBattery().power_W -- the real, already-
//       wired sensor that represents power actually being delivered by the
//       solar+wind+boost-converter chain right now. This is a placeholder
//       choice, same spirit as CHARGE_DUTY_STEP_PERCENT and the thermal
//       limits above: a reasonable starting number, not a datasheet or
//       spec-derived figure, chosen to sit clearly above "just a trickle
//       from one source" and clearly below "both sources contributing" so
//       the demo (e.g. blocking the solar panel) visibly changes which
//       loads stay enabled. TUNE once you have real bench numbers for what
//       solar-only vs. solar+wind power actually looks like.
//
// WHY:  This is the "enough power available" rule for LoadControl.cpp's
//       load-shedding check -- the automatic check that has final say over
//       a load relay even when the operator has tapped it ON, same layered
//       pattern as the existing hard safety limits overriding manual duty
//       cycle input.
// ---------------------------------------------------------------------------
#define LOAD_ENABLE_MIN_BATTERY_POWER_W 3.0

// ---------------------------------------------------------------------------
// BLOCK 29: buck converter -- gate drive pin and switching frequency
//
// WHAT: The pin carrying the PWM signal into the buck stage's gate driver,
//       and the switching frequency -- taken directly from the validated
//       LTspice design (12V in -> ~4.7-4.8V out, EPC2204 switch, 47uH
//       inductor, 220uF output cap), not estimated.
//
// HOW:  Uses STM32's TIM1 Channel 2 (pin D1), NOT the boost converter's
//       TIM8 Channel 2 (pin 11) -- a genuinely separate hardware timer,
//       not just a different channel of the same one. Sharing TIM8
//       between two independent converter modules would mean both
//       BoostConverter.cpp and BuckConverter.cpp touching the same
//       underlying timer peripheral's init/config, risking one module's
//       code stepping on the other's channel. A separate timer keeps
//       them fully independent, same as every other module boundary in
//       this firmware.
//
// WHY THIS PIN/TIMER NEEDS BENCH VERIFICATION, NOT JUST TRUST: D1's TIM1
//       CH2 mapping comes from a third-party reference (OpenMV's Giga
//       pin docs), not Arduino's own pinout page, and that source's own
//       tables didn't fully agree with each other in places. This is
//       exactly the "looks right on paper" situation that burned us on
//       the boost converter's PWM resolution (Mbed's PwmOut looked fine
//       in the API docs too) -- confirm this pin actually drives TIM1 CH2
//       correctly with a standalone test sketch (same approach as
//       PWM_Giga_Test before BoostConverter.cpp was trusted) before this
//       goes anywhere near the real gate driver.
//
//       100kHz matches the frequency actually used in the LTspice sim
//       (PULSE period = 10us), and conveniently matches the boost
//       converter's frequency too, though that's not required -- it's
//       just what the validated design used.
// ---------------------------------------------------------------------------
#define BUCK_GATE_DRIVE_PIN   1
#define BUCK_PWM_FREQUENCY_HZ 100000UL

// ---------------------------------------------------------------------------
// BLOCK 30: buck converter -- CV/CC regulation targets
//
// WHAT: The buck stage's control targets -- NOT a symmetric CC-then-CV
//       charge profile like the boost converter's battery charging.
//       This is CV-with-a-ceiling: BuckController.cpp holds 5.0V as the
//       normal, moment-to-moment target (whatever current that takes,
//       determined entirely by what's plugged into the load bus), and
//       only backs off voltage to cap current at BUCK_CC_CEILING_A if
//       combined load demand tries to exceed it.
//
// HOW:  BUCK_TARGET_VOLTAGE_V = 5.0 matches the block diagram's 12.8V->5V
//       load bus (the LTspice sim's ~4.7-4.8V settled output reflects
//       diode/inductor losses at that specific operating point -- 5.0V is
//       the real target, not the lossy sim result).
//
//       BUCK_CC_CEILING_A = 2.0 is a calculated total, not a guess: EV
//       charger (~1A via TP4056) + data center (~0.25A) + LED lamp
//       (~0.15A) + other (~0.2A) = 1.6A, rounded up to 2.0A for margin --
//       same ~50% headroom pattern as the boost converter's hard limits
//       (1.0A CC target -> 1.5A hard limit). TUNE the three placeholder
//       load estimates once real devices are chosen for data center/LED
//       lamp/other.
//
// WHY THIS MEANS A COMPONENT CHANGE, NOT JUST A FIRMWARE NUMBER: the
//       1N5819 diode in the current LTspice sim is only rated ~1A --
//       undersized for a 2.0A ceiling. Swap it for an SB540 (5A/40V,
//       already used elsewhere in this project for the solar/wind
//       isolation diodes) before trusting this on real hardware. This
//       constant describes what the FIRMWARE should target; it doesn't
//       by itself make the current hardware capable of delivering it.
// ---------------------------------------------------------------------------
#define BUCK_TARGET_VOLTAGE_V 5.0
#define BUCK_CC_CEILING_A     2.0

// ---------------------------------------------------------------------------
// BLOCK 31: WiFi dashboard -- network credentials and server port
//
// WHAT: The WiFi network the Giga joins, and the port its read-only status
//       dashboard listens on.
//
// HOW:  Fill in WIFI_SSID/WIFI_PASSWORD with your actual network before
//       uploading -- same as WiFi_Giga_Test.ino, which is where this
//       library (WiFi.h) and this exact connection approach were first
//       verified in isolation before being ported into the real firmware
//       here. WEB_DASHBOARD_PORT 80 is the default HTTP port, so the
//       dashboard is reachable by just typing the Giga's IP address into
//       a browser with nothing extra appended.
//
// WHY:  This is a local-network-only feature (same room/lab during a
//       demo), so a hardcoded SSID/password is an accepted simplification
//       here, not an oversight -- see 01-Wiring-and-Assembly.md for the
//       same reasoning already applied to the relay module's wiring.
//       Don't commit your real WiFi password if this project's repo is
//       ever made public.
//
// THIS IS STILL UNVERIFIED ON REAL HARDWARE: the WiFi.h library's API
//       (WiFi.begin(), WiFiServer, WiFiClient) is assumed correct based on
//       Arduino's own documentation and the standalone WiFi_Giga_Test.ino
//       sketch, but that sketch itself has not yet been confirmed working
//       on the actual board (hardware wasn't available at the time it was
//       written). Treat this module the same way BoostConverter.cpp's
//       TIM8 code was treated before its own oscilloscope confirmation --
//       plausible, not yet proven.
// ---------------------------------------------------------------------------
#define WIFI_SSID          "YOUR_WIFI_NETWORK_NAME_HERE"
#define WIFI_PASSWORD      "YOUR_WIFI_PASSWORD_HERE"
#define WEB_DASHBOARD_PORT 80

#endif
