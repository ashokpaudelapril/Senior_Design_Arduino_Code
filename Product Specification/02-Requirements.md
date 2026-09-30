# 2. Requirements

## 2.1 Functional Requirements

*What the subsystem must DO. Example table below — replace with your own IDs/wording.*

| ID | Requirement | Rationale |
|----|-------------|-----------|
| FR-1 | The system shall boost the combined solar/wind DC input to charge a 12.8V LiFePO4 battery. | |
| FR-2 | The system shall regulate charging current to a constant 1.0A while battery voltage is below the CV threshold. | |
| FR-3 | The system shall cap charging voltage at 14.4V once reached, tapering current. | |
| FR-4 | The system shall isolate the solar and wind branches from each other and from reverse current. | |
| FR-5 | The system shall report real-time voltage/current/power at monitored points. | |

## 2.2 Performance Requirements

| ID | Requirement | Target Value |
|----|-------------|---------------|
| PR-1 | Switching frequency | ~100 kHz |
| PR-2 | CC charging current | 1.0 A |
| PR-3 | CV charging voltage | 14.4 V |
| PR-4 | Input voltage range | *fill in, based on measured panel output* |
| PR-5 | Converter efficiency target | *fill in* |

## 2.3 Safety Requirements

*e.g. duty cycle ceilings, fault detection for open-circuit/no-load, overcurrent/overvoltage protection, thermal limits.*

## 2.4 Constraints

*Budget, component availability, timeline, course deliverable deadlines, etc.*
