
# PortGuard – Predictive USB Charging Health Analyzer

## Project Overview
PortGuard is an Arduino Nano-based embedded systems mini-project that monitors voltage and current in a test circuit. It identifies programmed conditions and provides visual and audible alerts.

## Objectives
- Monitor voltage and current using the INA219 sensor.
- Identify seven programmed conditions.
- Display readings and status on an LCD.
- Provide alerts using LEDs and a buzzer.

## Components Used
- Arduino Nano
- INA219 voltage and current sensor
- 16×2 I2C LCD
- Green LED and red LED
- HW-508 buzzer
- 220-ohm resistor and LED test load
- Push button
- Breadboard and jumper wires
- USB cable

## Working Principle
The INA219 measures voltage and current in the connected test-load circuit and sends the readings to the Arduino Nano. The Arduino processes the readings using programmed thresholds. The LCD displays the readings and detected condition, while the LEDs and buzzer provide status indications.

## Operating Modes

### Auto Mode
Uses actual sensor readings from the connected test circuit.

### Demo Mode
Uses simulated conditions to demonstrate the programmed responses.

## Conditions Monitored
1. Stable
2. Unstable
3. Low Voltage
4. Low Current
5. High Voltage
6. High Current
7. Interrupted

## Connections
- INA219 SDA → Arduino Nano A4
- INA219 SCL → Arduino Nano A5

## Output
The LCD displays readings and the detected condition. The green LED indicates a stable condition, while the red LED indicates warning conditions. The buzzer provides audible alerts for selected warnings.

## Limitations
The current prototype uses an LED and a 220-ohm resistor as a test load. It has not been tested with an actual mobile phone charging setup. The displayed health score is a programmed indicator, not a measurement of actual phone battery health.

## Future Improvements
- Integrate a suitable USB measurement path.
- Test with appropriate charging sources and loads.
- Add data logging and further analysis.

## Technologies Used
- Embedded C / Arduino programming
- Arduino Nano
- INA219 sensor
- I2C communication

