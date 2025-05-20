# Solar Module Test

A monitoring system for solar modules using the SparkFun RedBoard Artemis Nano platform.

## Hardware Components
- SparkFun RedBoard Artemis Nano
- BH1750 Light Sensor (I2C address 0x23)
- INA228 Current/Voltage Sensors:
  - Solar Panel Monitor (0x40)
  - Battery Power Monitor (0x42)
  - Load Power Monitor (0x41)

## Features
- Real-time monitoring of light intensity (lux)
- Voltage, current, and power measurements for solar panel, battery, and load
- Serial output of all sensor readings

## Requirements
- PlatformIO or Arduino IDE
- Adafruit_INA228 library (modified for direct I2C communication)
- hp_BH1750 library

## Wiring
- BH1750 connected to I2C (SDA/SCL)
- INA228 sensors connected to I2C with appropriate addresses (0x40, 0x41, 0x42)

## Usage
1. Upload the code to your SparkFun RedBoard Artemis Nano
2. Open Serial Monitor at 115200 baud
3. View real-time sensor readings updated every 3 seconds

## License
MIT 