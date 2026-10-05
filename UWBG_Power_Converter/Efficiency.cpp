// ---------------------------------------------------------------------------
// EFFICIENCY MODULE (implementation)
//
// WHAT: Implements efficiency_calculate() as described in Efficiency.h.
//
// HOW:  Takes one reading from the common bus sensor (input) and the
//       battery sensor (output), then divides -- guarded against the
//       near-zero-input case described in Config.h's
//       EFFICIENCY_MIN_INPUT_POWER_W comment.
//
// WHY:  The common bus sensor sits right at the boost converter's actual
//       input, so this is a direct efficiency measurement.
// ---------------------------------------------------------------------------

#include "Efficiency.h"
#include "Sensors.h"
#include "Config.h"

EfficiencyReading efficiency_calculate() {
  PowerReading commonBus = sensors_readCommonBus();   // Snapshot of the boost converter's actual input
  PowerReading battery   = sensors_readBattery();      // Snapshot of boost output

  EfficiencyReading result;
  result.inputPower_W  = commonBus.power_W;   // Power actually entering the boost converter
  result.outputPower_W = battery.power_W;     // Power actually reaching the battery

  if (result.inputPower_W < EFFICIENCY_MIN_INPUT_POWER_W) {
    result.efficiencyPercent = 0.0;   // Input too small to divide by meaningfully -- report 0 instead of noise
  } else {
    result.efficiencyPercent = (result.outputPower_W / result.inputPower_W) * 100.0;   // Standard efficiency ratio
  }

  return result;   // Hand all three numbers back together
}
