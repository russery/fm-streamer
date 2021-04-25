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
#include <ESPAsyncWebServer.h>

const char index_html[] PROGMEM = R"rawliteral(
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
      <input type="range" id="volume" name="volume" min="0" max="100" step="1", value="%VOL%" onchange="this.form.submit();"><br>
    </form>
%UPTIME%
    </body>
</html>
)rawliteral";

extern const char *WIFI_SSID;
extern const char *WIFI_PASSWORD;
const char *MDNS_ADDRESS PROGMEM = "fm-streamer";
const char *RDS_STATION_NAME = "FMSTREAM"; // 8 characters max
const char *CONFIG_FILE_NAME PROGMEM = "/cfg/fm_streamer_config.txt";
const uint DFT_STATION PROGMEM = 0;
const uint DFT_FREQ PROGMEM = 88100;
const uint DFT_POWER PROGMEM = 90;
const uint DFT_VOLUME PROGMEM = 15;

typedef struct {
  const char URL[128];
  const char Name[32];
} Stream_t;
const Stream_t StationList[] PROGMEM = {
    {{.URL = "http://streams.kqed.org/kqedradio"},
     {.Name = "KQED San Francisco"}},
    {{.URL = "http://ais-edge16-jbmedia-nyc04.cdnstream.com/1power"},
     {.Name = "PowerHitz"}},
    {{.URL = "https://kunrstream.com:8000/live"}, {.Name = "KUNR Reno"}}};
const uint NUM_STATIONS PROGMEM = sizeof(StationList) / sizeof(Stream_t);
uint curr_station = 0;
FmRadio fm_radio;
InternetStream stream = InternetStream(4096, &(fm_radio.i2s_output));
bool UseAutoVolume = true;

#if defined(ESP32)
AsyncWebServer webserver(80);
#endif

void (*resetFunc)(void) = 0;

#if defined(ESP32)
String WebpageProcessor(const String &var) {
  if (var == "STATION-LIST") {
    char buff[512] = {0};
    uint startchar = 0;
    for (int i = 0; i < sizeof(StationList) / sizeof(StationList[0]); i++) {
      startchar += sprintf(
          &buff[startchar],
          "      <input type=\"radio\" id=\"station%d\" name=\"station\" "
          "value=\"%d\"%s onchange=\"this.form.submit();\">"
          "      <label for=\"station1\">%s</label><br>\r\n",
          i, i, (i == curr_station) ? " checked" : "", StationList[i].Name);
    }
    return String(buff);
  } else if (var == "FREQ") {
    return String(float(fm_radio.GetFreq()) / 1000.0f, 2);
  } else if (var == "POW") {
    return String(fm_radio.GetTxPower());
  } else if (var == "VOL") {
    return String(fm_radio.GetVolume());
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

void HandlePagePost(AsyncWebServerRequest *request) {
  if (request->hasParam(F("station")), true) {
    curr_station = (uint)request->getParam(F("station"), true)->value().toInt();
    assert(curr_station < NUM_STATIONS);
  }
  if (request->hasParam(F("freq")), true) {
    uint new_freq =
        (uint)(request->getParam(F("freq"), true)->value().toFloat() * 1000);
    fm_radio.SetFreq(new_freq);
  }
  if (request->hasParam(F("txpower")), true) {
    uint txpower = (uint)request->getParam(F("txpower"), true)->value().toInt();
    fm_radio.SetTxPower(txpower);
  }
  if (request->hasParam(F("volume")), true) {
    uint volume = (uint)request->getParam(F("volume"), true)->value().toInt();
    if (fm_radio.GetVolume() != volume) {
      fm_radio.SetVolume(volume);
      UseAutoVolume = false;
    }
  }
  WriteConfig(curr_station, fm_radio.GetFreq(), fm_radio.GetTxPower(),
              fm_radio.GetVolume());
  request->redirect("/");
}
#endif // ESP32

void WriteConfig(uint station, uint freq, uint power, uint vol) {
#if defined(ESP32)
  struct stat st;
  if (stat(CONFIG_FILE_NAME, &st) == 0) {
    unlink(CONFIG_FILE_NAME);
  }
  FILE *configfile = fopen(CONFIG_FILE_NAME, "w");
  if (!configfile) {
    Serial.println("File open failed");
  } else {
    fprintf(configfile, "%d %d %d %d ", station, freq, power, vol);
    fclose(configfile);
  }
#elif defined(ESP8266)
  if (SPIFFS.exists(CONFIG_FILE_NAME)) {
    SPIFFS.remove(CONFIG_FILE_NAME);
  }
  File configfile = SPIFFS.open(CONFIG_FILE_NAME, "w");
  if (!configfile) {
    Serial.println("File open failed");
  } else {
    configfile.printf("%d %d %d %d ", station, freq, power, vol);
    configfile.close();
  }
#endif
}

#if defined(ESP32)
String ReadConfigVal(FILE *configfile) {
  String val = "";
  val.reserve(16);
  char r[2];
  fgets(r, 2, configfile);
  while (r[0] != ' ') {
    val += r[0];
    fgets(r, 2, configfile);
  }
  return val;
}
#elif defined(ESP8266)
String ReadConfigVal(File *configfile) {
  String val = "";
  val.reserve(16);
  char r = (char)configfile->read();
  while (r != ' ') {
    val += r;
    r = (char)configfile->read();
  }
  return val;
}
#endif

// cppcheck-suppress unusedFunction
void setup() {
  delay(500);
  Serial.begin(115200);
  Serial.println("\r\nFM Streamer Starting...\r\n\n");

  pinMode(LED_STREAMING_PIN, OUTPUT);
  digitalWrite(LED_STREAMING_PIN, LED_OFF);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

#if defined(ESP32)
  webserver.on("/", [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, WebpageProcessor);
  });
  webserver.onNotFound([](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, WebpageProcessor);
  }); // Just direct everything to the same page
  webserver.on("/post", HTTP_POST, HandlePagePost);
#endif

  fm_radio.Start(RDS_STATION_NAME);

#if defined(ESP32)
  esp_vfs_spiffs_conf_t conf = {.base_path = "/cfg",
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = true};
  esp_vfs_spiffs_register(&conf);
  struct stat st;
  if (stat(CONFIG_FILE_NAME, &st) != 0) {
    // Config doesn't exist, so initialize with default values
    Serial.println("No config found... writing defaults.");
    WriteConfig(DFT_STATION, DFT_FREQ, DFT_POWER, DFT_VOLUME);
  }
  FILE *configfile = fopen(CONFIG_FILE_NAME, "r");
  curr_station = ReadConfigVal(configfile).toInt();
  fm_radio.SetFreq(ReadConfigVal(configfile).toInt());
  fm_radio.SetTxPower(ReadConfigVal(configfile).toInt());
  fm_radio.SetVolume(ReadConfigVal(configfile).toInt());
  fclose(configfile);
#elif defined(ESP8266)
  SPIFFS.begin();
  if (!SPIFFS.exists(CONFIG_FILE_NAME)) {
    // Config doesn't exist, so initialize with default values
    Serial.println("No config found... writing defaults.");
    WriteConfig(DFT_STATION, DFT_FREQ, DFT_POWER, DFT_VOLUME);
  }
  File configfile = SPIFFS.open(CONFIG_FILE_NAME, "r");
  curr_station = ReadConfigVal(&configfile).toInt();
  fm_radio.SetFreq(ReadConfigVal(&configfile).toInt());
  fm_radio.SetTxPower(ReadConfigVal(&configfile).toInt());
  fm_radio.SetVolume(ReadConfigVal(&configfile).toInt());
  configfile.close();
#endif
}

void loop() {
  static enum LoopState {
    ST_WIFI_CONNECT,
    ST_STREAM_START,
    ST_STREAM_CONNECTING,
    ST_STREAMING
  } state = ST_WIFI_CONNECT;
  static bool mdns_active = false;
  static unsigned long stream_start_time_ms = 0;

  switch (state) {
  case ST_WIFI_CONNECT:
    if (WiFi.status() == WL_CONNECTED) {
#if defined(ESP32)
      webserver.begin();
#endif

#if defined(ESP32)
      mdns_init();
      mdns_hostname_set(MDNS_ADDRESS);
      mdns_active =
          (mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0) == ESP_OK);
#elif defined(ESP8266)
      mdns_active = MDNS.begin(MDNS_ADDRESS);
      MDNS.addService("http", "tcp", 80);
#endif
      state = ST_STREAM_START;
    }
    break;
  case ST_STREAM_START:
#if defined(ESP8266)
    MDNS.update();
#endif
    stream.OpenUrl(StationList[curr_station].URL);
    fm_radio.SetRdsText(StationList[curr_station].Name);
    digitalWrite(LED_STREAMING_PIN, LED_OFF);
    state = ST_STREAM_CONNECTING;
    stream_start_time_ms = millis();
    break;
  case ST_STREAM_CONNECTING:
#if defined(ESP8266)
    MDNS.update();
#endif
    if (stream.Loop()) {
      state = ST_STREAMING;
    } else if (millis() - stream_start_time_ms > 30000) {
      // Timed out trying to connect, try resetting
      resetFunc();
    }
    break;
  case ST_STREAMING:
#if defined(ESP8266)
    MDNS.update();
#endif
    static uint previous_station = curr_station;
    if (curr_station != previous_station) {
      // User changed station
      previous_station = curr_station;
      state = ST_STREAM_START;
    }
    static unsigned long last_autovol_ms = 0;
    if ((UseAutoVolume) && (millis() - last_autovol_ms > 5000)) {
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
                    // cppcheck-suppress knownConditionTrueFalse
                    (mdns_active) ? MDNS_ADDRESS : (PGM_P)("<inactive>"),
                    // cppcheck-suppress knownConditionTrueFalse
                    (mdns_active) ? (PGM_P)(".local") : "");
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
    Serial.printf_P((PGM_P) "\r\nStation: %s", StationList[curr_station].Name);
    Serial.printf_P((PGM_P) "\r\nFreq: %5.2fMHz  Power: %3d%%   Volume: %3d%%",
                    float(fm_radio.GetFreq()) / 1000.0f, fm_radio.GetTxPower(),
                    fm_radio.GetVolume());
  }
  yield(); // Make sure WiFi can do its thing.
}
