/*
Streams an internet radio station over FM radio. This is accomplished
with an ESP8266 bridging audio to an I2S-enabled FM transmitter IC.

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

#ifndef __INTERNET_STREAM_H
#define __INTERNET_STREAM_H

#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioOutputI2S.h>
#include <AudioGeneratorMP3.h>

class InternetStream {
public:
	InternetStream(uint buffer_size_bytes, AudioOutputI2S *i2s_sink);
	~InternetStream();

	void OpenUrl(const char* stream_url);
	bool Loop(void);

private:
	AudioGeneratorMP3 *mp3_;
	AudioFileSourceICYStream *file_;
	AudioFileSourceBuffer *buff_;
	AudioOutputI2S *out_;
};


#endif // __INTERNET_STREAM_H