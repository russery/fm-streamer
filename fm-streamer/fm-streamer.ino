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
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include "arduino_secrets.h"
#include "internet_stream.h"
#include "fm_radio.h"

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
    <form method="post" action="/post">
%STATION-LIST%<br>
      <label for="freq">Radio Frequency (MHz):</label>
      <input type="number" id="freq" name="freq" min="76.0" max="108.0" step="0.1" value="%FREQ%" onchange="this.form.submit();"><br>
      <label for="txpower">Transmit Power</label>
      <input type="range" id="txpower" name="txpower" min="0" max="100" step="1", value="%POW%" onchange="this.form.submit();"><br>
      <label for="txpower">Audio Volume</label>
      <input type="range" id="volume" name="volume" min="0" max="100" step="1", value="%VOL%" onchange="this.form.submit();"><br>
    </form>
    </body>
</html>
)rawliteral";

extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
const char* MDNS_ADDRESS PROGMEM = "fm-streamer";
const char* CONFIG_FILE_NAME PROGMEM = "/fm_streamer_config.txt";
const uint DFT_STATION PROGMEM = 0;
const uint DFT_FREQ PROGMEM = 88100;
const uint DFT_POWER PROGMEM = 90;
const uint DFT_VOLUME PROGMEM = 80;

typedef struct { const char URL[128]; const char Name[32]; } Stream_t;
const Stream_t StationList[] PROGMEM = {
    {{.URL="http://streams.kqed.org/kqedradio"}, {.Name="KQED San Francisco"}},
    {{.URL="http://stream.revma.ihrhls.com/zc2804"}, {.Name="KBCO Denver"}},
    {{.URL="https://kunrstream.com:8443/live"}, {.Name="KUNR Reno"}}
};
const uint NUM_STATIONS PROGMEM = sizeof(StationList) / sizeof(Stream_t);
uint curr_station = 0;

//AsyncWebServer webserver(80);
FmRadio fm_radio;

InternetStream stream = InternetStream(1024, &(fm_radio.i2s_input));

String WebpageProcessor(const String& var){
    if (var == "STATION-LIST") {
        char list[512] = {0};
        uint startchar = 0;
        for(int i=0; i<sizeof(StationList)/sizeof(StationList[0]); i++) {
            startchar += sprintf(&list[startchar],
                "      <input type=\"radio\" id=\"station%d\" name=\"station\" value=\"%d\"%s onchange=\"this.form.submit();\">" \
                "      <label for=\"station1\">%s</label><br>\r\n",
                i, i, (i == curr_station) ? " checked" : "", StationList[i].Name);
        }
        return String(list);
    } else if (var == "FREQ") {
        return String(float(fm_radio.GetFreq()) / 1000.0f, 2);
    } else if (var == "POW") {
        return String(fm_radio.GetTxPower());
    } else if (var == "VOL") {
        return String(fm_radio.GetVolume());
    }
    return String();
}

void ServePage(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, WebpageProcessor);
}

void HandlePagePost(AsyncWebServerRequest *request) {
    if (request->hasParam(F("station")), true) {
        curr_station = (uint)request->getParam(F("station"), true)->value().toInt();
        assert(curr_station < NUM_STATIONS);
    }
    if (request->hasParam(F("freq")), true) {
        // Setting new freq on radio here results in a fault - not sure why. So we just store it and set it later in loop().
        uint new_freq = (uint)(request->getParam(F("freq"), true)->value().toFloat() * 1000);
        fm_radio.SetFreq(new_freq);
    }
    if (request->hasParam(F("txpower")), true) {
        uint txpower = (uint)request->getParam(F("txpower"), true)->value().toInt();
        fm_radio.SetTxPower(txpower);
    }
    if (request->hasParam(F("volume")), true) {
        uint volume = (uint)request->getParam(F("volume"), true)->value().toInt();
        fm_radio.SetVolume(volume);
    }
    WriteConfig(curr_station, fm_radio.GetFreq(), fm_radio.GetTxPower(), fm_radio.GetVolume());
    request->redirect("/");
}

void WriteConfig(uint station, uint freq, uint power, uint vol){
    if (SPIFFS.exists(CONFIG_FILE_NAME)) {
        SPIFFS.remove(CONFIG_FILE_NAME);
    }
    File configfile = SPIFFS.open(CONFIG_FILE_NAME, "w");
    if (!configfile) {
        Serial.println("File open failed...");
    } else {
      char cfg_str[64] = {0};
      sprintf(cfg_str, "%d %d %d %d ", station, freq, power, vol);
      configfile.print(cfg_str);
      configfile.close();
    }
}

String ReadConfigVal(File *configfile){
    String val = "";
    char r = (char)configfile->read();
    while(r != ' ') {
        val += r;
        r = (char)configfile->read();
    }
    return val;
}

void setup() {
    delay(500);
    Serial.begin(115200);
    Serial.println("FM Streamer Starting...\r\n\n");

    // Check clock freq:
    assert(ESP.getCpuFreqMHz() == 160); // Clock Frequency must be 160MHz - make sure this is configured in Arduino IDE or CLI

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // webserver.on("/", ServePage);
    // webserver.onNotFound(ServePage); // Just direct everything to the same page
    // webserver.on("/post", HTTP_POST, HandlePagePost);

    fm_radio.Start();

    SPIFFS.begin();
    if (!SPIFFS.exists(CONFIG_FILE_NAME)){
        // Initialize config with default values
        Serial.println("No config found... writing defaults.");
        WriteConfig(DFT_STATION, DFT_FREQ, DFT_POWER, DFT_VOLUME);
    } else {
        File configfile = SPIFFS.open(CONFIG_FILE_NAME, "r");
        curr_station = ReadConfigVal(&configfile).toInt();
        fm_radio.SetFreq(ReadConfigVal(&configfile).toInt());
        fm_radio.SetTxPower(ReadConfigVal(&configfile).toInt());
        fm_radio.SetVolume(ReadConfigVal(&configfile).toInt());
        configfile.close();
    }
}


void loop() {
    static enum LoopState {
        ST_WIFI_CONNECT, ST_STREAM_START, ST_STREAM_CONNECTING, ST_STREAMING
    } state_ = ST_WIFI_CONNECT;
    static uint last_print_ms = 0;
    static bool mdns_active = false;
    bool stream_is_running;

    switch (state_){
        case ST_WIFI_CONNECT:
            if(WiFi.status() == WL_CONNECTED){
                //webserver.begin();
                mdns_active = MDNS.begin(MDNS_ADDRESS);
                MDNS.addService("http", "tcp", 80);
                state_ = ST_STREAM_START;
            }
            break;
        case ST_STREAM_START:
            MDNS.update();
            stream.OpenUrl(StationList[curr_station].URL);
            state_ = ST_STREAM_CONNECTING;
            break;
        case ST_STREAM_CONNECTING:
            MDNS.update();
           stream_is_running = stream.Loop();
           if (stream_is_running){
               state_ = ST_STREAMING;
           }
            break;
        case ST_STREAMING:
            MDNS.update();
           stream_is_running = stream.Loop();
           if (!stream_is_running){
               state_ = ST_STREAM_START;
           }
            break;
    }

    if(millis()-last_print_ms > 1000) {
        last_print_ms = millis();
            Serial.print(F("\r\n\n\n"));
            Serial.print(F("\r\n-----------------------------------"));
            Serial.print(F("\r\n        fm-streamer status"));
            Serial.print(F("\r\n-----------------------------------"));
            uint sec = millis() / 1000;
            Serial.printf_P((PGM_P)"\r\nUptime: %d days %d hrs %d mins %d sec", sec / 86400, (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
            Serial.printf_P((PGM_P)"\r\nSSID: \"%s\"", WiFi.SSID().c_str());
            Serial.printf_P((PGM_P)"\r\nIP Address: %s DNS: %s%s", WiFi.localIP().toString().c_str(),
                (mdns_active) ? MDNS_ADDRESS : (PGM_P)("<inactive>"), (mdns_active) ? (PGM_P)(".local") : "");
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
}
