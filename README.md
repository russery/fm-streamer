# FM Streamer
Are there not a lot of good FM radio stations in your vicinity? Have you ever wished you could receive your hometown station on your FM radio while you're away? Well this is the project for you!

This is a simple ESP8266 project that grabs an MP3 internet stream and bridges it to an FM transmitter, so that you can listen to the stream on your old-school FM radio.


# Setup / Usage
If you're on Linux or Mac (or any system with GNU Make installed), install [arduino-cli](https://github.com/arduino/arduino-cli). This project is set up to work out of the box with it, and this will save you having to set up the ESP8266 and required libraries in your Arduino IDE.

1. Run `make config-tools` to install libraries and ESP8266 core.
1. Rename `arduino_secrets.h.example` to `arduino_secrets.h` and change the wifi SSID and password in it to match the network you wish to attach to.
1. If you want to change the radio streams, edit them in the `StationList` struct in fm-streamer.ino.
1. Power on your ESP8266 and connect its serial port to your computer. Edit the Makefile to change the `SERIAL_PORT` to match how the ESP8266 comes up on your system.
1. Run `make program` to compile the code and upload it to your board.

---

# Under the Hood
This project consists of three hardware components:
1. NodeMCU v1.0 ESP3266
1. [Adafruit UDA1334A I2S Stereo DAC breakout board](https://www.adafruit.com/product/3678)
1. [Adafruit Si4713 FM Transmitter breakout board](https://www.adafruit.com/product/1958)

## Hardware
These are wired together like so:
<img src="./docs/hardware-wiring.png" width="400" />

Everything is powered from the NodeMCU's 3.3V output. The Si4713 FM transmitter is connected to the NodeMCU over I2C, the UDA1334 DAC is connected to it over I2S, and then the UDA1334 DAC outputs audio to the Si4713 transmitter via analog stereo audio.

I also wired a 4.7kΩ resistor from each of the DAC's analog audio outputs to GND to provide a bit of loading for it. I found that this helped reduce noise and clipping on the FM transmitter's input, although I did not experiment much with values or whether this is truly necessary.

## Software
The software makes use of the ESP8266Audio library and Adafruit's Si4713 driver:
<img src="./docs/software-diagram.png" width="600" />


