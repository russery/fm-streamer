/*
Implements web server to change fm streamer configuration, and
manages configuration in non-volatile memory.

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

#include "webserver.h"
#if defined(ESP32)
#include <AsyncTCP.h>
#include <WiFi.h>
#include <esp_spiffs.h>
#include <mdns.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncTCP.h>
#endif

namespace {
constexpr char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>FM Streamer</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="10">
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
      <input type="range" id="volume" name="volume" min="0" max="100" step="1", value="%VOL%"%VOLDISA% onchange="this.form.submit();">
      <input type="checkbox" id="autovol" name="autovol"%AUTOVOL% onchange="this.form.submit();"><label for="autovol">Auto</label><br>
    </form>
%UPTIME%
    </body>
</html>
)rawliteral";
}

String Webserver::WebpageProcessor_(const String &var) {
  if (var == "STATION-LIST") {
    char buff[512] = {0};
    uint startchar = 0;
    for (int i = 0; i < sizeof(StationList_) / sizeof(StationList_[0]); i++) {
      startchar += sprintf(
          &buff[startchar],
          "      <input type=\"radio\" id=\"station%d\" name=\"station\" "
          "value=\"%d\"%s onchange=\"this.form.submit();\">"
          "      <label for=\"station1\">%s</label><br>\r\n",
          i, i, (i == cfg_->GetStation()) ? " checked" : "",
          StationList_[i].Name);
    }
    return String(buff);
  } else if (var == "FREQ") {
    return String(cfg_->GetFreqMHz(), 2);
  } else if (var == "POW") {
    return String(cfg_->GetPower());
  } else if (var == "VOL") {
    return String(cfg_->GetVolume());
  } else if (var == "VOLDISA") {
    return (cfg_->GetAutoVolume()) ? " disabled" : "";
  } else if (var == "AUTOVOL") {
    return (cfg_->GetAutoVolume()) ? " checked" : "";
  } else if (var == "UPTIME") {
    char buff[128] = {0};
    unsigned long sec = millis() / 1000;
    sprintf(buff,
            "    <div>Uptime: %3d days %2d hrs %2d mins %2d sec</div>\r\n",
            (uint)(sec / 86400L), (uint)(sec / 3600L) % 24,
            (uint)(sec / 60L) % 60, (uint)sec % 60);
    return String(buff);
  }
  return String();
}

void Webserver::HandlePagePost_(AsyncWebServerRequest *request) {
  if (request->hasParam(F("station"), true))
    cfg_->SetStation(
        (uint)request->getParam(F("station"), true)->value().toInt());
  if (request->hasParam(F("freq"), true))
    cfg_->SetFreqMHz(request->getParam(F("freq"), true)->value().toFloat());
  if (request->hasParam(F("txpower"), true))
    cfg_->SetPower(request->getParam(F("txpower"), true)->value().toInt());
  if (request->hasParam(F("volume"), true))
    cfg_->SetVolume(
        (uint)request->getParam(F("volume"), true)->value().toInt());
  if (request->hasParam(F("autovol"), true)) {
    cfg_->SetAutoVolume(true); // param only present if true
  } else {
    cfg_->SetAutoVolume(false); // param only present if true
  }
  config_changed_ = true;
  cfg_->WriteToFlash();
  request->redirect("/");
}

bool Webserver::IsMdnsActive(void) { return mdns_active_; }

bool Webserver::IsConfigChanged(void) {
  if (config_changed_) {
    config_changed_ = false;
    return true;
  }
  return false;
}

Webserver::Stream_t Webserver::GetCurrentStream(void) {
  return StationList_[cfg_->GetStation()];
}

void Webserver::Start(void) {
#if defined(ESP32)
  // Register server responses:
  server_.on("/", [&](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html,
                    [&](const String &var) -> String {
                      return this->WebpageProcessor_(var);
                    });
  });
  server_.onNotFound([&](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html,
                    [&](const String &var) -> String {
                      return this->WebpageProcessor_(var);
                    });
  }); // Just direct everything to the same page
  server_.on("/post", HTTP_POST, [&](AsyncWebServerRequest *request) -> void {
    return this->HandlePagePost_(request);
  });

  // Start serving
  server_.begin();
#endif
}

void Webserver::StartMdns(void) {
// Start MDNS
#if defined(ESP32)
  mdns_init();
  mdns_hostname_set(MDNS_ADDRESS);
  mdns_active_ =
      (mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0) == ESP_OK);
#elif defined(ESP8266)
  mdns_active_ = MDNS.begin(MDNS_ADDRESS);
  MDNS.addService("http", "tcp", 80);
#endif
}

void Webserver::Loop(void) {
#if defined(ESP8266)
  MDNS.update();
#endif
}
