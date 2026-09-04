#pragma once

#if defined(ENABLE_ADVERT_PROTECT)

#include <Mesh.h>

/*
 * Advert protection policy for lusofw (ENABLE_ADVERT_PROTECT).
 * Fully static: no instance state, no changes to the embedding class beyond
 * one call site (see examples/simple_repeater/MyMesh.cpp).
 *
 * The first advert heard from a given remote REPEATER is allowed to be
 * repeated by us; any further advert from the same origin (public key) is a
 * duplicate and is not repeated until 12h have passed since the last allowed
 * one. Suppressed adverts are still processed locally (contacts, neighbours,
 * network time) -- the caller just marks them do-not-retransmit.
 *
 * All state is RAM-only: it resets on reboot (like the neighbours list and
 * NetTimeSync's high-water mark), so the first advert heard after a reboot is
 * repeated once and the window re-arms.
 *
 * The window is armed at ACCEPT time: an advert we allow but that is later
 * dropped by allowPacketForward() (forwarding disabled, hop limit) still
 * starts a new window. Accepted to keep a single call site; it self-corrects
 * at the next window.
 */
class AdvertProtection {
public:
  // Minimum time between repeats, by us, of adverts from the same repeater.
  static inline const uint32_t REPEAT_WINDOW_SECS = 12ul * 60 * 60;

  /**
   * \brief  Decide (and record) whether a flood advert from a REPEATER origin
   *         may be repeated by us now: true for the first one, false for any
   *         further advert from the same public key inside the window.
   * \param  origin  the advert's (already signature-verified) identity
   * \param  now     current local time
   */
  static bool allowRepeaterAdvertRepeat(const mesh::Identity& origin, uint32_t now);
};

#endif // ENABLE_ADVERT_PROTECT
