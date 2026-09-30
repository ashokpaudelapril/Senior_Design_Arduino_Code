# 5. Firmware Design

## 5.1 Module Overview

*Describe the modular structure of the UWBG_Power_Converter sketch: Config, Sensors, BoostConverter, ChargeController (and Display, once built). One or two sentences per module on its responsibility.*

## 5.2 Sensing

*How sensor data is acquired (INA226 over I2C), calibration approach (shunt resistor, current range), and units used throughout the firmware.*

## 5.3 PWM Generation

*Timer1 hardware PWM configuration: frequency, resolution, why this timer/pin was chosen.*

## 5.4 CC/CV Control Algorithm

*Describe the control strategy (e.g. incremental step control), the decision logic between CC and CV phases, tuning parameters (step size, control loop rate), and known limitations (e.g. no proportional/derivative term, behavior under open-circuit/no-load).*

## 5.5 Timing Architecture

*Non-blocking millis()-based scheduling — why delay() was avoided, how multiple tasks (control loop, sensor printing, eventually display) run at independent rates.*

## 5.6 Safety Handling

*Duty cycle clamping, safety ceiling below 100%, fault conditions considered (open circuit, sensor failure, overcurrent/overvoltage) and how each is or will be handled.*

## 5.7 Known Limitations / Future Work

*Anything intentionally simplified for this stage — e.g. simple step controller instead of PID, placeholder buck converter, limited sensor coverage — and what would need to change to mature it.*
