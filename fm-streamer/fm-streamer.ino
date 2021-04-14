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

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

const char *URL="http://streams.kqed.org/kqedradio";

///////// RADIO STUFF
#define RESETPIN 12
#define FMSTATION 8810      // 10230 == 102.30 MHz
Adafruit_Si4713 radio = Adafruit_Si4713(RESETPIN);

InternetStream *stream;

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

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Try forever
    while (WiFi.status() != WL_CONNECTED) {
        Serial.printf("...");
        delay(1000);
    }
    Serial.printf("\r\nConnected to WiFi network: \"%s\"\r\n", WIFI_SSID);

    // audioLogger = &Serial;

    stream = new InternetStream(URL);


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

    if (stream->DoStream()) {
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
    } else {
        delay(1000);
        Serial.println("\r\nStreaming crapped out, starting over again...");
        delete stream;
        stream = new InternetStream(URL);
    }
}
