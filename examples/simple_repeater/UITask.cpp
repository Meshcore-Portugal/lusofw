#include "UITask.h"
#include "target.h"
#include <Arduino.h>
#include <helpers/CommonCLI.h>
#include <target.h>
#include <lusofw/BatteryCurve.h>
#include <lusofw/BootScreen.h>

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
  if (millis() < _started_at + BOOT_SCREEN_MILLIS) { // boot screen
    BootScreen::drawBootScreen(_display, _version_info, UIColor::secondary_txt);
  } else if (_powering_off_at > 0) {
    BootScreen::drawBootScreen(_display, "Turning OFF", UIColor::secondary_txt);
  } else {
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
