#ifndef EFFICIENCY_H
#define EFFICIENCY_H

// ---------------------------------------------------------------------------
// EFFICIENCY MODULE (public interface)
//
// WHAT: Computes the boost converter's power efficiency by comparing
//       common DC bus input power against battery/charge-output power.
//       Returns all three numbers together (input, output, efficiency%) so
//       callers don't have to re-derive them.
//
// HOW:  efficiency_calculate() reads the common bus and battery sensors
//       itself (via Sensors.h) and does the arithmetic -- callers just ask
//       for one EfficiencyReading, the same pattern as PowerReading from
//       the Sensors module.
//
// WHY:  This is a DERIVED metric built from existing sensor readings, not a
//       new sensor or a control decision -- it doesn't belong in
//       Sensors.cpp (which only knows about individual INA226 chips) or
//       ChargeController.cpp (which is about deciding duty cycle, not
//       reporting statistics). Giving it its own small module keeps each
//       piece of the firmware focused on one job.
//
// NOTE: Input power is read directly from the common DC bus sensor, which
//       sits after the SB540 isolation diodes, so the diodes' forward-voltage
//       drop is already included in the measurement.
// ---------------------------------------------------------------------------

struct EfficiencyReading {
  float inputPower_W;      // Common DC bus power -- the boost converter's actual input
  float outputPower_W;     // Battery/charge-output power (boost output)
  float efficiencyPercent; // outputPower_W / inputPower_W * 100, or 0 if input is negligible
};

EfficiencyReading efficiency_calculate();

#endif
