/*
Manages station and radio configurations.

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

#include "config.h"
#if defined(ESP32)
#include <esp_spiffs.h>
#include <sys/stat.h>
#endif // ESP32

namespace {
#if defined(ESP32)
String ReadConfigVal_(FILE *configfile) {
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
String ReadConfigVal_(File *configfile) {
  String val = "";
  val.reserve(16);
  char r = (char)configfile->read();
  while (r != ' ') {
    val += r;
    r = (char)configfile->read();
  }
  return val;
}
#endif // ESP32 / ESP8266
} // namespace

void Config::Start(void) {
#if defined(ESP32)
  // Start up flash filesystem and initialize config
  esp_vfs_spiffs_conf_t conf = {.base_path = FILE_DIR,
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = true};
  esp_vfs_spiffs_register(&conf);
  struct stat st;
  if (stat(FILE_NAME, &st) != 0) {
    WriteToFlash(); // Write in defaults
  }
  FILE *configfile = fopen(FILE_NAME, "r");
#elif defined(ESP8266)
  SPIFFS.begin();
  if (!SPIFFS.exists(FILE_NAME)) {
    WriteToFlash(); // Write in defaults
  }
  File configfile = SPIFFS.open(FILE_NAME, "r");
#endif
  station_ = ReadConfigVal_(configfile).toInt();
  freq_ = ReadConfigVal_(configfile).toInt();
  power_ = ReadConfigVal_(configfile).toInt();
  volume_ = ReadConfigVal_(configfile).toInt();
#if defined(ESP32)
  fclose(configfile);
#elif defined(ESP8266)
  configfile.close();
#endif // ESP32 / ESP8266
}

void Config::WriteToFlash(void) {
#if defined(ESP32)
  struct stat st;
  if (stat(FILE_NAME, &st) == 0)
    unlink(FILE_NAME);
  FILE *configfile = fopen(FILE_NAME, "w");
  fprintf(configfile, "%d %d %d %d ", station_, freq_, power_, volume_);
  fclose(configfile);
#elif defined(ESP8266)
  if (SPIFFS.exists(FILE_NAME))
    SPIFFS.remove(FILE_NAME);
  File configfile = SPIFFS.open(FILE_NAME, "w");
  configfile.printf("%d %d %d %d ", station_, freq_, power_, volume_);
  configfile.close();
#endif // ESP32 / ESP8266
}

bool Config::SetStation(uint station) {
  if (station > num_stations_)
    return false;
  station_ = station;
  return true;
}

bool Config::SetFreqkHz(uint freq) {
  if ((freq > 108000) || (freq < 76000))
    return false;
  freq_ = freq;
  return true;
}

bool Config::SetFreqMHz(float freq) {
  return SetFreqkHz((uint)(freq * 1000.0f));
}

bool Config::SetPower(uint power) {
  if (power > 100)
    return false;
  power_ = power;
  return true;
}

bool Config::SetVolume(uint volume) {
  if (volume > 100)
    return false;
  volume_ = volume;
  return true;
}
