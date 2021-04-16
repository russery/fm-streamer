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
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include "arduino_secrets.h"
#include "internet_stream.h"
#include "fm_radio.h"

// TODO - preprocessor to build this automatically and eliminate hard-coded values?
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>FM Streamer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
  </style>
  </head>
  <body>
    <h2>FM Streamer</h2>
    <form action="/update">
      <input type="radio" id="station1" name="station" value="1"%ST_1_C%>
      <label for="station1">KQED</label><br>
      <input type="radio" id="station2" name="station" value="2"%ST_2_C%>
      <label for="station2">Radio RST</label><br>
      <input type="radio" id="station3" name="station" value="3"%ST_3_C%>
      <label for="station3">NJOY</label><br>
      <br>
      <label for="freq">Radio Frequency (MHz):</label>
      <input type="number" id="freq" name="freq" min="88.0" max="108.0" step="0.1" value="%FREQ%"><br>
      <label for="txpower">Transmit Power</label>
      <input type="range" id="txpower" name="txpower" min="0" max="100" step="1", value="%POW%"><br>
      <label for="txpower">Audio Volume</label>
      <input type="range" id="volume" name="volume" min="0" max="100" step="1", value="%VOL%"><br>
      <br>
      <input type="submit" value="Update">
    </form>
  </body>
</html>
)rawliteral";

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

typedef struct { const char URL[128]; const char Name[32]; } Stream_t;
const Stream_t StationList[] PROGMEM = {
    {{.URL="http://streams.kqed.org/kqedradio"}, {.Name="KQED"}},
    {{.URL="http://mms.hoerradar.de:8000/rst128k"}, {.Name="Radio RST"}},
    {{.URL="http://ndr-edge-206c.fra-lg.cdn.addradio.net/ndr/njoy/live/mp3/128/stream.mp3"}, {.Name="NJOY"}}
};
const uint NUM_STATIONS = sizeof(StationList) / sizeof(Stream_t);
uint curr_station = 0;

AsyncWebServer webserver = AsyncWebServer(80);
FmRadio fm_radio = FmRadio();
InternetStream stream = InternetStream(4096, &fm_radio.i2s_input);

String webpage_processor(const String& var){
    if (var == "ST_1_C") {
        return (0 == curr_station) ? F(" checked") : String();
    } else if (var == "ST_2_C") {
        return (1 == curr_station) ? F(" checked") : String();
    } else if (var == "ST_3_C") {
        return (2 == curr_station) ? F(" checked") : String();
    } else if (var == "FREQ") {
        return String(float(fm_radio.GetFreq()) / 1000.0f, 2);
    } else if (var == "POW") {
        return String(fm_radio.GetTxPower());
    } else if (var == "VOL") {
        return String(fm_radio.GetVolume());
    }
    return String();
}

void setup() {
    Serial.begin(115200);

    // Check clock freq:
    assert(ESP.getCpuFreqMHz() == 160); // Clock Frequency must be 160MHz - make sure this is configured in Arduino IDE or CLI

    // Start connecting to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);


    // ----------------------------------------
    // Server setup
    // ----------------------------------------
    // Set route for index.html:
    webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, F("text/html"), index_html, webpage_processor);
    });

    // Set route for "/update" GET call:
    webserver.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String inputMessage;
        String inputParam;
        // GET input1 value on <ESP_IP>/update?state=<inputMessage>
        if (request->hasParam(F("station"))) {
            uint station_ind = (uint)request->getParam(F("station"))->value().toInt();
            curr_station = (uint)request->getParam(F("txpower"))->value().toInt();
            // TODO change stream URL
        } else if (request->hasParam(F("freq"))) {
            uint freq_khz = (uint)(request->getParam(F("freq"))->value().toFloat() * 1000);
            fm_radio.SetFreq(freq_khz);
        } else if (request->hasParam(F("txpower"))) {
            uint txpower = (uint)request->getParam(F("txpower"))->value().toInt();
            fm_radio.SetTxPower(txpower);
        } else if (request->hasParam(F("volume"))) {
            uint volume = (uint)request->getParam(F("volume"))->value().toInt();
            fm_radio.SetVolume(volume);
        } else {
            inputMessage = F("No message sent");
            inputParam = F("none");
        }
        request->send_P(200, "text/html", index_html, webpage_processor);
    });
    webserver.begin();

    fm_radio.Start();
    fm_radio.SetTxPower(100);
    fm_radio.SetVolume(80);
    fm_radio.SetFreq(88100);
}


void loop() {
    static enum LoopState {
        ST_WIFI_CONNECT, ST_STREAM_START, ST_STREAM_CONNECTING, ST_STREAMING
    } state_ = ST_WIFI_CONNECT;
    static uint last_print_ms = 0;
    bool stream_is_running;

    switch (state_){
        case ST_WIFI_CONNECT:
            if(WiFi.status() == WL_CONNECTED){
                state_ = ST_STREAM_START;
            }
            break;
        case ST_STREAM_START:
            stream.OpenUrl(StationList[curr_station].URL);
            state_ = ST_STREAM_CONNECTING;
            break;
        case ST_STREAM_CONNECTING:
            stream_is_running = stream.Loop();
            if (stream_is_running){
                state_ = ST_STREAMING;
            }
        case ST_STREAMING:
            stream_is_running = stream.Loop();
            if (!stream_is_running){
                state_ = ST_STREAM_START;
            }
            break;
    }

    if(millis()-last_print_ms > 500) {
        last_print_ms = millis();
            Serial.print(F("\r\n\n\n"));
            Serial.print(F("\033[2J"));
            Serial.print(F("\r\n-----------------------------------"));
            Serial.print(F("\r\n        fm-streamer status"));
            Serial.print(F("\r\n-----------------------------------"));
            uint sec = millis() / 1000;
            Serial.printf_P((PGM_P)"\r\nUptime: %d days %d hrs %d mins %d sec", sec / 86400, (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
            Serial.printf_P((PGM_P)"\r\nSSID: \"%s\"", WiFi.SSID().c_str());
            Serial.printf_P((PGM_P)"\r\nIP Address: %s", WiFi.localIP().toString().c_str());
            Serial.print(F("\r\nStatus: "));
            switch (state_){
                case ST_WIFI_CONNECT: Serial.print(F("Connecting to Wifi")); break;
                case ST_STREAM_START: Serial.print(F("Starting internet stream")); break;
                case ST_STREAM_CONNECTING: Serial.print(F("Connecting to internet stream")); break;
                case ST_STREAMING: Serial.print(F("Streaming from internet"));break;
            }
            Serial.printf_P((PGM_P)"\r\nStation: %s", StationList[curr_station].Name);
            Serial.printf_P((PGM_P)"\r\nFreq: %5.2fMHz  Power: %3d%%   Volume: %3d%%",
                float(fm_radio.GetFreq()) / 1000.0f, fm_radio.GetTxPower(), fm_radio.GetVolume());
    }
    delay(10);
}
