#include "options.h"
#if (TS_MODEL!=TS_MODEL_UNDEFINED) && (DSP_MODEL!=DSP_DUMMY)
#include "Arduino.h"
#include "touchscreen.h"
#include "config.h"
#include "controls.h"
#include "display.h"
#include "player.h"

#ifndef TS_X_MIN
  #define TS_X_MIN              400
#endif
#ifndef TS_X_MAX
  #define TS_X_MAX              3800
#endif
#ifndef TS_Y_MIN
  #define TS_Y_MIN              260
#endif
#ifndef TS_Y_MAX
  #define TS_Y_MAX              3800
#endif
#ifndef TS_STEPS
  #define TS_STEPS              40
#endif
#ifndef TS_DOUBLE_TAP_TICKS
  #define TS_DOUBLE_TAP_TICKS   650
#endif
#ifndef TS_TAP_MIN_TICKS
  #define TS_TAP_MIN_TICKS      20
#endif
#ifndef TS_SWIPE_THRESHOLD
  #define TS_SWIPE_THRESHOLD    35
#endif

#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    // Das CYD-Display belegt HSPI. Der Touchcontroller verwendet deshalb
    // seinen eigenen VSPI-Bus mit frei angegebenen Pins.
    SPIClass  TSSPI(VSPI);
  #endif
  #include <XPT2046_Touchscreen.h>
  XPT2046_Touchscreen ts(TS_CS);
  typedef TS_Point TSPoint;
#elif TS_MODEL==TS_MODEL_GT911
  #include "../GT911_Touchscreen/TAMC_GT911.h"
  TAMC_GT911 ts = TAMC_GT911(TS_SDA, TS_SCL, TS_INT, TS_RST, 0, 0);
  typedef TP_Point TSPoint;
#endif

void TouchScreen::init(uint16_t w, uint16_t h){
  
#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    TSSPI.begin(TS_SPIPINS);
    ts.begin(TSSPI);
  #else
    #if TS_HSPI
      ts.begin(SPI2);
    #else
      ts.begin();
    #endif
  #endif
  ts.setRotation(config.store.fliptouch?3:1);
#endif
#if TS_MODEL==TS_MODEL_GT911
  ts.begin();
  ts.setRotation(config.store.fliptouch?0:2);
#endif
  _width  = w;
  _height = h;
#if TS_MODEL==TS_MODEL_GT911
  ts.setResolution(_width, _height);
#endif
}

tsDirection_e TouchScreen::_tsDirection(uint16_t x, uint16_t y) {
  int16_t dX = x - _oldTouchX;
  int16_t dY = y - _oldTouchY;
  if (abs(dX) > TS_SWIPE_THRESHOLD || abs(dY) > TS_SWIPE_THRESHOLD) {
    if (abs(dX) > abs(dY)) {
      if (dX > 0) {
        return TSD_RIGHT;
      } else {
        return TSD_LEFT;
      }
    } else {
      if (dY > 0) {
        return TSD_DOWN;
      } else {
        return TSD_UP;
      }
    }
  } else {
    return TDS_REQUEST;
  }
}

void TouchScreen::flip(){
#if TS_MODEL==TS_MODEL_XPT2046
  ts.setRotation(config.store.fliptouch?3:1);
#endif
#if TS_MODEL==TS_MODEL_GT911
  ts.setRotation(config.store.fliptouch?0:2);
#endif
}

void TouchScreen::loop(){
  uint16_t touchX, touchY;
  static bool wastouched = true;
  static uint32_t touchLongPress;
  static tsDirection_e direct;
  static uint16_t touchVol, touchStation;
  static bool singleTapPending = false;
  static uint32_t lastTapTicks = 0;
  static uint8_t volumeBeforeMute = 12;
  if (!_checklpdelay(20, _touchdelay)) return;
#if TS_MODEL==TS_MODEL_GT911
  ts.read();
#endif
  bool istouched = _istouched();
  if(istouched){
  #if TS_MODEL==TS_MODEL_XPT2046
    TSPoint p = ts.getPoint();
    touchX = constrain(map(p.x, TS_X_MIN, TS_X_MAX, 0, _width), 0, _width - 1);
    touchY = constrain(map(p.y, TS_Y_MIN, TS_Y_MAX, 0, _height), 0, _height - 1);
  #elif TS_MODEL==TS_MODEL_GT911
    TSPoint p = ts.points[0];
    touchX = p.x;
    touchY = p.y;
  #endif
  if (!wastouched) { /*     START TOUCH     */
      _oldTouchX = touchX;
      _oldTouchY = touchY;
      touchVol = touchX;
      touchStation = touchY;
      direct = TDS_REQUEST;
      touchLongPress=millis();
    } else { /*     SWIPE TOUCH     */
      direct = _tsDirection(touchX, touchY);
      switch (direct) {
        case TSD_LEFT:
        case TSD_RIGHT: {
            singleTapPending = false;
            touchLongPress=millis();
            if(display.mode()==PLAYER || display.mode()==VOL){
              int16_t xDelta = map(abs(touchVol - touchX), 0, _width, 0, TS_STEPS);
              display.putRequest(NEWMODE, VOL);
              if (xDelta>1) {
                controlsEvent((touchVol - touchX)<0);
                touchVol = touchX;
              }
            }
            break;
          }
        case TSD_UP:
        case TSD_DOWN: {
            singleTapPending = false;
            touchLongPress=millis();
            if(display.mode()==PLAYER || display.mode()==STATIONS){
              int16_t yDelta = map(abs(touchStation - touchY), 0, _height, 0, TS_STEPS);
              display.putRequest(NEWMODE, STATIONS);
              if (yDelta>1) {
                controlsEvent((touchStation - touchY)<0);
                touchStation = touchY;
              }
            }
            break;
          }
        default:
            break;
      }
    }
    if (config.store.dbgtouch) {
      Serial.print(", x = ");
      Serial.print(p.x);
      Serial.print(", y = ");
      Serial.println(p.y);
    }
  }else{
    if (wastouched) {/*     END TOUCH     */
      Serial.printf("##TOUCH.RELEASE#: mode=%d y=%u direction=%d held=%lu\n",
                    display.mode(), touchStation, direct,
                    (unsigned long)(millis() - touchLongPress));
      if (direct == TDS_REQUEST) {
        uint32_t pressTicks = millis()-touchLongPress;
        if( pressTicks < BTN_PRESS_TICKS*2){
          if(pressTicks > TS_TAP_MIN_TICKS) {
            if(display.mode() == STATIONS) {
              // In der Playlist startet ein kurzer Druck direkt die sichtbare
              // Zeile unter dem Finger statt immer nur den mittleren Eintrag.
              singleTapPending = false;
              display.selectPlaylistAtY(touchStation);
              direct = TSD_STAY;
              wastouched = istouched;
              return;
            }
            uint32_t now = millis();
            if(singleTapPending && now-lastTapTicks <= TS_DOUBLE_TAP_TICKS) {
              singleTapPending = false;
              if(config.store.volume > 0) {
                volumeBeforeMute = config.store.volume;
                player.setVol(0);
              } else {
                if(volumeBeforeMute == 0) volumeBeforeMute = 12;
                player.setVol(volumeBeforeMute);
              }
              display.putRequest(NEWMODE, VOL);
            } else {
              singleTapPending = true;
              lastTapTicks = now;
            }
          }
        }else{
          singleTapPending = false;
          if(display.mode() == PLAYER) {
            player.toggle();
          } else if(display.mode() == STATIONS) {
            display.putRequest(NEWMODE, PLAYER);
          }
        }
      }
      direct = TSD_STAY;
    } else if(singleTapPending && millis()-lastTapTicks > TS_DOUBLE_TAP_TICKS) {
      singleTapPending = false;
      if(display.mode() == PLAYER) {
        display.putRequest(NEWMODE, STATIONS);
      } else {
        onBtnClick(EVT_BTNCENTER);
      }
    }
  }
  wastouched = istouched;
}

bool TouchScreen::_checklpdelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

bool TouchScreen::_istouched(){
#if TS_MODEL==TS_MODEL_XPT2046
  return ts.touched();
#elif TS_MODEL==TS_MODEL_GT911
  return ts.isTouched;
#endif
}

#endif  // TS_MODEL!=TS_MODEL_UNDEFINED
