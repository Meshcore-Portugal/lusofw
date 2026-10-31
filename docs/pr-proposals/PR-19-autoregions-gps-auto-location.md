# RFC: Use available GPS fixes for automatic AutoRegions assignment

**Status:** Draft proposal / feature suggestion for LusoFW  
**Scope:** GPS-capable repeater firmware and other supported device variants  
**Implementation:** Not included in this PR

## Summary

Allow `AutoRegions` to use a device's live GPS position when the user has explicitly enabled GPS-based regional assignment and the selected firmware target has a detected, usable GPS provider. The coordinates should flow from the existing location-provider subsystem into the existing region-polygon evaluation. Users should no longer have to run `gps setloc` just to make automatic region assignment use the GPS position.

GPS should take precedence over manually configured coordinates while this feature is enabled and a fresh fix is available. The feature must be opt-in and disabled by default for privacy.

The existing country, macro-region, district, Europe and RF-scope definitions remain the source of truth. This proposal is about selecting a coordinate source and triggering re-evaluation at the right time; it does not propose a new region database or polygon algorithm.

This is a documentation-only suggestion. It records a recommended design for maintainer discussion; it does not modify firmware behaviour, board build flags or defaults.

## Current implementation

The repository already contains most of the GPS building blocks:

- `LocationProvider` defines the common interface for latitude/longitude, validity, timestamps, satellite count, start/stop and polling.
- `MicroNMEALocationProvider` implements that interface for serial NMEA receivers. Its latitude and longitude values are exposed in microdegrees.
- `EnvironmentSensorManager` can compile in GPS support, detect a receiver, start and stop it, poll it, and copy a valid fix into `SensorManager::node_lat` / `node_lon` as decimal degrees.
- `SensorManager::isGPSDetected()` and `getLocationProvider()` expose the runtime detection/provider state.
- The RAK WisBlock path also has a board-specific GPS provider.
- The existing `gps on` / `gps off` commands control the current GPS setting; `gps setloc` copies the current sensor coordinates into `NodePrefs::node_lat` / `node_lon`, saves preferences and triggers the node-configuration callback.

However, the current AutoRegions path does not consume the live position:

1. `AutoRegions::checkRegionAutoAssign()` reads `NodePrefs::node_lat` and `node_lon`. It does not query `SensorManager` or a `LocationProvider`.
2. `EnvironmentSensorManager::loop()` updates the live sensor coordinates, but it does not notify AutoRegions when a new fix arrives or changes.
3. The existing `gps setloc` command is currently the bridge between live GPS and AutoRegions evaluation.
4. GPS support is compile-time gated by `ENV_INCLUDE_GPS`. A board having GPS pins is not, by itself, enough to create a provider. The current `AutoRegions.h` policy comment says hardware GPS is deliberately not consulted because GPS is disabled on repeaters; GPS-capable hardware/build profiles therefore need an explicit, reviewed enablement change.
5. AutoRegions considers configured coordinates available only when at least one coordinate is non-zero and `advert_loc_policy != ADVERT_LOC_NONE`. Otherwise, it tries the node-name-prefix fallback. Compile-time `ADVERT_LAT` / `ADVERT_LON` defaults are used for regulation only; they do not create geographical regions.
6. In `CommonCLI::buildAdvertData()`, `ADVERT_LOC_NONE` omits coordinates from the advertisement; `ADVERT_LOC_PREFS` uses the saved preference coordinates; and `ADVERT_LOC_SHARE` uses the live sensor coordinates.

Relevant code:

- [AutoRegions.cpp](../../src/lusofw/AutoRegions.cpp) and [AutoRegions.h](../../src/lusofw/AutoRegions.h)
- [SensorManager.h](../../src/helpers/SensorManager.h)
- [LocationProvider.h](../../src/helpers/sensors/LocationProvider.h) and [MicroNMEALocationProvider.h](../../src/helpers/sensors/MicroNMEALocationProvider.h)
- [EnvironmentSensorManager.h](../../src/helpers/sensors/EnvironmentSensorManager.h) and [EnvironmentSensorManager.cpp](../../src/helpers/sensors/EnvironmentSensorManager.cpp)
- [MyMesh.cpp](../../examples/simple_repeater/MyMesh.cpp) and [MyMesh.h](../../examples/simple_repeater/MyMesh.h)
- [CommonCLI.cpp](../../src/helpers/CommonCLI.cpp), including `gps setloc` and advertisement location policy handling
- GPS-capable board targets and their PlatformIO build profiles

## Proposed behaviour

### 1. Use the existing GPS provider, not a second GPS implementation

Keep hardware-specific detection and power management in `EnvironmentSensorManager` and the existing `LocationProvider` implementations. AutoRegions should receive a selected coordinate pair (and, ideally, its source/freshness metadata) rather than knowing about UARTs, GPIO pins, MicroNMEA, RAK GPS hardware or individual boards.

Only targets that explicitly compile GPS support and have a detected/usable provider should use this path. Do not infer GPS availability from pin definitions alone. Do not enable GPS in every repeater profile indiscriminately; enable only hardware profiles that actually contain a supported receiver.

### 2. Make GPS-based AutoRegions an explicit privacy opt-in

Introduce a dedicated persisted boolean preference, tentatively named `auto_regions_gps_enabled`, with a default of `false`.

- The user must explicitly enable it before AutoRegions may start or consume GPS for regional assignment.
- Provide a clear CLI/configuration control, for example `autoregions gps on|off`; the final command name can follow maintainer conventions.
- Disabling this preference must stop AutoRegions from requesting GPS fixes. It must not change the existing `gps advert` preference.
- Keep this preference separate from the existing `gps_enabled` setting. The latter controls the current normal/continuous GPS operation; the new preference expresses consent to use GPS for automatic regional assignment.
- If normal GPS operation is already active, AutoRegions should reuse a fresh live fix rather than starting another acquisition.
- If normal GPS operation is inactive but the AutoRegions opt-in is enabled, AutoRegions may request a temporary one-shot acquisition and then restore the receiver to its previous state.

The feature must remain disabled by default on all targets, including GPS-equipped boards. A build having GPS support must not silently imply user consent to use it for regional inference.

### 3. GPS acquisition timeout: 15 seconds, without blocking startup

Use a **15-second maximum acquisition window** as the initial design target (within the requested 10–15 second range).

- Implement the wait as a non-blocking state machine driven by the normal firmware loop; do not use a blocking delay loop.
- Accept the first valid, fresh fix received within the window.
- If the deadline expires without a valid fix, stop the temporary acquisition, restore the prior GPS power state and continue with the fallback chain.
- Normal mesh/radio startup must never wait for the GPS timeout.
- A 15-second window is a power/startup budget, not a guarantee that a cold-start receiver will obtain a fix. If no fix arrives in that window, fallback is expected; later retries should be a separately reviewed policy rather than silently extending startup or leaving GPS powered.

If GPS was already active before AutoRegions requested a location, do not turn it off when the one-shot request completes.

### 4. Define coordinate-source precedence explicitly

When `auto_regions_gps_enabled` is true, use this order:

1. A valid, fresh GPS fix. GPS takes priority over manually configured coordinates while this feature is enabled.
2. Manually configured `NodePrefs::node_lat` / `node_lon`, provided the existing `advert_loc_policy != ADVERT_LOC_NONE` condition allows them.
3. The existing node-name-prefix fallback when neither coordinate source is usable.
4. Compile-time default coordinates remain regulation-only and must not create regions.

When the new preference is disabled, preserve the current AutoRegions source-selection behaviour unchanged.

GPS coordinates must be range-checked (finite values, latitude [-90, 90], longitude [-180, 180]) and checked for freshness before polygon evaluation. A stale, invalid or unavailable fix must not override usable manual coordinates.

### 5. Keep advertisement privacy separate from local GPS region inference

The current implementation gives `advert_loc_policy` two related effects:

- In `CommonCLI::buildAdvertData()`, `ADVERT_LOC_NONE` omits exact coordinates from the self-advertisement, `ADVERT_LOC_PREFS` uses saved preference coordinates, and `ADVERT_LOC_SHARE` uses live sensor coordinates.
- In AutoRegions, `ADVERT_LOC_NONE` also prevents configured preference coordinates from being used for polygon assignment. The node-name-prefix fallback can still assign coarse regions when coordinates are unavailable.

The proposal is to preserve the existing exact-coordinate advertisement behaviour and introduce a separate, explicit opt-in for GPS-derived regional assignment:

- Enabling `auto_regions_gps_enabled` authorizes AutoRegions to derive coarse region membership from a GPS fix. It does **not** authorize exact GPS coordinates to be added to advertisements.
- `ADVERT_LOC_NONE` must continue to omit exact coordinates from advertisements. Do not change its payload behaviour.
- Do not copy one-shot GPS coordinates into `NodePrefs::node_lat` / `node_lon` or persist exact live coordinates as a side effect of AutoRegions.
- Prefer passing the one-shot coordinate pair directly from the provider to AutoRegions, rather than publishing it through `SensorManager::node_lat` / `node_lon`. This avoids accidentally making a temporary AutoRegions fix available to the `ADVERT_LOC_SHARE` or telemetry paths.
- If the user has explicitly enabled normal GPS operation and selected `ADVERT_LOC_SHARE`, the existing live-coordinate sharing behaviour remains unchanged.
- Document clearly that region membership is a coarser location signal and may be observable through mesh region/scope behaviour. `ADVERT_LOC_NONE` hides exact coordinates; it must not be described as a guarantee that no approximate location can be inferred. The new preference is the explicit consent for the additional GPS-derived coarse assignment.

This keeps the three concerns distinct: GPS hardware operation, GPS-based local regional inference, and exact-coordinate advertisement policy.

### 6. Re-evaluate only when useful

- Trigger an evaluation when the first valid GPS fix becomes available.
- For continuously active GPS, re-evaluate after the position changes sufficiently or crosses a relevant region boundary.
- Avoid evaluating on every GPS poll.
- Use a freshness check and coordinate validation before polygon lookup.
- Avoid repeated `RegionMap` writes when the resulting region tree has not changed. Preserve the current map-change-only persistence behaviour.
- Avoid rapid oscillation near polygon borders; define a stability/hysteresis policy if GPS jitter can repeatedly switch between adjacent polygons.
- On GPS loss, do not replace a previously valid assignment with zero/invalid coordinates. Retain the last safe assignment or follow an explicitly documented fallback policy.

### 7. Reuse the existing region catalog and assignment rules

Once a valid GPS fix is available, pass its latitude/longitude to the existing polygon evaluation. Reuse the compiled-in country registry and all existing region layers:

- country membership;
- macro-regions;
- districts;
- `#europe` membership;
- country-specific RF scopes such as Portugal's `#pt-433` and `#pt-868`;
- existing hierarchy injection, cycle checks and region-map persistence.

Do not duplicate polygon definitions or introduce a second region-assignment implementation for GPS.

## Fallback and failure behaviour

| GPS / coordinate state | Expected AutoRegions behaviour |
|---|---|
| GPS support not compiled for the target | Preserve the current configured-coordinate and name-prefix behaviour |
| AutoRegions GPS opt-in is disabled | Do not request or consume GPS; preserve current behaviour |
| Opt-in enabled, but no provider is detected | Use configured coordinates (subject to the existing advert policy gate), then name-prefix fallback |
| Provider detected, but no valid fresh fix within 15 seconds | Stop temporary acquisition, restore the previous GPS state, then use configured coordinates and name-prefix fallback |
| GPS fix invalid, stale or out of range | Ignore the fix and use the fallback chain |
| Valid GPS fix available and opt-in enabled | Use GPS coordinates first, even when valid manual coordinates are configured |
| GPS later stops or loses fix | Do not feed zero/invalid coordinates into polygon evaluation; retain the last safe result or follow the agreed fallback policy |

A missing GPS receiver or a slow time-to-first-fix must never hold up the repeater's normal mesh/radio startup.

## Acceptance criteria for an implementation PR

- [ ] A new persisted AutoRegions GPS opt-in exists and defaults to disabled.
- [ ] The user can explicitly enable and disable GPS-based AutoRegions independently of exact-coordinate advertisement policy.
- [ ] No GPS acquisition for AutoRegions occurs while the new opt-in is disabled.
- [ ] GPS-capable repeater build profiles explicitly enable the existing provider only where the hardware is present; non-GPS profiles remain unchanged.
- [ ] A detected GPS receiver can provide the first AutoRegions location automatically, without `gps setloc`.
- [ ] GPS takes precedence over configured coordinates only while the opt-in is enabled and the fix is valid/fresh.
- [ ] A one-shot acquisition has a 15-second non-blocking deadline and restores the previous GPS power state.
- [ ] The 15-second timeout never blocks normal firmware startup.
- [ ] One-shot GPS coordinates are passed to AutoRegions without being persisted or accidentally exposed through advertisement/telemetry sensor fields.
- [ ] `ADVERT_LOC_NONE`, `ADVERT_LOC_PREFS` and `ADVERT_LOC_SHARE` retain their existing exact-coordinate advertisement semantics.
- [ ] The coarse-location implications of region membership are documented next to the new opt-in.
- [ ] The current country, macro-region, district, Europe and RF-region catalog is reused unchanged.
- [ ] Manual configured coordinates and node-name fallback remain available when GPS is unavailable.
- [ ] No-GPS builds continue to compile and behave as before.
- [ ] Region-map flash writes occur only when the resulting map changes.
- [ ] Unit tests cover opt-in off/on, GPS precedence, no provider, no fix/timeout, invalid coordinates, stale fix, manual-coordinate fallback, name-prefix fallback, movement across a region boundary, repeated identical fixes, and advertisement policy independence.
- [ ] Hardware tests cover at least one serial GPS target (for example, Heltec T114) and one supported alternative provider/board if included in the build matrix.

## Suggested implementation split

1. **Preference and user control:** add the disabled-by-default AutoRegions GPS opt-in and a CLI/configuration control.
2. **GPS integration:** expose a small, board-independent, non-blocking one-shot acquisition/update API through `SensorManager` / `EnvironmentSensorManager`, preserving the receiver's prior power state.
3. **AutoRegions input:** decouple polygon evaluation from `NodePrefs` so it can evaluate an explicit GPS coordinate pair without persisting or publishing exact coordinates.
4. **Source selection and privacy:** implement GPS-first precedence only when opted in, preserve the existing advertisement policy semantics and document the coarse-region disclosure.
5. **Target enablement:** enable `ENV_INCLUDE_GPS` only in repeater profiles whose hardware actually includes a supported GPS receiver.
6. **Tests and hardware validation:** add native tests for opt-in, source selection and fallback behaviour, then verify acquisition/power lifecycle on real GPS boards.

## Decisions proposed for maintainer review

| Topic | Proposed decision |
|---|---|
| User consent | Dedicated AutoRegions GPS preference, default `false` |
| Coordinate priority | Fresh valid GPS first while opted in; configured coordinates second; name prefix third |
| Exact-coordinate advertisement | Keep current `ADVERT_LOC_NONE` / `PREFS` / `SHARE` behaviour unchanged |
| Coarse-region privacy | Explicit AutoRegions GPS opt-in is consent to GPS-derived region membership; document that regions can reveal approximate location |
| Acquisition budget | 15 seconds, non-blocking; fallback and power-state restoration on timeout |
| Persistence | Never persist the one-shot exact GPS coordinates |

## Remaining implementation questions

1. What exact preference and CLI names best match LusoFW conventions?
2. Should timed-out one-shot acquisition be retried later (for example, after an explicit user action or a low-frequency background retry), or only on the next boot/configuration event?
3. What freshness threshold and polygon-boundary hysteresis are appropriate for the supported GPS providers and stationary/mobile targets?

## Scope of this PR

This PR records a feature suggestion and proposed acceptance criteria only. It does not modify firmware behaviour, enable GPS flags on any board, or change existing preferences. Implementation should follow after maintainers review the privacy, power and lifecycle design.
