# FM Streamer



# Setup / Usage

Install [arduino-cli](https://github.com/arduino/arduino-cli). This project is set up to work out of the box with it, and this will save you having to set up the ESP8266 in your Arduino IDE.

Run `make config-tools` to install libraries and ESP8266 core.

Rename `arduino_secrets.h.example` to `arduino_secrets.h` and change the wifi SSID and password in it to match the network you wish to attach to.


TODO:
- Try sending page from PROGMEM bit by bit, instead of passing whole thing to client.print()
- Create modes, streamer vs. config server. Deallocate streamer while serving and vice versa. Maybe this'll fix memory issues?
- Handle board LED
- Clean up board wiring
