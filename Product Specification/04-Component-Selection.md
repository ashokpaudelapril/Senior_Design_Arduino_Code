# 4. Component Selection

*For each key component: what it is, key specs, and WHY it was chosen (over alternatives considered, if any).*

## 4.1 Switching Device

**Part:** Renesas TP65H150G4PS SuperGaN
*Key specs, why GaN over Si/SiC, why this specific part given prototyping constraints.*

## 4.2 Boost Inductor

**Part:** *fill in*
*Inductance, saturation current, DCR — why these values fit the design.*

## 4.3 Boost Diode

**Part:** 1N5822 Schottky
*Why Schottky, why this current/voltage rating.*

## 4.4 Gate Driver

**Part:** Microchip TC4427AEPA
*Why this driver, drive voltage/current requirements from the GaN switch.*

## 4.5 Bulk/Bypass Capacitors

*Input/output capacitor selection, ripple current requirements.*

## 4.6 Sensing

**Part:** INA226 (CJMCU-226 modules)
*Why this sensor, shunt resistor choice, addressing scheme.*

## 4.7 Controller

**Part:** Arduino Mega 2560
*Why this MCU (I/O count, timer availability, etc.).*

## 4.8 Battery

**Part:** KBT LiFePO4, 12.8V 5Ah
*Why LiFePO4, charge profile implications for CC/CV design.*
