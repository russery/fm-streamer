/*
Includes pin definitions and other hardware-specific things.

Copyright (C) 2021  Robert Ussery

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bsp.h"

bool BSP::LED_increasing_brightness_ = true;
bool BSP::LED_on_ = false;

void BSP::StartSi47xxClock(uint freq_hz) {
#ifdef HARDWARE_REV0
  ledcSetup(0, freq_hz, 8);
  ledcAttachPin(RADIO_CLK_PIN, 0);
  ledcWrite(0, 127); // 50% duty cycle on 8-bit timer
#endif               // HARDWARE_REV0
}

void BSP::InitLED(void) {
  pinMode(LED_STREAMING_PIN, OUTPUT);
#if defined(ESP32)
  ledcSetup(1, 1000, 8);
  ledcAttachPin(LED_STREAMING_PIN, 1);
#endif
  SetLED(false);
}

void BSP::SetLED(bool state) {
  if (state) {
#if defined(ESP8266)
    digitalWrite(LED_STREAMING_PIN, LED_ON);
#endif
    LED_on_ = true;
  } else {
#if defined(ESP8266)
    digitalWrite(LED_STREAMING_PIN, LED_OFF);
#elif defined(ESP32)
    ledcWrite(1, 0);
#endif
    LED_on_ = false;
  }
}

void BSP::Loop(void) {
#if defined(ESP32)
  static unsigned long last_led_ms = 0;
  if (millis() - last_led_ms > 10 && LED_on_) {
    last_led_ms = millis();
    int new_brightness = ledcRead(1);
    if (LED_increasing_brightness_) {
      if (++new_brightness > LED_MAX_BRIGHTNESS) {
        LED_increasing_brightness_ = false;
        new_brightness--;
      }
    } else {
      if (--new_brightness < LED_MIN_BRIGHTNESS) {
        LED_increasing_brightness_ = true;
        new_brightness++;
      }
    }
    ledcWrite(1, new_brightness);
  }
#endif
}