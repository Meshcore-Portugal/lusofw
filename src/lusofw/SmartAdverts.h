#pragma once

#if defined(ENABLE_SMART_ADVERTS)

#include <Mesh.h>
#include <string.h>

/*
 * Smart advert scheduling policy for lusofw (ENABLE_SMART_ADVERTS).
 * Fully static, pure math: no state, no timers -- the caller (MyMesh) owns the
 * timer variable and the log output, this class only computes WHEN the next
 * flood advert is due.
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
 * to the same slot measured from boot, with jitter mixed in from millis() so
 * a fleet of nodes rebooting together still spreads out.
 */
class SmartAdverts {
public:
  // Rolling window length: 23h so the slot drifts across the whole day.
  static inline const uint32_t WINDOW_SIZE_SECONDS = 23ul * 3600;
  // Adverts whose slotmates collide are separated by up to this much.
  static inline const int32_t JITTER_MAX_SECONDS = 3;
  // Epochs below this (Jan 1, 2020) mean the RTC has no usable time.
  static inline const uint32_t MIN_VALID_EPOCH = 1577836800;

  /**
   * \brief  Seconds from `now_epoch`/`now_millis` until this node's next smart
   *         flood advert slot (always > 0, except the no-RTC clamp at 0).
   * \param  name        node name (hashed together with the key; NULL -> "")
   * \param  pub_key     node public key (first 4 bytes are hashed)
   * \param  now_epoch   current wall-clock time (getRTCClock()->getCurrentTime())
   * \param  now_millis  current uptime millis(), used for jitter when no RTC
   */
  static uint32_t nextAdvertWaitSeconds(const char* name, const uint8_t* pub_key,
                                        uint32_t now_epoch, uint32_t now_millis) {
    // Deterministic hash of the node identity: uniform and unique per node.
    uint32_t hash = 0;
    name = name ? name : "";
    mesh::Utils::sha256((uint8_t*)&hash, sizeof(hash), (const uint8_t*)name, strlen(name), pub_key, 4);

    const uint32_t my_offset = hash % WINDOW_SIZE_SECONDS;

    // No RTC: rely on uptime millis for jitter that varies across reboots.
    if (now_epoch < MIN_VALID_EPOCH) {
      int32_t random_jitter = ((hash ^ now_millis) % 7) - 3;
      uint32_t fallback_wait = my_offset + random_jitter;
      if ((int32_t)fallback_wait < 0) {
        fallback_wait = 0;   // prevent underflow
      }
      return fallback_wait;
    }

    // With RTC: schedule for the next occurrence in the global calendar.
    uint32_t current_cycle_start = now_epoch - (now_epoch % WINDOW_SIZE_SECONDS);
    uint32_t my_target_epoch = current_cycle_start + my_offset;
    int32_t random_jitter = ((hash ^ current_cycle_start) % ((JITTER_MAX_SECONDS * 2) + 1)) - JITTER_MAX_SECONDS;
    int64_t target_epoch = (int64_t)my_target_epoch + random_jitter;

    // If the calculated target for the current cycle is already in the past or
    // exactly right now, advance to the next cycle to avoid firing twice in a row.
    if ((int64_t)now_epoch >= target_epoch) {
      current_cycle_start += WINDOW_SIZE_SECONDS;
      my_target_epoch = current_cycle_start + my_offset;

      // Re-calculate jitter for the new cycle.
      random_jitter = ((hash ^ current_cycle_start) % ((JITTER_MAX_SECONDS * 2) + 1)) - JITTER_MAX_SECONDS;
      target_epoch = (int64_t)my_target_epoch + random_jitter;
    }

    // target_epoch is now strictly greater than now_epoch.
    return (uint32_t)(target_epoch - (int64_t)now_epoch);
  }
};

#endif // ENABLE_SMART_ADVERTS
