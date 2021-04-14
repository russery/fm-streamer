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

extern "C" {
#include "user_interface.h"
}

#include <assert.h>
#include <Arduino.h>
#include <Adafruit_Si4713.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
#include "arduino_secrets.h"

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

const char *URL="http://streams.kqed.org/kqedradio";
//const char *URL="http://kvbstreams.dyndns.org:8000/wkvi-am";

const uint AUDIO_STREAM_BUFFER_BYTES = 10240;

AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;



///////// RADIO STUFF

#define _BV(n) (1 << n)
#define RESETPIN 12
#define FMSTATION 8810      // 10230 == 102.30 MHz
Adafruit_Si4713 radio = Adafruit_Si4713(RESETPIN);






// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
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

void StartStream() {
    delete file;
    delete buff;
    delete out;
    delete mp3;
    file = new AudioFileSourceICYStream(URL);
    file->RegisterMetadataCB(MDCallback, (void*)"ICY");
    buff = new AudioFileSourceBuffer(file, AUDIO_STREAM_BUFFER_BYTES);
    buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
    out = new AudioOutputI2S();
    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
    mp3->begin(buff, out);
}

void setup()
{
    system_update_cpu_freq(160);
    Serial.begin(115200);
    delay(1000);

    // Check clock freq:
    int clk_freq = ESP.getCpuFreqMHz();
    assert(clk_freq == 160); // Clock Frequency must be 160MHz - make sure this is configured in Arduino IDE or CLI

    Serial.println("\r\nConnecting to WiFi");
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    // Try forever
    while (WiFi.status() != WL_CONNECTED) {
        Serial.printf("...");
        delay(1000);
    }
    Serial.printf("\r\nConnected to WiFi network: \"%s\"\r\n", ssid);

    audioLogger = &Serial;
    StartStream();




    if(!radio.begin()) {  // begin with address 0x63 (CS high default)
        Serial.println("Ahhhh crap couldn't find the radio!!");
        while(true);
    }

    Serial.print("\nSet TX power");
    radio.setTXpower(115);  // dBuV, 88-115 max

    Serial.print("\nTuning into ");
    Serial.print(FMSTATION/100);
    Serial.print('.');
    Serial.println(FMSTATION % 100);
    radio.tuneFM(FMSTATION); // 102.3 mhz

    // This will tell you the status in case you want to read it from the chip
    radio.readTuneStatus();
    Serial.print("\tCurr freq: ");
    Serial.println(radio.currFreq);
    Serial.print("\tCurr freqdBuV:");
    Serial.println(radio.currdBuV);
    Serial.print("\tCurr ANTcap:");
    Serial.println(radio.currAntCap);

    // begin the RDS/RDBS transmission
    radio.beginRDS();
    radio.setRDSstation("AdaRadio");
    radio.setRDSbuffer( "Adafruit g0th Radio!");

    Serial.println("RDS on!");
}


void loop()
{
    static int lastms = 0;

    if (mp3->isRunning()) {
        if (millis()-lastms > 1000) {
            lastms = millis();
            Serial.printf("Running for %d ms...\r\n", lastms);
            Serial.flush();


            radio.readASQ();
            Serial.print("\tCurr ASQ: 0x");
            Serial.println(radio.currASQ, HEX);
            Serial.print("\tCurr InLevel:");
            Serial.println(radio.currInLevel);
        }
        delay(10);
        if (!mp3->loop()) mp3->stop();
    } else {
        delay(1000);
        Serial.println("\r\nStreaming crapped out, starting over again...");
        StartStream();
    }
}
