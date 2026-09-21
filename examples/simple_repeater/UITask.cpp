#include "UITask.h"
#include "target.h"
#include <Arduino.h>
#include <helpers/CommonCLI.h>
#include <target.h>
#include <lusofw/BatteryCurve.h>
#include <lusofw/BootScreen.h>

#ifdef HAS_RGB_LOGO
#include "../companion_radio/ui-new/logo_rgb.h"
#endif

#ifndef USER_BTN_PRESSED
#define USER_BTN_PRESSED LOW
#endif

#define AUTO_OFF_MILLIS      20000  // 20 seconds
#define BOOT_SCREEN_MILLIS   5000   // 5 seconds
#define BATT_REFRESH_MILLIS  60000  // 60 seconds
#define POWEROFF_DELAY       3000

void UITask::begin(NodePrefs* node_prefs, const char* build_date, const char* firmware_version) {
  _prevBtnState = HIGH;
  _auto_off = millis() + AUTO_OFF_MILLIS;
  _started_at = millis();
  _node_prefs = node_prefs;
  _cached_batt_mv = 0;
  _next_batt_chck = 0;
  _display->turnOn();

#if defined(PIN_USER_BTN) && defined(DISPLAY_CLASS)
  user_btn.begin();
#endif

  // strip off dash and commit hash by changing dash to null terminator
  // e.g: v1.2.3-abcdef -> v1.2.3
  char *version = strdup(firmware_version);
  char *dash = strchr(version, '-');
  if(dash){
    *dash = 0;
  }

  // v1.2.3 (1 Jan 2025)
  snprintf(_version_info, sizeof(_version_info), "%s (%s)", version, build_date);
  free(version);
}

void UITask::renderCurrScreen() {
  char tmp[80];
#ifdef HAS_RGB_LOGO
  // [lusofw] keep on merge: boot logo is NOT drawn here. It is blitted once
  // AFTER endFrame() in loop(), so the 1-bit diff repaint can never overwrite
  // it with black/white bands (fixes blink + slow band-by-band paint).
  if (millis() < _started_at + BOOT_SCREEN_MILLIS) {
    _display->setColor(UIColor::primary_txt);
    _display->setTextSize(1);
    _display->drawTextCentered(_display->width() / 2, 37, BootScreen::WEBSITE);
    _display->setColor(UIColor::secondary_txt);
    _display->drawTextCentered(_display->width() / 2, 47, _version_info);
  } else if (_powering_off_at > 0) {
    int lx = (PANEL_NATIVE_W - MESHCORE_LOGO_RGB_W) / 2;
    int ly = (PANEL_NATIVE_H - MESHCORE_LOGO_RGB_H) / 2;
    _display->drawRGBBitmap(lx, ly, MESHCORE_LOGO_RGB_W, MESHCORE_LOGO_RGB_H, meshcore_logo_rgb);
    _display->setColor(UIColor::primary_txt);
    _display->setTextSize(1);
    _display->drawTextCentered(_display->width() / 2, 37, BootScreen::WEBSITE);
    _display->setColor(UIColor::secondary_txt);
    _display->drawTextCentered(_display->width() / 2, 47, "Turning OFF");
  } else {
#else
  if (millis() < _started_at + BOOT_SCREEN_MILLIS) { // boot screen
    BootScreen::drawBootScreen(_display, _version_info, UIColor::secondary_txt);
  } else if (_powering_off_at > 0) {
    BootScreen::drawBootScreen(_display, "Turning OFF", UIColor::secondary_txt);
  } else {
#endif
    _display->setCursor(0, 0);
    _display->setTextSize(1);
    _display->setColor(UIColor::primary_txt);
    _display->print(_node_prefs->node_name);

    // freq / sf
    _display->setCursor(0, 20);
    sprintf(tmp, "FREQ: %06.3f SF%d", _node_prefs->freq, _node_prefs->sf);
    _display->print(tmp);

    // bw / cr
    _display->setCursor(0, 30);
    sprintf(tmp, "BW: %03.2f CR: %d", _node_prefs->bw, _node_prefs->cr);
    _display->print(tmp);

    // battery percent (bottom-right corner)
    renderBatteryPercent();
  }
}

void UITask::renderBatteryPercent() {
  int batteryPercentage = BatteryCurve::lipoPercentFromMilliVolts(_cached_batt_mv);

  char tmp[8];
  sprintf(tmp, "%d%%", batteryPercentage);

  _display->setColor(UIColor::primary_txt);
  _display->setTextSize(1);
  _display->drawTextRightAlign(_display->width(), _display->height() - 8, tmp);
}

void UITask::loop() {
#if defined(PIN_USER_BTN) && defined(DISPLAY_CLASS)
  int ev = user_btn.check();
  if (ev == BUTTON_EVENT_CLICK) {
    if (_display->isOn()) {
      // TODO: any action ?
    } else {
      _display->turnOn();
    }
    _auto_off = millis() + AUTO_OFF_MILLIS;   // extend auto-off timer
  } else if (ev == BUTTON_EVENT_LONG_PRESS) {
      _display->turnOn();
      Serial.println("Powering Off");
      _powering_off_at = millis() + POWEROFF_DELAY;
  }
#endif

  if (_display->isOn()) {
    if (millis() > _next_batt_chck) {
      _cached_batt_mv = board.getBattMilliVolts();
      _next_batt_chck = millis() + BATT_REFRESH_MILLIS;
    }
    if (millis() >= _next_refresh) {
      _display->startFrame();
      renderCurrScreen();
      _display->endFrame();

#ifdef HAS_RGB_LOGO
      // [lusofw] keep on merge: RGB logo blitted once, after endFrame(), so
      // the 1-bit diff repaint can never stomp it with black/white bands
      // (fixes blink-once + slow band paint during boot screen wait).
      // Requires PANEL_NATIVE_W / PANEL_NATIVE_H flags on color panels.
      static bool _logo_drawn = false;
      if (!_logo_drawn && _display->isOn()) {
        int lx = (PANEL_NATIVE_W - MESHCORE_LOGO_RGB_W) / 2;
        int ly = (PANEL_NATIVE_H - MESHCORE_LOGO_RGB_H) / 2;
        _display->drawRGBBitmap(lx, ly, MESHCORE_LOGO_RGB_W, MESHCORE_LOGO_RGB_H, meshcore_logo_rgb);
        _logo_drawn = true;
      }
#endif

      _next_refresh = millis() + 1000;   // refresh every second
    }
    if (millis() > _auto_off) {
      _display->turnOff();
    }
  }

  if (_powering_off_at > 0) { // power off timer armed
#ifdef LED_PIN
    digitalWrite(LED_PIN, LED_STATE_ON); // switch on the led until poweroff
#endif
    if (millis() > _powering_off_at) {
      _board->powerOff();  // should not return
    }
  }
}
