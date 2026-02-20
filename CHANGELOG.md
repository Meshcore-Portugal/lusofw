# Changelog

## [v0.0.2] - 19/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Enable AHTx0 sensors
- Heltec v4 build error fix
- Consensus time sync over the network based on advert data
- Limit repeater flood advert packet forwarding using a probabilistic reduction
- Limit repeater flood adverts to the maintenance window between 02:00 to 07:00

### CLI commands

- `get flood.advert.base`
- `set flood.advert.base <0-1>`: defaults to 0.308f

------

## [v0.0.1] - 13/02/2026

Based on upstream MeshCore dev@3f33455b4d96426b2f8b462b48ff1d4e31de1bf8.

### Features

- Change default configuration to use 433 MHz frequency band
- Configure bridge mode to be disabled by default
- Disable advertising functionality during system initialization
- Disable all sensor features and interfaces except BME280, BMP280 and INA3221
- Enable CLI boosted gain settings for SX126X radio modules (LNA)
- Enforce duty cycle limits using token bucket algorithm
- Implement hardware support for T114 sensor modules
- Neighbours older than 48h will be automatically removed.
- The #portugal region is added and set as flood by default.

### CLI commands

- `get radio.lna`: Gets the SX126X boosted gain status on/off
- `set radio.lna <on|off>`: Sets the SX126X boosted gain status on/off

### Notes

The T114 platform now features I2C sensor compatibility with the following pin configuration:
 1. VCC (3v3)
 2. GND
 3. GPIO8 (SCL)
 4. GPIO7 (SDA)