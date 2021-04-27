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

#if defined(ESP32)
// Radio interface pins:
constexpr uint RADIO_RESET_PIN PROGMEM = 23;
constexpr uint I2S_BCLK_PIN PROGMEM = 4;
constexpr uint I2S_WCLK_PIN PROGMEM = 17;
constexpr uint I2S_DATA_PIN PROGMEM = 16;

// Status LED and states:
constexpr char LED_STREAMING_PIN PROGMEM = 2;
constexpr char LED_OFF PROGMEM = LOW;
constexpr char LED_ON PROGMEM = HIGH;

#elif defined(ESP8266)
// Radio interface pins:
constexpr uint RADIO_RESET_PIN PROGMEM = 12;

// Status LED and states:
constexpr char LED_STREAMING_PIN PROGMEM = 16;
constexpr char LED_OFF PROGMEM = HIGH;
constexpr char LED_ON PROGMEM = LOW;

#endif // ESP32 / ESP8266

#endif // __BSP_H