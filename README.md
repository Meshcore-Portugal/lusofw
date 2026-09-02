## About MeshCore

MeshCore is a lightweight, portable C++ library that enables multi-hop packet routing for embedded projects using LoRa and other packet radios. It is designed for developers who want to create resilient, decentralized communication networks that work without the internet.

## About lusofw
Lusofw is the official MeshCore firmware distribution tailored specifically for the Portuguese community. This project aims to enhance the user experience with quality-of-life improvements while maintaining compatibility with the upstream MeshCore repository.

### Philosophy

This is not intended to be a hard fork of the upstream repository. Instead, lusofw serves as:

- A release channel with curated quality-of-life improvements
- A testing ground for features that will eventually be merged upstream
- A community-focused distribution that provides faster access to enhancements

## Key Features

lusofw layers Portugal-focused defaults and quality-of-life enhancements on top of MeshCore:

- **Automatic region assignment (AutoRegions)** — repeaters derive their geographic region (EU → country → district → NUTS2/CIMS) from their stored or GPS coordinates, with no manual setup.
- **Smart flood adverts** — deterministic, collision-resistant scheduling across a rolling 23-hour window; each node's slot is derived from its name and public key.
- **Hardware CAD listen-before-talk** — channel activity is sensed before every transmit, enabled by default to reduce collisions.
- **Network time synchronization** — radio clocks sync from a trusted network time source, with replay-protected updates.
- **Loop detection** — enabled by default at minimal sensitivity to prevent packet storms.
- **Version-aware defaults migration** — firmware upgrades apply curated defaults safely, tracked by a persisted version stamp.
- **Airtime duty-cycle enforcement** — a token bucket keeps transmissions within regulatory limits.
- **Environment sensors** — BME280, BMP280, INA3221, AHTx0 and SHTC3 supported out of the box.
- **RS232 & ESP-NOW bridges** — integrate the mesh with external serial or Wi-Fi systems.
- **Broad device support** — Heltec (including the T114 true-color UI), LilyGO, RAK, Seeed, M5Stack and more.

### Links

- Official Portuguese website: https://meshcore.pt
- Online flasher: https://flasher.meshcore.pt
- Original website: https://meshcore.io/
