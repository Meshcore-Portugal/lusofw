## About MeshCore

MeshCore is a lightweight, portable C++ library that enables multi-hop packet routing for embedded projects using LoRa and other packet radios. It is designed for developers who want to create resilient, decentralized communication networks that work without the internet.

## About lusofw
Lusofw is the official MeshCore firmware distribution tailored specifically for the Portuguese community. This project aims to enhance the user experience with quality-of-life improvements while maintaining compatibility with the upstream MeshCore repository.

### Philosophy

This is not intended to be a hard fork of the upstream repository. Instead, lusofw is:

- A release channel with curated quality-of-life improvements
- A testing ground for features that will eventually be merged upstream
- A community-focused distribution that provides faster access to enhancements

## Differences from upstream

Based on MeshCore v1.18.0. Values are for the repeater unless the "Applies to" column says otherwise. Each link goes to the full explanation in [docs/lusofw.md](docs/lusofw.md).

### Changed defaults

| Setting | Upstream | lusofw | Applies to | When |
|---|---|---|---|---|
| [`advert.interval`](docs/lusofw.md#smart-adverts) | `1` (zero-hop advert every 2 min) | `0` (off) | repeater | every upgrade |
| [`flood.advert.interval`](docs/lusofw.md#smart-adverts) | `47` h, counted from boot | `23`: one fixed slot per node in a 23 h UTC window | repeater | every upgrade |
| [`loop.detect`](docs/lusofw.md#loop-detection) | `minimal` | `moderate` | repeater | every upgrade |
| [`path.hash.mode`](docs/lusofw.md#2-byte-path-hashes) | `0` (1 byte) | `1` (2 bytes) | repeater, companion | repeater: every upgrade. Companion: fresh install |
| [`int.thresh`](docs/lusofw.md#interference-threshold-auto-mode) | `0` (off) | `auto` (8 to 16 dB by SF) | repeater, companion | once |
| [`af` / duty cycle](docs/lusofw.md#radio-regulation) | `1.0` (50%) | `9.0` (10%) when located in Europe | repeater | until set by hand |
| [`tx` power](docs/lusofw.md#radio-regulation) | board default | capped at 27, 14 or 10 dBm by EU sub-band | repeater | until set by hand |
| [`txdelay` / `rxdelay` / `direct.txdelay`](docs/lusofw.md#repeater-tiers) | 0.5 / 0.0 / 0.3 | tier 3: 0.3 / 3.0 / 0.1 for nodes with no stored tier | repeater | once |
| [Node name](docs/lusofw.md#build-changes) | `repeater`, `Test BBS`, `sensor` | `lusofw-<version>` | repeater, room, sensor | build |
| [Advert on boot](docs/lusofw.md#build-changes) | on | off | repeater, room, sensor | build |
| [GPS](docs/lusofw.md#build-changes) | built into many variants | removed from most variants; kept on ThinkNode M4 and on the Heltec T1, Heltec T114, ThinkNode M8 and Muzi Base superIO companions | all | build |
| [Sensor drivers](docs/lusofw.md#build-changes) | 14 | AHTx0, BME280, BMP280, INA3221 (+SHTC3 on RAK) | all | build |

"Every upgrade" means the value is reset on each firmware version change and anything you set by hand is lost. "Once" means a one-shot migration; your later changes stick. See [defaults migration](docs/lusofw.md#defaults-migration).

Hardware listen-before-talk (`cad`) is off in both firmwares. The duty-cycle token bucket and the RS232 and ESP-NOW bridges come from upstream.

### Added behaviour

| Feature | What it does | Applies to |
|---|---|---|
| [Automatic regions](docs/lusofw.md#automatic-regions) | Creates `#europe`, `#pt`, district, macro region and `#pt-433`/`#pt-868` from coordinates or the name prefix | repeater |
| [Smart adverts](docs/lusofw.md#smart-adverts) | Flood advert at a slot hashed from name and key, ±3 s jitter | repeater |
| [Advert protection](docs/lusofw.md#advert-protection) | Relays each remote repeater's advert at most once per 12 h | repeater |
| [Mobile repeater filter](docs/lusofw.md#mobile-repeater-filter) | Does not relay flood adverts from repeater keys starting `0x01` | repeater |
| [Network time](docs/lusofw.md#network-time) | Sets the clock from a trusted, replay-protected timekeeper | repeater |
| [Repeater tiers](docs/lusofw.md#repeater-tiers) | `set tier 0-3` sets the three delay factors for the site | repeater |
| [TX queue expiry](docs/lusofw.md#core-packet-handling) | Drops packets queued longer than 60 s | all |
| [CAD retry jitter](docs/lusofw.md#core-packet-handling) | Busy-channel retry is random 120 to 360 ms instead of 3 fixed steps | all |
| [Advert location fallback](docs/lusofw.md#location-in-adverts) | With the `share` policy and no GPS fix, adverts use the `set lat` / `set lon` position instead of 0,0 | repeater, room, sensor |
| [Neighbour expiry](docs/lusofw.md#neighbours) | Forgets neighbours not heard for 48 h | repeater |
| [Board fixes](docs/lusofw.md#board-fixes) | nRF52 DFU register, nRF52 direct GDFU OTA, T-LoRa V2.1 phantom button | listed boards |
| [Display](docs/lusofw.md#display) | Portuguese boot logo, LiPo battery curve | boards with a screen |

### Links

- Official Portuguese website: https://meshcore.pt
- Online flasher: https://flasher.meshcore.pt
- Original website: https://meshcore.io/
