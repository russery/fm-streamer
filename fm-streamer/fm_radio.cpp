/*
Wrapper for the Adafruit Si4713 library.

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

#include "fm_radio.h"
#include <assert.h>

void FmRadio::Start(const char *station_id) {
#if defined(ESP32)
  // ESP32 I2S default pinout interferes with I2C, so we have to change it:
  i2s_output.SetPinout(I2S_BCLK_PIN, I2S_WCLK_PIN, I2S_DATA_PIN);
#endif
  radio_.begin();
  radio_.beginRDS();
  radio_.setRDSstation((char *)station_id);
}

void FmRadio::SetTxPower(uint percent) {
  uint dbuv;
  if (percent > 100)
    percent = 100; // Saturate at 100%
  if (percent == 0)
    dbuv = 0; // Turn off if 0%
  else
    dbuv = ((120 - 88) * percent / 100) + 88; // 88-120dBuV is valid range
  radio_.setTXpower(dbuv);
  txpower_percent_ = percent;
}

uint FmRadio::GetTxPower(void) { return txpower_percent_; }

void FmRadio::SetVolume(uint percent) {
  if (percent > 100)
    percent = 100; // Saturate at 100%
  float gain = (float)(1.0f * percent) /
               100.0f; // Gain range is 0-4, but we only need 0-1
  i2s_output.SetGain(gain);
  vol_percent_ = percent;
}

uint FmRadio::GetVolume(void) { return vol_percent_; }

void FmRadio::DoAutoSetVolume(int target_volume) {
  static float avg_input = target_volume;
  static float integral_error = 0;
  static float previous_error = 0;
  const int AVG_INPUT_CYCLES = 20;
  const float K_P = 5.0f;
  const float K_I = 1.0f;
  const float K_D = 0.0f;
  const float INT_SAT_VAL = 10.0f;

  avg_input = (((AVG_INPUT_CYCLES - 1) * avg_input) + GetInputLevel()) /
              AVG_INPUT_CYCLES;

  float error = target_volume - avg_input;

  // Calculate integral and saturate:
  integral_error = integral_error + error;
  if (integral_error > INT_SAT_VAL)
    integral_error = INT_SAT_VAL;
  else if (integral_error < -INT_SAT_VAL)
    integral_error = -INT_SAT_VAL;

  // Calculate derivative:
  float derivative_error = error - previous_error;
  previous_error = error;

  uint newvolume = (uint)(error * K_P) + (uint)(integral_error * K_I) +
                   (uint)(derivative_error * K_D);

  SetVolume(newvolume);
  // Serial.printf("\r\nAvg Input: %3.2f flags: %x vol: %d inlvl: %d int: %f",
  // avg_input, radio_.currASQ, GetVolume(), radio_.currInLevel,
  // integral_error);
}

int FmRadio::GetInputLevel(void) {
  radio_.readASQ();
  return radio_.currInLevel;
}

void FmRadio::SetFreq(uint khz) {
  if (khz > 108000)
    khz = 108000; // Saturate at 108MHz
  else if (khz < 76000)
    khz = 76000;          // Saturate at 76MHz
  khz = khz - (khz % 50); // Ensure we're aligned on 50kHz boundaries
  radio_.tuneFM(
      khz /
      10); // Despite Adafruit library var name, chip expects units of 10kHz
  freq_khz_ = khz;
}

uint FmRadio::GetFreq(void) { return freq_khz_; }

void FmRadio::SetRdsText(const char *text) {
  char temp_buff[64];
  strncpy_P(temp_buff, text, sizeof(temp_buff));
  radio_.setRDSbuffer(temp_buff);
}
