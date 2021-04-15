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


#include <assert.h>
#include <Arduino.h>
#include <Adafruit_Si4713.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "arduino_secrets.h"
#include "internet_stream.h"
#include "webpage.h"

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

const char *URL="http://streams.kqed.org/kqedradio";

///////// RADIO STUFF
#define RESETPIN 12
#define FMSTATION 8810      // 10230 == 102.30 MHz
Adafruit_Si4713 radio = Adafruit_Si4713(RESETPIN);

InternetStream *stream;
WebPage webpage = WebPage();

enum LoopState {ST_WIFI_CONNECT, ST_STREAM_START, ST_STREAM_CONNECTING, ST_STREAMING} state_ = ST_WIFI_CONNECT;


void PrintStatus_(void) {
    Serial.printf("\r\n\n\n");
    //Serial.printf("\033[2J");
    Serial.printf("\r\n-----------------------------------");
    Serial.printf("\r\n        fm-streamer status");
    Serial.printf("\r\n-----------------------------------");
    uint sec = millis() / 1000;
    Serial.printf("\r\nUptime: %d days %d hrs %d mins %d sec", sec / 86400, (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
    Serial.printf("\r\nSSID: \"%s\"", WiFi.SSID().c_str());
    Serial.printf("\r\nIP Address: %s", WiFi.localIP().toString().c_str());
    Serial.printf("\r\nStatus: ");
    switch (state_){
        case ST_WIFI_CONNECT: Serial.printf("Connecting to Wifi"); break;
        case ST_STREAM_START: Serial.printf("Starting internet stream"); break;
        case ST_STREAM_CONNECTING: Serial.printf("Connecting to internet stream"); break;
        case ST_STREAMING: Serial.printf("Streaming from internet");break;
    }
}


void setup() {
    Serial.begin(115200);

    // Check clock freq:
    assert(ESP.getCpuFreqMHz() == 160); // Clock Frequency must be 160MHz - make sure this is configured in Arduino IDE or CLI

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    //Start up the FM Radio
    bool radio_started = radio.begin();
    assert(radio_started);

    //Serial.print("\nSet TX power");
    radio.setTXpower(115);  // dBuV, 88-115 max

    // Serial.print("\nTuning into ");
    // Serial.print(FMSTATION/100);
    // Serial.print('.');
    // Serial.println(FMSTATION % 100);
    radio.tuneFM(FMSTATION); // 102.3 mhz

    // This will tell you the status in case you want to read it from the chip
    // radio.readTuneStatus();
    // Serial.print("\tCurr freq: ");
    // Serial.println(radio.currFreq);
    // Serial.print("\tCurr freqdBuV:");
    // Serial.println(radio.currdBuV);
    // Serial.print("\tCurr ANTcap:");
    // Serial.println(radio.currAntCap);

    // // begin the RDS/RDBS transmission
    // radio.beginRDS();
    // radio.setRDSstation("AdaRadio");
    // radio.setRDSbuffer( "Adafruit g0th Radio!");
    webpage.Start();
}


void loop() {
    static int last_print_ms = 0;
    bool stream_is_running;

    switch (state_){
        case ST_WIFI_CONNECT:
            if(WiFi.status() == WL_CONNECTED){
                state_ = ST_STREAM_START;
            }
            break;
        case ST_STREAM_START:
            stream = new InternetStream(URL, 10240);
            state_ = ST_STREAM_CONNECTING;
            break;
        case ST_STREAM_CONNECTING:
            stream_is_running = stream->Loop();
            if (stream_is_running){
                state_ = ST_STREAMING;
            }
        case ST_STREAMING:
            stream_is_running = stream->Loop();
            if (!stream_is_running){
                delete stream;
                state_ = ST_STREAM_START;
            }
            break;
    }

    if (state_ != ST_WIFI_CONNECT) {
        webpage.Loop();
    }

    if(millis()-last_print_ms > 500) {
        last_print_ms = millis();
        PrintStatus_();
    }
    delay(10);
}
