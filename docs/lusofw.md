# How lusofw differs from MeshCore

This page lists every behaviour change lusofw makes on top of upstream MeshCore. The current base is MeshCore v1.17.1 (`d9296435`). The [README](../README.md#differences-from-upstream) has the summary tables; each section here explains one row.

Unless stated otherwise, a change applies to the repeater firmware only. Companion, room server and sensor firmware keep upstream behaviour except where a section says so.

Most features sit behind a `LUSOFW_*` build flag in `platformio.ini`. Turning a flag off restores the upstream code path.

## Defaults migration

Build flag: none (always on for repeaters). Code: `src/lusofw/Defaults.cpp`.

The repeater stores the firmware version it last booted in `/lusofw`. When the running version differs (including the first boot after flashing), it rewrites some preferences, saves them, stamps the new version and reboots once.

These five settings are reset on every version change. Values you set with the CLI are lost on each upgrade:

| Setting | Value after upgrade |
|---|---|
| `advert.interval` | `0` (off) |
| `flood.advert.interval` | `23` |
| `loop.detect` | `moderate` |
| `path.hash.mode` | `1` (2-byte hashes) |
| `advert.loc.policy` | `prefs` |

These are one-shot migrations. They run once, when upgrading from an older version or on a fresh install, and your later changes are kept:

| From version older than | Change |
|---|---|
| 2026.7.1 | `af` set to `1.0`. `txdelay` 0.5, `rxdelay` 0.0, `direct.txdelay` 0.3. |
| 2026.9.1-rc4 | `radio_manual` latch inferred: set if `af` is not 1.0 or `tx` is not the board default. |
| 2026.9.1 | `cad` turned off. `int.thresh` set to `auto` (255) if it was `0`. RX boosted gain turned on (SX1262/SX1268). The legacy `#portugal` region is removed. |

The companion firmware does its own rewrite, on every boot: if `int_thr` is `0` when prefs load, it becomes `auto` and is saved. A companion cannot keep `int.thresh` at `0`.

## Repeater tiers

Build flag: none. Code: `src/lusofw/RepeaterRole.h`. CLI: `set tier <0-3>`, `get tier`.

A tier sets the three retransmit delay factors in one go. Higher sites wait longer, so lower nodes nearby get the first chance to relay.

| Tier | Site | `direct.txdelay` | `rxdelay` | `txdelay` |
|---|---|---|---|---|
| 0 | Backbone, peaks, towers | 2.0 | 3.0 | 2.0 |
| 1 | Regional, high ground | 1.0 | 3.0 | 1.5 |
| 2 | Local rooftop | 0.4 | 3.0 | 0.8 |
| 3 | Personal, indoor | 0.1 | 3.0 | 0.3 |
| upstream | | 0.3 | 0.0 | 0.5 |

A node with no stored tier is assigned tier 3 at boot. On a fresh install the 2026.7.1 migration above then runs and puts the upstream delays back, so `get tier` reports `3` while the delays are `txdelay` 0.5, `rxdelay` 0.0, `direct.txdelay` 0.3. Upgrades from 2026.7.1 or later skip that migration and do get the tier 3 values. Run `set tier 3` (or your real tier) once after flashing a new node.

## Automatic regions

Build flag: `LUSOFW_AUTO_REGIONS`. Code: `src/lusofw/AutoRegions.cpp`, polygons in `src/lusofw/regions/`.

The repeater works out its regions and creates them in the region map with no manual setup. It tries, in order:

1. The coordinates set with `set lat` / `set lon`, if non-zero and `advert.loc.policy` is not `none`. These are matched against district and macro-region polygons.
2. The first two letters of the node name, case-insensitive, for example `AV` for Aveiro, `BR` for Braga. This gives one district and one macro region.
3. The build-time `ADVERT_LAT` / `ADVERT_LON`, used for radio regulation only. No regions are created.

A match in Portugal creates `#europe`, `#pt`, the district and macro region, plus `#pt-433` or `#pt-868` from the current frequency. Regions you created yourself are never touched. Only Portugal is compiled in; Spain and Brazil tables exist but are disabled in `AutoRegions.cpp`.

Hardware GPS is not used. lusofw removes GPS support from repeaters.

Regions are re-evaluated when you change the name, coordinates, location policy or frequency.

### Radio regulation

When the node is located in Europe, AutoRegions also sets TX power and duty cycle:

| Frequency | TX power cap |
|---|---|
| 869.40 to 869.65 MHz | 27 dBm |
| 863 to 870 MHz (rest) | 14 dBm |
| 433.05 to 434.79 MHz | 10 dBm |
| other | 10 dBm |

The cap never exceeds the board's `LORA_TX_POWER`. Boards with an external PA subtract its gain first (`LUSOFW_TX_PA_GAIN`, 10 dB on the LilyGo T-Beam 1W). Duty cycle is set to 10% (`af 9`). Outside Europe, or with no location, the board default power and `af 1` (50%) apply.

Running `set tx`, `set af` or `set dutycycle` sets a `radio_manual` latch. From then on AutoRegions leaves power and duty cycle alone.

## Smart adverts

Build flag: `LUSOFW_SMART_ADVERTS`. Code: `src/lusofw/SmartAdverts.h`.

Upstream sends a zero-hop advert every `advert.interval` × 2 minutes and a flood advert every `flood.advert.interval` hours, counted from boot. Repeaters that reboot together stay in step and their adverts collide.

lusofw disables zero-hop adverts. Each repeater hashes its name and the first 4 bytes of its public key into a fixed slot inside a 23-hour window anchored to UTC, with up to ±3 s of jitter. Because the window is 23 hours, the slot moves an hour earlier each day and covers the whole clock over 23 days. The slot changes only when the name or key changes.

`flood.advert.interval` works as an on/off switch here: any value above `0` enables the schedule, `0` disables it. Without a valid clock (before 1 Jan 2026) the window is measured from boot instead.

`tools/next_adverts.js` and `tools/find_advert_name.js` compute slots offline.

## Advert protection

Build flag: `LUSOFW_ADVERT_PROTECT`. Code: `src/lusofw/AdvertProtection.cpp`.

A repeater relays the flood advert of each remote repeater at most once every 12 hours, keyed on public key. Duplicates are still processed locally (contacts, neighbours, time) but not retransmitted. This stops old firmware stuck in a boot loop from flooding the mesh with adverts for hours. The table lives in RAM and resets on reboot.

## Mobile repeater filter

Build flag: none.

Flood adverts from repeaters whose public key starts with `0x01` are not forwarded. lusofw treats `0x01` as the marker for mobile repeaters. New repeater keys are regenerated if they start with `0x01`, in addition to upstream's reserved `0x00` and `0xFF`.

## Network time

Build flag: `LUSOFW_NETWORK_TIME`. Code: `src/lusofw/NetTimeSync.cpp`.

The repeater sets its clock from adverts signed by one trusted timekeeper key, compiled into the firmware. An advert is accepted when it is a valid signed advert of type `none`, arrived in fewer than 10 hops, carries a timestamp after 1 Jan 2026, and is newer than the last accepted timestamp. Timestamps more than 365 days ahead of the last accepted one are rejected.

The clock may move forward or backward. Rate limiters re-anchor when the clock steps back, so a correction does not lock out anonymous requests or neighbour discovery.

## Interference threshold auto mode

Build flag: `LUSOFW_RADIO_AUTO_THRESH`. Code: `src/lusofw/InterferenceAuto.h`. Applies to repeater, room server, sensor and companion.

`set int.thresh 255` (shown as `auto`) derives the RSSI listen-before-talk threshold from the live spreading factor: SF7 8 dB, SF8 10 dB, SF9 11 dB, SF10 14 dB, SF11 and SF12 16 dB above the noise floor.

Upstream's companion firmware ignores `int.thresh` and always returns `0`. lusofw applies it and defaults it to `auto`. Repeaters get `auto` through the migration. Room server and sensor keep upstream's default of `0` but accept `255`.

## Loop detection

Upstream ships `loop.detect` off. lusofw sets it to `moderate` on every version change. Moderate stops forwarding a flood packet whose path already contains the repeater's own hash twice (1-byte hashes) or once (2- and 3-byte hashes).

## 2-byte path hashes

Upstream repeaters use 1-byte path hashes (`path.hash.mode 0`). lusofw uses 2 bytes (`path.hash.mode 1`) on repeaters, which cuts false matches between repeaters that share a first key byte. Companion firmware keeps the upstream default.

## Core packet handling

These apply to every firmware type, since they live in `src/`.

- Outbound packets that wait in the TX queue longer than 60 s are dropped and removed from the dedup table, so a later copy can still be relayed. Override with `MAX_PACKET_QUEUE_AGE_MS`.
- After a busy channel the retry delay is a random value from 120 to 360 ms. Upstream picks 120, 240 or 360 ms, which can keep nearby nodes in lock-step.

The packet pool also ignores null packets and logs a possible double free.

## Neighbours

Repeater neighbours not heard for 48 hours are removed from the list.

## Location in adverts

When the firmware is built without GPS, which is the case for all lusofw repeaters, adverts always carry the configured latitude and longitude. `advert.loc.policy none` does not hide them. Upstream honours the policy in all builds.

## Build changes

- Default node name is `lusofw-<version>` for repeater, room server and sensor, instead of `repeater`, `Test BBS` and `sensor`. Per-variant names are removed with `build_unflags`.
- `ENABLE_ADVERT_ON_BOOT` is off: repeaters, room servers and sensors do not advertise on boot.
- GPS is removed from most variants. The Heltec T1 and T114 companion builds keep it.
- Sensor drivers are cut to AHTx0, BME280, BMP280 and INA3221. SHTC3 is added on RAK4631 and RAK3112 (RAK1901). Upstream also builds SHT4x, LPS22HB, INA219, INA226, INA260, MLX90614, VL53L0X, BME680 and BMP085.
- Extra build targets: Heltec V4 RS232 bridge, Heltec V4 R8 ESP-NOW bridge, SenseCap Solar RS232 bridge. The bridges themselves are upstream code.
- `MESH_DEBUG` and `LUSOFW_RADIO_DEBUG` are on in `platformio.ini`. Release builds strip them with `DISABLE_DEBUG=1`.
- `LUSOFW_STEALTH_MODE` (off by default) makes a repeater ignore node discovery requests.

## Board fixes

- nRF52: clears the GPREGRET register at boot and before power-off, so a stale DFU value cannot drop the device into the bootloader.
- Heltec T114: `start ota` reboots straight into a `gdfu-` bootloader (`LUSOFW_OTA_DIRECT`). True-colour Portuguese boot logo and DejaVu Sans font.
- LilyGo T-LoRa V2.1-1.6: user button removed. GPIO0 floats on this board and could be read as a long press that powers the repeater off.
- LilyGo T-Echo Lite: uses the SX126x LDO regulator.

## Display

- `LUSOFW_BOOT_LOGO_PT`: Portuguese boot logo instead of the MeshCore wordmark.
- `LUSOFW_LIPO_CURVE`: battery percentage follows a LiPo discharge curve instead of a linear voltage map. The repeater home screen shows it.
