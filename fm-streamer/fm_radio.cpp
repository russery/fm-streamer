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
  i2s_output.SetPinout(BSP::I2S_BCLK_PIN, BSP::I2S_WCLK_PIN, BSP::I2S_DATA_PIN);
#if defined(HARDWARE_REV0)
  radio_.Start(true); // Start with I2S input
#else
  radio_.Start(false); // Start with analog input
#endif
  radio_.BeginRDS();
  radio_.SetRDSstation((char *)station_id);
}

void FmRadio::SetI2SInputEnable(bool enabled) {
  if (enabled)
    radio_.EnableI2SInput(44100); // Hardcoded to 44.1kHz - maybe make this tied
                                  // to actual stream bitrate somehow?
  else
    radio_.DisableI2SInput();
}

void FmRadio::SetTxPower(uint percent) {
  uint dbuv;
  if (percent > 100)
    percent = 100; // Saturate at 100%
  if (percent == 0)
    dbuv = 0; // Turn off if 0%
  else
    dbuv = ((120 - 88) * percent / 100) + 88; // 88-120dBuV is valid range
  radio_.SetTXpower(dbuv);
  txpower_percent_ = percent;
}

uint FmRadio::GetTxPower(void) { return txpower_percent_; }

void FmRadio::SetVolume(uint percent) {
  if (percent > 100)
    percent = 100; // Saturate at 100%
  float gain = (float)(1.5f * percent) /
               100.0f; // Gain range is 0-4, but we're only using 0-1.5
  i2s_output.SetGain(gain);
  vol_percent_ = percent;
}

uint FmRadio::GetVolume(void) { return vol_percent_; }

void FmRadio::DoAutoSetVolume(int target_volume) {
  static float avg_input = target_volume;
  static float integral_error = 0;
  static float previous_error = 0;
  const uint AVG_INPUT_CYCLES = 10;
  const float K_P = 0.3f;
  const float K_I = 0.1f;
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

  uint newvolume =
      (uint)(error * K_P + integral_error * K_I + derivative_error * K_D) +
      GetVolume();

  SetVolume(newvolume);
  // Serial.printf(
  //     "\r\nAvg Input: %3.2f flags: %x vol: %d error: %f inlvl: %d int: %f",
  //     avg_input, radio_.CurrASQ, GetVolume(), error, radio_.CurrInLevel,
  //     integral_error);
}

int FmRadio::GetInputLevel(void) {
  radio_.ReadASQStatus();
  return radio_.CurrInLevel;
}

void FmRadio::SetFreq(uint khz) {
  if (khz > 108000)
    khz = 108000; // Saturate at 108MHz
  else if (khz < 76000)
    khz = 76000; // Saturate at 76MHz
  radio_.TuneFM(khz);
  freq_khz_ = khz;
}

uint FmRadio::GetFreq(void) { return freq_khz_; }

void FmRadio::SetRdsText(const char *text) {
  char temp_buff[64];
  strncpy_P(temp_buff, text, sizeof(temp_buff));
  radio_.SetRDSbuffer(temp_buff);
}
