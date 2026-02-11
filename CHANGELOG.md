# Changelog

## [Unreleased]

Based on upstream MeshCore dev@bcb7a8067e56b602dd434865f4542890a4d35446.

### Features

- Disable advertising functionality during system initialization
- Implement hardware support for T114 sensor modules
- Enforce duty cycle limits using token bucket algorithm
- Enable CLI boosted gain settings for SX126X radio modules (LNA)
- Configure bridge mode to be disabled by default
- Change default configuration to use 433 MHz frequency band
- Disable all sensor features and interfaces except BME280, BMP280 and INA3221

### CLI commands

- `get radio.lna`: Gets the SX126X boosted gain status on/off
- `set radio.lna <on|off>`: Sets the SX126X boosted gain status on/off

### Notes

The T114 platform now features I2C sensor compatibility with the following pin configuration:
 1. VCC (3v3)
 2. GND
 3. GPIO8 (SCL)
 4. GPIO7 (SDA)