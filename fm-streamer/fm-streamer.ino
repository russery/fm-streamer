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

#include "arduino_secrets.h"
#include "bsp.h"
#include "fm_radio.h"
#include "internet_stream.h"
#include "webserver.h"
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;
const char *RDS_STATION_NAME = "FMSTREAM"; // 8 characters max

FmRadio fm_radio;
InternetStream stream(4096, &(fm_radio.i2s_output));
Webserver webserver;

void (*resetFunc)(void) = 0;

// cppcheck-suppress unusedFunction
void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("\r\nFM Streamer Starting...\r\n\n");

  pinMode(LED_STREAMING_PIN, OUTPUT);
  digitalWrite(LED_STREAMING_PIN, LED_OFF);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  webserver.Start();

  fm_radio.Start(RDS_STATION_NAME);
}

void loop() {
  static enum LoopState {
    ST_WIFI_CONNECT,
    ST_STREAM_START,
    ST_STREAM_CONNECTING,
    ST_STREAMING
  } state = ST_WIFI_CONNECT;
  static unsigned long stream_start_time_ms = 0;
  static bool use_auto_volume = true;

  switch (state) {
  case ST_WIFI_CONNECT:
    if (WiFi.status() == WL_CONNECTED) {
      state = ST_STREAM_START;
      webserver.StartMdns();
    }
    break;
  case ST_STREAM_START:
    stream.OpenUrl(webserver.GetCurrentStream().URL);
    fm_radio.SetRdsText(webserver.GetCurrentStream().Name);
    digitalWrite(LED_STREAMING_PIN, LED_OFF);
    state = ST_STREAM_CONNECTING;
    stream_start_time_ms = millis();
    // Mute radio output while stream is starting up
    fm_radio.SetVolume(0);
    break;
  case ST_STREAM_CONNECTING:
    if (stream.Loop()) {
      state = ST_STREAMING;
      // Now that stream is up, unmute radio:
      fm_radio.SetVolume(webserver.cfg.GetVolume());
    } else if (millis() - stream_start_time_ms > 30000) {
      // Timed out trying to connect, try resetting
      resetFunc();
    }
    break;
  case ST_STREAMING:
    static unsigned long last_autovol_ms = 0;
    if ((use_auto_volume) && (millis() - last_autovol_ms > 5000)) {
      last_autovol_ms = millis();
      fm_radio.DoAutoSetVolume();
    }
    digitalWrite(LED_STREAMING_PIN, LED_ON);
    if (!stream.Loop()) {
      state = ST_STREAM_START;
    }
    break;
  }

  static unsigned long last_print_ms = 0;
  if (millis() - last_print_ms > 1000) {
    last_print_ms = millis();
    Serial.print(F("\r\n\n\n"));
    Serial.print(F("\r\n-----------------------------------"));
    Serial.print(F("\r\n        fm-streamer status"));
    Serial.print(F("\r\n-----------------------------------"));
    unsigned long sec = millis() / 1000;
    Serial.printf_P((PGM_P) "\r\nUptime: %d days %d hrs %d mins %d sec",
                    sec / 86400, (sec / 3600) % 24, (sec / 60) % 60, sec % 60);
    Serial.printf_P((PGM_P) "\r\nSSID: \"%s\"", WiFi.SSID().c_str());
    Serial.printf_P((PGM_P) "\r\nIP Address: %s DNS: %s%s",
                    WiFi.localIP().toString().c_str(),
                    (webserver.IsMdnsActive()) ? webserver.MDNS_ADDRESS
                                               : (PGM_P)("<inactive>"),
                    (webserver.IsMdnsActive()) ? (PGM_P)(".local") : "");
    Serial.print(F("\r\nStatus: "));
    switch (state) {
    case ST_WIFI_CONNECT:
      Serial.print(F("Connecting to Wifi"));
      break;
    case ST_STREAM_START:
      Serial.print(F("Starting internet stream"));
      break;
    case ST_STREAM_CONNECTING:
      Serial.print(F("Connecting to internet stream"));
      break;
    case ST_STREAMING:
      Serial.print(F("Streaming from internet"));
      break;
    }
    Serial.printf_P((PGM_P) "\r\nStation: %s",
                    webserver.GetCurrentStream().Name);
    Serial.printf_P((PGM_P) "\r\nFreq: %5.2fMHz  Power: %3d%%   Volume: %3d%%",
                    float(fm_radio.GetFreq()) / 1000.0f, fm_radio.GetTxPower(),
                    fm_radio.GetVolume());
  }

  webserver.Loop();
  if (webserver.IsConfigChanged()) {
    static uint previous_station = webserver.cfg.GetStation();
    uint curr_station = webserver.cfg.GetStation();
    if (curr_station != previous_station) {
      // User changed station
      previous_station = curr_station;
      state = ST_STREAM_START;
    }
    // If volume has been changed, disable autovolume
    uint volume = webserver.cfg.GetVolume();
    if (fm_radio.GetVolume() != volume) {
      fm_radio.SetVolume(volume);
      use_auto_volume = false;
    }
    fm_radio.SetFreq(webserver.cfg.GetFreqkHz());
    fm_radio.SetTxPower(webserver.cfg.GetPower());
  }

  yield(); // Make sure WiFi can do its thing.
}
