#pragma once

#if defined(ENABLE_NETWORK_TIME)

#include <Mesh.h>

/*
 * Network time synchronisation policy for lusofw (ENABLE_NETWORK_TIME).
 * Fully static: no instance state, no changes to the embedding class beyond
 * one-line call sites (see examples/simple_repeater/MyMesh.cpp).
 *
 * The trusted timekeeper identity, plausibility checks and anti-replay policy
 * all live in the .cpp.
 *
 * Freshness is judged against the TIMEKEEPER's clock, never our own: a timestamp
 * is fresh iff it is strictly newer than the highest one ever ACCEPTED. The
 * high-water mark is RAM-only: it resets on reboot, and the first trusted
 * advert heard afterwards re-anchors it.
 *
 * Consequences:
 *  - the local clock may be corrected in EITHER direction (a board whose crystal
 *    ran ahead is pulled back) without weakening replay protection;
 *  - while running, a replayed advert carries an old timestamp (<= mark) and is
 *    always dropped;
 *  - after a reboot, a replayed old advert can only pin the clock at an old
 *    (genuine) timestamp, and only until a newer genuine advert gets through --
 *    no worse than plain RF jamming of the time service, which no broadcast
 *    time protocol can prevent.
 */
class NetTimeSync {
public:
  // Forward sanity bound relative to the current mark. Guards against a buggy
  // (or compromised) timekeeper signing one absurdly large timestamp, which
  // would otherwise wedge the clock ahead of true time until real time caught
  // up (or the next reboot).
  static inline const uint32_t MAX_FORWARD_SECS = 365ul * 24 * 60 * 60;

  /**
   * \brief  Handle an (already signature-verified) advert that may be from the
   *         trusted timekeeper. Applies the clock (either direction) if it is.
   * \returns  true if the clock was updated -- the caller may want to reschedule
   *           clock-driven work (eg. advert timers).
   */
  static bool handleTimekeeperAdvert(const mesh::Identity& id, uint32_t timestamp,
                                     const uint8_t* app_data, size_t app_data_len,
                                     int path_len, mesh::RTCClock& clk);

  static uint32_t getHighWater();
};

#endif // ENABLE_NETWORK_TIME
