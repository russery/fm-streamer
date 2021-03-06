/*
Manages station and radio configurations.

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

#ifndef __CONFIG_H
#define __CONFIG_H

#include "bsp.h"
#define FILE_DIR "/cfg"
#define FILE_NAME FILE_DIR "/fm_streamer_config.txt"

class Config {
public:
  explicit Config(uint num_stations) : num_stations_(num_stations){};
  void Start(void);
  uint GetStation(void) { return station_; }
  bool SetStation(uint station);
  uint GetFreqkHz(void) { return freq_; }
  float GetFreqMHz(void) { return (float)GetFreqkHz() / 1000.0f; }
  bool SetFreqkHz(uint freq);
  bool SetFreqMHz(float freq);
  uint GetPower(void) { return power_; }
  bool SetPower(uint power);
  uint GetVolume(void) { return volume_; }
  bool SetVolume(uint volume);
  bool GetAutoVolume(void) { return auto_volume_; }
  void SetAutoVolume(bool autovol) { auto_volume_ = autovol; }
  void WriteToFlash(void);

private:
  uint num_stations_;
  uint station_ = 0;
  uint freq_ = 88100;
  uint power_ = 90;
  uint volume_ = 15;
  bool auto_volume_ = true;
};

#endif //__CONFIG_H
