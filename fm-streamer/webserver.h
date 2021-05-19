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
#ifndef __WEBSERVER_H
#define __WEBSERVER_H

#include "config.h"
#include <ESPAsyncWebServer.h>

class Webserver {
public:
  typedef struct {
    const char URL[128];
    const char Name[32];
  } Stream_t;

  static constexpr char *MDNS_ADDRESS PROGMEM = "fm-streamer";
  static constexpr uint NUM_STATIONS PROGMEM = 3;

  explicit Webserver(Config *cfg, uint port = 80) : cfg_{cfg}, server_(port){};
  void Start(void);
  void StartMdns(void);
  void Loop(void);
  bool IsConfigChanged(void);
  bool IsMdnsActive(void);
  Stream_t GetCurrentStream(void);

private:
  const Stream_t StationList_[NUM_STATIONS] = {
      {.URL = "http://streams.kqed.org/kqedradio",
       .Name = "KQED San Francisco"},
      {.URL = "https://kunrstream.com:8000/live", .Name = "KUNR Reno"},
      {.URL = "http://live-mp3-128.kexp.org/",
       .Name = "KEXP Seattle"}};
  Config *cfg_;
  AsyncWebServer server_;
  bool config_changed_ = true;
  bool mdns_active_ = false;

  // cppcheck-suppress unusedPrivateFunction
  String WebpageProcessor_(const String &var);
  // cppcheck-suppress unusedPrivateFunction
  void HandlePagePost_(AsyncWebServerRequest *request);
};

#endif //__WEBSERVER_H