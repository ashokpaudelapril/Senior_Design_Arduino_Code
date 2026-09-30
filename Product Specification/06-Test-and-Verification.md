# 6. Test and Verification

*For each requirement in 02-Requirements.md, describe how it will be verified.*

| Requirement ID | Test Method | Equipment | Pass Criteria | Status |
|----------------|-------------|-----------|----------------|--------|
| PR-1 (switching frequency) | Oscilloscope on gate-drive pin | Oscilloscope | ~100kHz ±tolerance | *Not started / In progress / Passed* |
| PR-2 (CC current) | Measure charging current into battery/load under varying input | Multimeter / INA226 logging | Holds at 1.0A ±tolerance while below CV threshold | |
| PR-3 (CV voltage) | Measure battery voltage as charge progresses | Multimeter / INA226 logging | Caps at 14.4V, current tapers | |
| FR-4 (isolation) | Verify no reverse current flows between branches | Multimeter (current mode) | No measurable reverse current | |
| ... | | | | |

## 6.1 Test Log

*Chronological notes from actual test sessions — date, what was tested, results, issues found.*
