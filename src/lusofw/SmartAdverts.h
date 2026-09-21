#pragma once

#if defined(LUSOFW_SMART_ADVERTS)

#include <Mesh.h>
#include <string.h>

/*
 * Smart advert scheduling policy for lusofw (LUSOFW_SMART_ADVERTS).
 *
 * Instead of every repeater advertising on its own fixed interval (which
 * synchronises into storms), each node hashes its identity into one
 * deterministic slot of a global 23h rolling window. The window is anchored to
 * the wall clock (UTC epoch), so all nodes agree on the calendar without
 * talking to each other; a per-cycle jitter (±3s, derived from the same hash)
 * breaks remaining ties. The slot only changes when the node's name or public
 * key changes, so the schedule is stable across reboots.
 *
 * Without a usable RTC the wall clock is unknown; the schedule then degrades
 * to the same identity-derived slot measured from boot, with jitter varied per
 * uptime window. The uptime clock wraps every ~49.7 days, so one fallback
 * window can be irregular at the wrap.
 */
class SmartAdverts {
public:
  // Rolling window length: 23h so the slot drifts across the whole day.
  static inline const uint32_t WINDOW_SIZE_SECONDS = 23ul * 3600;
  // Adverts whose slotmates collide are separated by up to this much.
  static inline const int32_t JITTER_MAX_SECONDS = 3;
  // Epochs below this (Jan 1, 2026) mean the RTC has no usable time.
  static inline const uint32_t MIN_VALID_EPOCH = 1767225600;

  // The caller converts the wait to milliseconds and passes it as an int.
  static_assert(((int64_t)WINDOW_SIZE_SECONDS + 2 * JITTER_MAX_SECONDS + 1) * 1000 <= 2147483647LL,
                "smart advert wait must fit in Dispatcher::futureMillis(int)");

  /**
   * \brief  Seconds from `now_epoch`/`now_millis` until this node's next smart
   *         flood advert slot (always 1..82,806 seconds).
   * \param  name        node name (hashed together with the key; NULL -> "")
   * \param  pub_key     node public key (first 4 bytes are hashed)
   * \param  now_epoch   current wall-clock time (getRTCClock()->getCurrentTime())
   * \param  now_millis  current uptime millis(), used for scheduling when no RTC
   */
  static uint32_t nextAdvertWaitSeconds(const char* name, const uint8_t* pub_key,
                                        uint32_t now_epoch, uint32_t now_millis) {
    // Deterministic hash of the node identity: uniform and unique per node.
    uint32_t hash = 0;
    name = name ? name : "";
    mesh::Utils::sha256((uint8_t*)&hash, sizeof(hash), (const uint8_t*)name, strlen(name), pub_key, 4);

    const uint32_t my_offset = hash % WINDOW_SIZE_SECONDS;

    // No RTC: anchor the 23h window to uptime. Recompute jitter whenever the
    // candidate advances, so re-arming after a slot cannot reveal another slot
    // a few seconds later in the same window.
    if (now_epoch < MIN_VALID_EPOCH) {
      const uint32_t window_ms = WINDOW_SIZE_SECONDS * 1000ul;
      int64_t current_window_start = (int64_t)now_millis - (int64_t)(now_millis % window_ms);
      uint32_t window_index = (uint32_t)(current_window_start / window_ms);
      int32_t random_jitter_ms =
          ((int32_t)((hash ^ window_index) % ((JITTER_MAX_SECONDS * 2) + 1))
           - JITTER_MAX_SECONDS) * 1000;
      int64_t target_millis = current_window_start + (int64_t)my_offset * 1000 + random_jitter_ms;

      while (target_millis <= (int64_t)now_millis) {
        current_window_start += (int64_t)window_ms;
        window_index++;
        random_jitter_ms =
            ((int32_t)((hash ^ window_index) % ((JITTER_MAX_SECONDS * 2) + 1))
             - JITTER_MAX_SECONDS) * 1000;
        target_millis = current_window_start + (int64_t)my_offset * 1000 + random_jitter_ms;
      }

      int64_t wait_ms = target_millis - (int64_t)now_millis;
      return (uint32_t)((wait_ms + 999) / 1000); // round up: never return zero
    }

    // With RTC: keep cycle arithmetic wider than the epoch input so advancing
    // across the uint32 boundary cannot wrap back into the past.
    int64_t current_cycle_start = (int64_t)now_epoch - (int64_t)(now_epoch % WINDOW_SIZE_SECONDS);
    int32_t random_jitter =
        (int32_t)((hash ^ (uint32_t)current_cycle_start) % ((JITTER_MAX_SECONDS * 2) + 1))
        - JITTER_MAX_SECONDS;
    int64_t target_epoch = current_cycle_start + (int64_t)my_offset + random_jitter;

    // A near-zero slot with negative jitter can still precede the cycle start,
    // so advance until the target is strictly in the future (at most twice).
    while (target_epoch <= (int64_t)now_epoch) {
      current_cycle_start += (int64_t)WINDOW_SIZE_SECONDS;
      random_jitter =
          (int32_t)((hash ^ (uint32_t)current_cycle_start) % ((JITTER_MAX_SECONDS * 2) + 1))
          - JITTER_MAX_SECONDS;
      target_epoch = current_cycle_start + (int64_t)my_offset + random_jitter;
    }

    return (uint32_t)(target_epoch - (int64_t)now_epoch);
  }
};

#endif // LUSOFW_SMART_ADVERTS
