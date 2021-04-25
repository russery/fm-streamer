/*
Wrapper for the Adafruit Si4713 library and I2S audio output to radio.

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

#ifndef __FM_RADIO_H
#define __FM_RADIO_H

#include <Adafruit_Si4713.h>
#include <AudioOutputI2S.h>

// Pin definition - move to BSP someday?
#if defined(ESP32)
const uint RADIO_RESET_PIN PROGMEM = 23;
const uint I2S_BCLK_PIN = 4;
const uint I2S_WCLK_PIN = 17;
const uint I2S_DATA_PIN = 16;
#elif defined(ESP8266)
const uint RADIO_RESET_PIN PROGMEM = 12;
#endif

class FmRadio {
public:
  AudioOutputI2S i2s_output;

  void Start(const char *station_id = "FM Streamer");
  void SetTxPower(uint percent);
  uint GetTxPower(void);
  void SetVolume(uint percent);
  uint GetVolume(void);
  void DoAutoSetVolume(int target_volume = -6);
  int GetInputLevel(void);
  void SetFreq(uint khz);
  uint GetFreq(void);
  void PowerDown(void);
  void SetRdsText(const char *text);

  Adafruit_Si4713 radio_ = Adafruit_Si4713(RADIO_RESET_PIN);

private:
  uint freq_khz_ = 8810;
  uint txpower_percent_ = 88;
  uint vol_percent_ = 80;
};

#endif // __FM_RADIO_H