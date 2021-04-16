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

#include <assert.h>

void FmRadio::Start(void){
	bool radio_started = radio_.begin();
    assert(radio_started);
    radio_.beginRDS();
    radio_.setRDSstation("FM Streamer");
}

void FmRadio::PowerDown(void){
	radio_.reset();
}

void FmRadio::SetTxPower(uint percent){
	uint dbuv;
	if (percent > 100) percent = 100; // Saturate at 100%
	if (percent == 0) dbuv = 0; // Turn off if 0%
	else dbuv = ((120-88) * percent / 100) + 88; // 88-120dBuV is valid range
	radio_.setTXpower(dbuv);
	txpower_dbuv_ = dbuv;
}

uint FmRadio::GetTxPower(void){
	return txpower_dbuv_;
}

void FmRadio::SetVolume(uint percent){
	if (percent > 100) percent = 100; // Saturate at 100%
	float gain = (float)(4 * percent) / 100.0f; // Gain range is 0-4
	i2s_input.SetGain(gain);
	vol_percent_ = percent;
}

bool DoAutoSetVolume(void){
	// TODO
	// Read input level at FM radio and adjust I2S volume until it's just below clipping.
	return true;
}

void FmRadio::SetFreq(uint khz) {
	if (khz > 108000) khz = 108000; // Saturate at 108MHz
	else if (khz < 76000) khz = 76000; // Saturate at 76MHz
	khz = khz - (khz % 50); // Ensure we're aligned on 50kHz boundaries
	freq_khz_ = khz;
	radio_.tuneFM(khz/10); // Despite Adafruit library var name, chip expects units of 10kHz
	freq_khz_ = khz;
}

uint FmRadio::GetFreq(void) {
	return freq_khz_;
}

void FmRadio::SetRdsText(char *text){
	radio_.setRDSbuffer(text);
}