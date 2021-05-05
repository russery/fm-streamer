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

#include "internet_stream.h"
#include <Arduino.h>

InternetStream::InternetStream(uint buffer_size_bytes, AudioOutputI2S *i2s_sink)
    : out_(i2s_sink) {
  file_ = new AudioFileSourceHTTPStream();
  buff_ = new AudioFileSourceBuffer(file_, buffer_size_bytes);
  // cppcheck-suppress [noOperatorEq,noCopyConstructor]
  mp3_ = new AudioGeneratorMP3();
}

InternetStream::~InternetStream() {
  delete file_;
  delete buff_;
  delete mp3_;
}

void InternetStream::OpenUrl(const char *stream_url) {
  file_->open(stream_url);
  mp3_->begin(buff_, out_);
}

bool InternetStream::Loop(void) { return mp3_->loop(); }

void InternetStream::Flush(void) {
  file_->close();
  out_->flush();
  mp3_->stop();
}
