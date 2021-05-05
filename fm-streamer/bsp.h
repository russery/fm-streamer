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

#ifndef __BSP_H
#define __BSP_H

#include <Arduino.h>

class BSP {
public:
#if defined(ESP32)
  // Status LED and states:
  static constexpr char LED_STREAMING_PIN PROGMEM = 2;
  static constexpr char LED_OFF PROGMEM = LOW;
  static constexpr char LED_ON PROGMEM = HIGH;

#elif defined(ESP8266)
  // Status LED and states:
  static constexpr char LED_STREAMING_PIN PROGMEM = 16;
  static constexpr char LED_OFF PROGMEM = HIGH;
  static constexpr char LED_ON PROGMEM = LOW;
#endif // ESP32 / ESP8266

#if defined(HARDWARE_PROTO_ESP8266)
  static constexpr uint RADIO_RESET_PIN PROGMEM = 12;
  static constexpr uint SI47XX_CHIP_VERSION = 13; // SI4713
    static constexpr uint SI47xx_I2C_ADDR PROGMEM = 0x63;


  static constexpr uint I2S_BCLK_PIN PROGMEM = 15;
  static constexpr uint I2S_WCLK_PIN PROGMEM = 2;
  static constexpr uint I2S_DATA_PIN PROGMEM = 3;

#elif defined(HARDWARE_PROTO_ESP32)
  static constexpr uint RADIO_RESET_PIN PROGMEM = 23;
  static constexpr uint SI47XX_CHIP_VERSION = 13; // SI4713
  static constexpr uint SI47xx_I2C_ADDR PROGMEM = 0x63;

  static constexpr uint I2S_BCLK_PIN PROGMEM = 4;
  static constexpr uint I2S_WCLK_PIN PROGMEM = 17;
  static constexpr uint I2S_DATA_PIN PROGMEM = 16;

#elif defined(HARDWARE_REV0)
  static constexpr uint RADIO_RESET_PIN PROGMEM = 13;
  static constexpr uint RADIO_CLK_PIN PROGMEM = 25;
  static constexpr uint SI47XX_CHIP_VERSION = 21; // SI4721
    static constexpr uint SI47xx_I2C_ADDR PROGMEM = 0x11;


  static constexpr uint I2S_BCLK_PIN PROGMEM = 16;
  static constexpr uint I2S_WCLK_PIN PROGMEM = 17;
  static constexpr uint I2S_DATA_PIN PROGMEM = 18;

  static constexpr uint I2C_SDA_PIN PROGMEM = 26;
  static constexpr uint I2C_SCL_PIN PROGMEM = 27;

  // static constexpr uint DAC_RESET_PIN PROGMEM = 21;
  // static constexpr uint DAC_CLK_PIN PROGMEM = 34;

  // static constexpr uint SPI_SCK_PIN PROGMEM = 32;
  // static constexpr uint SPI_MOSI_PIN PROGMEM = 33;
  // static constexpr uint DAC_SPI_CS PROGMEM = 35;

#endif // Hardware versions

  static void StartSi47xxClock(uint freq_hz);
};

#endif // __BSP_H