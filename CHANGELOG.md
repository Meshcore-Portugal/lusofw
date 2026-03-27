# Changelog

## [v0.0.7] - UNRELEASED

Based on MeshCore v1.14.1
main@467959cc3bfc884e5f3425caac89453a450151b6

### Features

- Set default `flood_advert_interval` to 24 hours (instead of disabled)
- Only schedule flood advert timers when `flood_advert_interval` is greater than 0

## [v0.0.6] - 06/03/2026

Based on MeshCore v1.14.0
dev@3fe2dd7f48733fe77da7549cd24ef28bf07e1e5a

### Features

- Disable advert interval by default (was 2 minutes)
- Disable flood advert interval by default (was 12 hours)
- Enable listen before talk with interference threshold of 14
- Refactor buildAdvertData to use prefs when no GPS support is enabled
- Set default loop detection preference to minimal sensitivity
- Add 0x01 to reserved identity hash prefixes

------

## [v0.0.5] - 04/03/2026

Based on upstream MeshCore dev@3e5522fcded70751c5a06ad1183b3eb1821397fd.

### Features

- Added master time synchronization feature using a network time master to sync radio clocks
- Consensus time sync is now optional and disabled by default (replaced by master time sync)
- Master time sync only accepts timestamps from a specific trusted identity
- Master time sync applies to packets with path length < 8 and timestamps after 2026
- Improved time consensus algorithm with trimmed mean approach for better outlier rejection
- Time sync now distinguishes initial sync (unlimited forward) vs maintenance sync (±60s limit)
- Extended time sync sample collection to accept adverts up to 8 hops (was 4)
- Flood advert filter now applies to all non-CHAT and non-NONE advert types (was REPEATER only)
- Increased time sync sample buffer from 8 to 16 samples
- Improved debug logging with human-readable DateTime formatting

### Build Configuration

- Added `ENABLE_MASTER_TIME_SYNC` build flag (enabled by default)
- Disabled `ENABLE_CONSENSUS_TIME_SYNC` by default (can be re-enabled if needed)

### Devcontainer

- Added Bun feature for development environment
- Changed USB mount from volume to device passthrough
- Added opencode CLI installation

------

## [v0.0.4] - 22/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Further logic improvement of the repeater flood adverts limiter,

------

## [v0.0.3] - 21/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Fix errors with the repeater flood adverts limiter,

------

## [v0.0.2] - 19/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Enable AHTx0 sensors
- Heltec v4 build error fix
- Consensus time sync over the network based on advert data
- Limit repeater flood advert packet forwarding using a probabilistic reduction
- Limit repeater flood adverts to the maintenance window between 02:00 to 06:00

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