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
#include <ArduinoOTA.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

extern const char WIFI_SSID[];
extern const char WIFI_PASSWORD[];
extern const char OTA_UPDATE_PWD[];
constexpr char RDS_STATION_NAME[] = "FMSTREAM"; // 8 characters max

FmRadio fm_radio;
InternetStream stream(2048, &(fm_radio.i2s_output));
Config cfg(Webserver::NUM_STATIONS);
Webserver webserver(&cfg);

void (*resetFunc)(void) = 0;

// cppcheck-suppress unusedFunction
void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("\r\nFM Streamer Starting...\r\n\n");

  pinMode(LED_STREAMING_PIN, OUTPUT);
  digitalWrite(LED_STREAMING_PIN, LED_OFF);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  cfg.Start();
  webserver.Start();
  fm_radio.Start(RDS_STATION_NAME);

  ArduinoOTA.setHostname(Webserver::MDNS_ADDRESS);
  if (strlen(OTA_UPDATE_PWD) > 0)
    ArduinoOTA.setPassword(OTA_UPDATE_PWD);

  ArduinoOTA.onStart([]() { Serial.println("Start updating sketch"); })
      .onEnd([]() { Serial.println("\nFinished Updating"); })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });
}

void loop() {
  static enum LoopState {
    ST_WIFI_CONNECT,
    ST_STREAM_START,
    ST_STREAM_CONNECTING,
    ST_STREAMING
  } state = ST_WIFI_CONNECT;
  static unsigned long stream_start_time_ms = 0;

  switch (state) {
  case ST_WIFI_CONNECT:
    if (WiFi.status() == WL_CONNECTED) {
      state = ST_STREAM_START;
      webserver.StartMdns();
      ArduinoOTA.begin();
    }
    break;
  case ST_STREAM_START:
    // fm_radio.SetInputEnable(false); // Disable I2S input
    fm_radio.SetVolume(0); // Mute radio output while stream is starting up
    stream.OpenUrl(webserver.GetCurrentStream().URL);
    fm_radio.SetRdsText(webserver.GetCurrentStream().Name);
    digitalWrite(LED_STREAMING_PIN, LED_OFF);
    state = ST_STREAM_CONNECTING;
    stream_start_time_ms = millis();
    break;
  case ST_STREAM_CONNECTING:
    if (stream.Loop()) {
      state = ST_STREAMING;
      fm_radio.SetVolume(cfg.GetVolume()); // Allow radio to turn back on
      // fm_radio.SetInputEnable(true); // Enable I2S input
    } else if (millis() - stream_start_time_ms > 30000) {
      resetFunc(); // Timed out trying to connect, try resetting
    }
    break;
  case ST_STREAMING:
    static unsigned long last_autovol_ms = 0;
    if ((cfg.GetAutoVolume()) && (millis() - last_autovol_ms > 5000)) {
      cfg.SetVolume(fm_radio.GetVolume()); // Update value in config
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
    Serial.printf_P(
        (PGM_P) "\r\nFreq: %5.2fMHz  Power: %3d%%   Volume: %3d%%%s",
        float(fm_radio.GetFreq()) / 1000.0f, fm_radio.GetTxPower(),
        fm_radio.GetVolume(), (cfg.GetAutoVolume()) ? "(Auto)" : "");
  }

  webserver.Loop();
  if (webserver.IsConfigChanged()) {
    static uint previous_station = cfg.GetStation();
    uint curr_station = cfg.GetStation();
    if (curr_station != previous_station) {
      previous_station = curr_station; // User changed station
      fm_radio.SetVolume(0); // Mute radio output while we close out the stream
      stream.Flush();
      state = ST_STREAM_START;
    }
    if (!cfg.GetAutoVolume()) {
      fm_radio.SetVolume(cfg.GetVolume());
    }
    fm_radio.SetFreq(cfg.GetFreqkHz());
    fm_radio.SetTxPower(cfg.GetPower());
  }

  ArduinoOTA.handle();

  yield(); // Make sure WiFi can do its thing.
}
