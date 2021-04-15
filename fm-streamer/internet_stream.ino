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

namespace {

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MetaDataCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
    const char *ptr = reinterpret_cast<const char *>(cbData);
    (void) isUnicode; // Punt this ball for now
    // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
    char s1[32], s2[64];
    strncpy_P(s1, type, sizeof(s1));
    s1[sizeof(s1)-1]=0;
    strncpy_P(s2, string, sizeof(s2));
    s2[sizeof(s2)-1]=0;
    Serial.printf("METADATA(%s) '%s' = '%s'\r\n", ptr, s1, s2);
    Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string) {
    const char *ptr = reinterpret_cast<const char *>(cbData);
    // Note that the string may be in PROGMEM, so copy it to RAM for printf
    char s1[64];
    strncpy_P(s1, string, sizeof(s1));
    s1[sizeof(s1)-1]=0;
    Serial.printf("STATUS(%s) '%d' = '%s'\r\n", ptr, code, s1);
    Serial.flush();
}

} // namespace

InternetStream::InternetStream(const char* stream_url, uint buffer_size_bytes) {
    file_ = new AudioFileSourceICYStream(stream_url);
    file_->RegisterMetadataCB(MetaDataCallback, (void*)"stream");
    file_->RegisterStatusCB(StatusCallback, (void*)"stream");

    buff_ = new AudioFileSourceBuffer(file_, buffer_size_bytes);
    buff_->RegisterMetadataCB(MetaDataCallback, (void*)"buffer");
    buff_->RegisterStatusCB(StatusCallback, (void*)"buffer");

    out_ = new AudioOutputI2S();
    out_->RegisterMetadataCB(MetaDataCallback, (void*)"I2S");
    out_->RegisterStatusCB(StatusCallback, (void*)"I2S");

    mp3_ = new AudioGeneratorMP3();
    mp3_->RegisterMetadataCB(MetaDataCallback, (void*)"mp3");
    mp3_->RegisterStatusCB(StatusCallback, (void*)"mp3");
    mp3_->begin(buff_, out_);
}

InternetStream::~InternetStream(){
	delete file_;
	delete buff_;
	delete out_;
	delete mp3_;
}

bool InternetStream::Loop(void){
	return mp3_->loop();
}
