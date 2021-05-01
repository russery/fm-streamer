/*
Silicon Devices Si4713 FM Transmitter Driver Library. Based on the
Adafruit Si4713 library (https://github.com/adafruit/Adafruit-Si4713-Library/),
but significantly simplified and cleaned up, and with support for some
additional command modes.

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

#ifndef __SI4713_H
#define __SI4713_H

#include "bsp.h"

class Si4713 {
public:
  explicit Si4713(uint reset_pin) : reset_pin_(reset_pin){};
  bool Start(bool use_i2s_input = false);
  void EnableI2SInput(uint sample_rate_hz =
                          44100); // Must be called after I2S stream is active.
  void DisableI2SInput(void);     // Must be called before stopping I2S
                                  // stream, or chip may require reset.
  void TuneFM(uint freqKHz);
  void ReadTuneStatus(void);
  void ReadTuneMeasure(uint freq);
  void SetTXpower(uint pwr, uint antcap = 0);
  void ReadASQStatus(void);

  void BeginRDS(uint programID = 0xADAF);
  void SetRDSstation(char *s);
  void SetRDSbuffer(char *s);

  uint CurrFreq;
  uint CurrdBuV;
  uint CurrAntCap;
  uint CurrNoiseLevel;
  uint CurrASQ;
  uint CurrInLevel;

  void SetGPIO(uint x);
  void SetGPIOCtrl(uint x);

private:
  uint reset_pin_;
  uint cmd_buff_[10]; // holds the command buffer

  void SendCommand_(uint len);
  void SetProperty_(uint p, uint v);
  uint GetStatus_(void);
};

#endif // __SI4713_H
