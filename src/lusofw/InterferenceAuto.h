#pragma once

/*
 * Auto interference threshold for lusofw.
 * Fully static: no instance state, no changes outside the embedding class'
 * getInterferenceThreshold() override (and the CommonCLI display of the
 * sentinel).
 *
 * prefs.interference_threshold keeps its meaning: 0 disables the RSSI-based
 * listen-before-talk, 1..254 is a fixed threshold in dB above the measured
 * noise floor. The reserved sentinel AUTO (255) instead derives the threshold
 * from the radio's LIVE spreading factor, so it follows `set radio` (applied
 * at reboot) and `tempradio` windows without further configuration.
 *
 * The Dispatcher re-queries getInterferenceThreshold() every
 * NOISE_FLOOR_CALIB_INTERVAL (2s), so an SF change is picked up on the next
 * calibration tick at the latest.
 *
 * Values are decode-margin based: a LoRa signal stays decodable down to about
 * -7.5dB (SF7) .. -20dB (SF12) SNR (cf. snr_threshold[] in
 * RadioLibWrappers.cpp), so the deferral cutoff sits just below each SF's
 * demodulation limit while staying well above RSSI jitter (the busy check is
 * a single-shot read against a 64-sample noise-floor average).
 */
class InterferenceAuto {
public:
  // Sentinel stored in prefs.interference_threshold: derive the threshold
  // from the current spreading factor instead of a fixed value.
  static const uint8_t AUTO = 255;

  /**
   * \brief  Resolve the RSSI listen-before-talk threshold (dB above the
   *         measured noise floor) from the user pref and the current SF.
   * \param  pref        prefs.interference_threshold (0, AUTO, or 1..254 dB)
   * \param  current_sf  the radio's live spreading factor
   * \return 0 to disable the check, or the threshold in dB above the floor
   */
  static int resolve(uint8_t pref, uint8_t current_sf) {
    if (pref != AUTO) {
      return pref;   // 0 = disabled, 1..254 = manual fixed threshold
    }
    if (current_sf < 7) current_sf = 7;    // decode-margin table starts at SF7
    if (current_sf > 12) current_sf = 12;
    static const uint8_t PER_SF[6] = { 8, 10, 11, 14, 16, 16 };   // SF7..SF12
    return PER_SF[current_sf - 7];
  }
};
