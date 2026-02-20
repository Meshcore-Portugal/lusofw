## About MeshCore

MeshCore is a lightweight, portable C++ library that enables multi-hop packet routing for embedded projects using LoRa and other packet radios. It is designed for developers who want to create resilient, decentralized communication networks that work without the internet.

## About lusofw
Lusofw is the official MeshCore firmware distribution tailored specifically for the Portuguese community. This project aims to enhance the user experience with quality-of-life improvements while maintaining compatibility with the upstream MeshCore repository.

You might support this project by [buying me a coffee](https://buymeacoffee.com/kasvoton).

### Philosophy

This is not intended to be a hard fork of the upstream repository. Instead, lusofw serves as:

- A release channel with curated quality-of-life improvements
- A testing ground for features that will eventually be merged upstream
- A community-focused distribution that provides faster access to enhancements

## Key Features

- Change default configuration to use 433 MHz frequency band
- Configure bridge mode to be disabled by default
- Consensus time sync over the network based on advert data
- Disable advertising functionality during system initialization
- Disable all sensor features and interfaces except AHTx0, BME280, BMP280 and INA3221
- Enable CLI boosted gain settings for SX126X radio modules (LNA)
- Enforce duty cycle limits using token bucket algorithm
- Implement hardware support for T114 sensor modules
- Limit repeater flood advert packet forwarding using a probabilistic reduction
- Limit repeater flood adverts to the maintenance window between 02:00 to 07:00
- Neighbours older than 48h will be automatically removed.
- The #portugal region is added and set as flood by default.

### Links

- Official Portuguese website: https://meshcore.pt
- Online flasher: https://flasher.meshcore.pt
- Original website: https://meshcore.co.uk
