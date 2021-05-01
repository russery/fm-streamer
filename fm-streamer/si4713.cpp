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

#include "si4713.h"
#include <Wire.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

constexpr uint SI4710_ADDR PROGMEM = 0x63;
constexpr uint SI4710_STATUS_CTS PROGMEM = 0x80;

constexpr uint CMD_POWER_UP PROGMEM = 0x01;
constexpr uint CMD_GET_REV PROGMEM = 0x10;
constexpr uint CMD_POWER_DOWN PROGMEM = 0x11;
constexpr uint CMD_SET_PROPERTY PROGMEM = 0x12;
constexpr uint CMD_GET_PROPERTY PROGMEM = 0x13;
constexpr uint CMD_GET_INT_STATUS PROGMEM = 0x14;
constexpr uint CMD_PATCH_ARGS PROGMEM = 0x15;
constexpr uint CMD_PATCH_DATA PROGMEM = 0x16;
constexpr uint CMD_TX_TUNE_FREQ PROGMEM = 0x30;
constexpr uint CMD_TX_TUNE_POWER PROGMEM = 0x31;
constexpr uint CMD_TX_TUNE_MEASURE PROGMEM = 0x32;
constexpr uint CMD_TX_TUNE_STATUS PROGMEM = 0x33;
constexpr uint CMD_TX_ASQ_STATUS PROGMEM = 0x34;
constexpr uint CMD_TX_RDS_BUFF PROGMEM = 0x35;
constexpr uint CMD_TX_RDS_PS PROGMEM = 0x36;
constexpr uint CMD_GPO_CTL PROGMEM = 0x80;
constexpr uint CMD_GPO_SET PROGMEM = 0x81;

constexpr uint PROP_GPO_IEN PROGMEM = 0x0001;
constexpr uint PROP_DIGITAL_INPUT_FORMAT PROGMEM = 0x0101;
constexpr uint PROP_DIGITAL_INPUT_SAMPLE_RATE PROGMEM = 0x0103;
constexpr uint PROP_REFCLK_FREQ PROGMEM = 0x0201;
constexpr uint PROP_REFCLK_PRESCALE PROGMEM = 0x0202;
constexpr uint PROP_TX_COMPONENT_ENABLE PROGMEM = 0x2100;
constexpr uint PROP_TX_AUDIO_DEVIATION PROGMEM = 0x2101;
constexpr uint PROP_TX_PILOT_DEVIATION PROGMEM = 0x2102;
constexpr uint PROP_TX_RDS_DEVIATION PROGMEM = 0x2103;
constexpr uint PROP_TX_LINE_LEVEL_INPUT_LEVEL PROGMEM = 0x2104;
constexpr uint PROP_TX_LINE_INPUT_MUTE PROGMEM = 0x2105;
constexpr uint PROP_TX_PREEMPHASIS PROGMEM = 0x2106;
constexpr uint PROP_TX_PILOT_FREQUENCY PROGMEM = 0x2107;
constexpr uint PROP_TX_ACOMP_ENABLE PROGMEM = 0x2200;
constexpr uint PROP_TX_ACOMP_THRESHOLD PROGMEM = 0x2201;
constexpr uint PROP_TX_ATTACK_TIME PROGMEM = 0x2202;
constexpr uint PROP_TX_RELEASE_TIME PROGMEM = 0x2203;
constexpr uint PROP_TX_ACOMP_GAIN PROGMEM = 0x2204;
constexpr uint PROP_TX_LIMITER_RELEASE_TIME PROGMEM = 0x2205;
constexpr uint PROP_TX_ASQ_INTERRUPT_SOURCE PROGMEM = 0x2300;
constexpr uint PROP_TX_ASQ_LEVEL_LOW PROGMEM = 0x2301;
constexpr uint PROP_TX_ASQ_DURATION_LOW PROGMEM = 0x2302;
constexpr uint PROP_TX_AQS_LEVEL_HIGH PROGMEM = 0x2303;
constexpr uint PROP_TX_AQS_DURATION_HIGH PROGMEM = 0x2304;
constexpr uint PROP_TX_RDS_INTERRUPT_SOURCE PROGMEM = 0x2C00;
constexpr uint PROP_TX_RDS_PI PROGMEM = 0x2C01;
constexpr uint PROP_TX_RDS_PS_MIX PROGMEM = 0x2C02;
constexpr uint PROP_TX_RDS_PS_MISC PROGMEM = 0x2C03;
constexpr uint PROP_TX_RDS_PS_REPEAT_COUNT PROGMEM = 0x2C04;
constexpr uint PROP_TX_RDS_MESSAGE_COUNT PROGMEM = 0x2C05;
constexpr uint PROP_TX_RDS_PS_AF PROGMEM = 0x2C06;
constexpr uint PROP_TX_RDS_FIFO_SIZE PROGMEM = 0x2C07;

void RequestBytes_(uint num_bytes) {
  Wire.requestFrom((uint8_t)SI4710_ADDR, (uint8_t)num_bytes);
  while (Wire.available() < num_bytes)
    yield();
}

void Si4713::SendCommand_(uint len) {
  Wire.beginTransmission((uint8_t)SI4710_ADDR);
  for (uint8_t i = 0; i < len; i++) {
    Wire.write(cmd_buff_[i]);
  }
  Wire.endTransmission();
  // Wait for status CTS bit, indicating command is complete:
  uint8_t status = 0;
  while (!(status & SI4710_STATUS_CTS)) {
    RequestBytes_(1);
    status = Wire.read();
  }
}

void Si4713::SetProperty_(uint property, uint value) {
  cmd_buff_[0] = CMD_SET_PROPERTY;
  cmd_buff_[1] = 0;
  cmd_buff_[2] = property >> 8;
  cmd_buff_[3] = property & 0xFF;
  cmd_buff_[4] = value >> 8;
  cmd_buff_[5] = value & 0xFF;
  SendCommand_(6);
}

bool Si4713::Start(bool use_i2s_input) {
  Wire.begin();
  pinMode(reset_pin_, OUTPUT);
  digitalWrite(reset_pin_, LOW);
  delay(1);
  digitalWrite(reset_pin_, HIGH);
  delay(1);

  cmd_buff_[0] = CMD_POWER_UP;
  // CTS interrupt disabled, GPO2 output disabled, boot normally, enable
  // oscillator, transmit:
  cmd_buff_[1] = 0x12;
  if (use_i2s_input)
    cmd_buff_[2] = 0x0F; // Digital input mode
  else
    cmd_buff_[2] = 0x50; // Analog input mode
  SendCommand_(3);

  if (use_i2s_input)
    SetProperty_(PROP_DIGITAL_INPUT_FORMAT, 0x0008); // I2S Format.
  SetProperty_(PROP_TX_PREEMPHASIS, 0);              // 75µS pre-emph (USA std)
  SetProperty_(PROP_TX_ACOMP_ENABLE,
               0x0003); // Turn on limiter and Audio Dynamic Range Control

  // Check for Si4713:
  cmd_buff_[0] = CMD_GET_REV;
  cmd_buff_[1] = 0;
  SendCommand_(2);
  RequestBytes_(2);
  Wire.read();
  return (Wire.read() == 13); // Good read from chip if we get Si47"13"
}

void Si4713::EnableI2SInput(uint sample_rate_hz) {
  SetProperty_(PROP_DIGITAL_INPUT_SAMPLE_RATE, sample_rate_hz);
}

void Si4713::DisableI2SInput(void) {
  SetProperty_(PROP_DIGITAL_INPUT_SAMPLE_RATE, 0);
}

void Si4713::TuneFM(uint freq_kHz) {
  freq_kHz /= 10; // Convert to 10kHz
  // Force freq to be a multiple of 50kHz:
  if (freq_kHz % 5 != 0)
    freq_kHz -= (freq_kHz % 5);

  cmd_buff_[0] = CMD_TX_TUNE_FREQ;
  cmd_buff_[1] = 0;
  cmd_buff_[2] = freq_kHz >> 8;
  cmd_buff_[3] = freq_kHz;
  SendCommand_(4);

  // Wait for Seek/Tune Complete (STC) bit to be set:
  while ((GetStatus_() & 0x81) != 0x81)
    yield();
}

void Si4713::SetTXpower(uint pwr, uint antcap) {
  cmd_buff_[0] = CMD_TX_TUNE_POWER;
  cmd_buff_[1] = 0;
  cmd_buff_[2] = 0;
  cmd_buff_[3] = pwr;
  cmd_buff_[4] = antcap;
  SendCommand_(5);

  // Wait for Seek/Tune Complete (STC) bit to be set:
  while ((GetStatus_() & 0x81) != 0x81)
    yield();
}

void Si4713::ReadASQStatus(void) {
  cmd_buff_[0] = CMD_TX_ASQ_STATUS;
  cmd_buff_[1] = 0x1;
  SendCommand_(2);

  RequestBytes_(5);
  Wire.read();
  CurrASQ = Wire.read();
  Wire.read();
  Wire.read();
  CurrInLevel = (int8_t)Wire.read();
}

void Si4713::ReadTuneStatus(void) {
  cmd_buff_[0] = CMD_TX_TUNE_STATUS;
  cmd_buff_[1] = 0x1;
  SendCommand_(2);

  RequestBytes_(8);

  Wire.read();
  Wire.read();
  CurrFreq = Wire.read();
  CurrFreq <<= 8;
  CurrFreq |= Wire.read();
  Wire.read();
  CurrdBuV = Wire.read();
  CurrAntCap = Wire.read();
  CurrNoiseLevel = Wire.read();
}

void Si4713::ReadTuneMeasure(uint freq_kHz) {
  freq_kHz /= 10; // Convert to 10kHz
  // Force freq to be a multiple of 50kHz:
  if (freq_kHz % 5 != 0)
    freq_kHz -= (freq_kHz % 5);

  cmd_buff_[0] = CMD_TX_TUNE_MEASURE;
  cmd_buff_[1] = 0;
  cmd_buff_[2] = freq_kHz >> 8;
  cmd_buff_[3] = freq_kHz;
  cmd_buff_[4] = 0;
  SendCommand_(5);

  while (GetStatus_() != 0x81)
    yield();
}

void Si4713::BeginRDS(uint programID) {
  SetProperty_(PROP_TX_AUDIO_DEVIATION, 6625); // 66.25KHz (default is 68.25)
  SetProperty_(PROP_TX_RDS_DEVIATION, 200);    // 2KHz (default)
  SetProperty_(PROP_TX_RDS_INTERRUPT_SOURCE, 0x0001); // RDS IRQ
  SetProperty_(PROP_TX_RDS_PI, programID);
  SetProperty_(PROP_TX_RDS_PS_MIX, 0x03);       // 50% mix (default)
  SetProperty_(PROP_TX_RDS_PS_MISC, 0x1808);    // RDSD0 & RDSMS (default)
  SetProperty_(PROP_TX_RDS_PS_REPEAT_COUNT, 3); // 3 repeats (default)
  SetProperty_(PROP_TX_RDS_MESSAGE_COUNT, 1);
  SetProperty_(PROP_TX_RDS_PS_AF, 0xE0E0); // no AF
  SetProperty_(PROP_TX_RDS_FIFO_SIZE, 0);
  SetProperty_(PROP_TX_COMPONENT_ENABLE, 0x0007); // Enable RDS
}

void Si4713::SetRDSstation(char *s) {
  uint slots = (strlen(s) + 3) / 4;

  for (uint i = 0; i < slots; i++) {
    memset(cmd_buff_, ' ', 6);
    memcpy(cmd_buff_ + 2, s, min(4, (int)strlen(s)));
    s += 4;
    cmd_buff_[6] = 0;
    cmd_buff_[0] = CMD_TX_RDS_PS;
    cmd_buff_[1] = i; // slot number
    SendCommand_(6);
  }
}

void Si4713::SetRDSbuffer(char *s) {
  uint slots = (strlen(s) + 3) / 4;

  for (uint i = 0; i < slots; i++) {
    memset(cmd_buff_, ' ', 8);
    memcpy(cmd_buff_ + 4, s, min(4, (int)strlen(s)));
    s += 4;
    cmd_buff_[8] = 0;
    cmd_buff_[0] = CMD_TX_RDS_BUFF;
    if (i == 0)
      cmd_buff_[1] = 0x06;
    else
      cmd_buff_[1] = 0x04;

    cmd_buff_[2] = 0x20;
    cmd_buff_[3] = i;
    SendCommand_(8);
  }
  // SetProperty_(PROP_TX_COMPONENT_ENABLE, 0x0007); // stereo, pilot+rds
}

uint Si4713::GetStatus_(void) {
  Wire.beginTransmission((uint8_t)SI4710_ADDR);
  Wire.write(CMD_GET_INT_STATUS);
  Wire.endTransmission();
  RequestBytes_(1);
  return Wire.read();
}
