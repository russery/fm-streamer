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

void BSP::StartSi47xxClock(uint freq_hz) {
#ifdef HARDWARE_REV0
  ledcSetup(0, freq_hz, 8);
  ledcAttachPin(RADIO_CLK_PIN, 0);
  ledcWrite(0, 127);
#endif // HARDWARE_REV0
}