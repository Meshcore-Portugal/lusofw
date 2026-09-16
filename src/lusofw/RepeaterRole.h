#pragma once

#include <helpers/CommonCLI.h>

class RepeaterRole {
public:
  static bool apply(NodePrefs& prefs, uint8_t role) {
    switch (role) {
      case 0: // Tier0 "backbone/infrastructure" (peaks, towers, hilltop) defaults
        prefs.direct_tx_delay_factor = 2.0f;
        prefs.rx_delay_base          = 3.0f;
        prefs.tx_delay_factor        = 2.0f;
        break;
      case 1: // Tier1 "regional/elevated" (mid-elev, high-rise, foothills) defaults
        prefs.direct_tx_delay_factor = 1.0f;
        prefs.rx_delay_base          = 3.0f;
        prefs.tx_delay_factor        = 1.5f;
        break;
      case 2: // Tier2 "local" (rooftop suburban) defaults
        prefs.direct_tx_delay_factor = 0.4f;
        prefs.rx_delay_base          = 3.0f;
        prefs.tx_delay_factor        = 0.8f;
        break;
      case 3: // Tier3 "personal/indoor" (ground, low-roof) defaults
        prefs.direct_tx_delay_factor = 0.1f;
        prefs.rx_delay_base          = 3.0f;
        prefs.tx_delay_factor        = 0.3f;
        break;
      default:
        return false;
    }
    prefs.role = role;
    return true;
  }
};
