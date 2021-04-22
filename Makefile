# Board serial port for upload
SERIAL_PORT ?= /dev/cu.usbserial-0001
BAUDRATE = 115200

# Arduino CLI Fully Qualified Board Name (FQBN)
MCU_TYPE ?= esp8266
#MCU_TYPE ?= esp32
ifeq ($(MCU_TYPE), esp8266)
	BOARD_TYPE ?= $(MCU_TYPE):$(MCU_TYPE):nodemcuv2
	PACKAGE_URLS ?= "https://arduino.esp8266.com/stable/package_esp8266com_index.json"
	BOARD_OPTIONS ?= :xtal=160
	GIT_LIBRARIES ?= https://github.com/me-no-dev/ESPAsyncWebServer.git https://github.com/me-no-dev/ESPAsyncTCP
else ifeq ($(MCU_TYPE), esp32)
	BOARD_TYPE ?= $(MCU_TYPE):$(MCU_TYPE):node32s
	PACKAGE_URLS ?= https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
	GIT_LIBRARIES ?= https://github.com/me-no-dev/ESPAsyncWebServer.git https://github.com/me-no-dev/AsyncTCP.git
	BOARD_OPTIONS ?=
endif
LIBRARIES ?= ESP8266Audio@1.9.0 "Adafruit Si4713 Library" # Add extra libraries in a space-separated list

PROJECT_BASE = fm-streamer
PROJECT ?= fm-streamer

# Tool paths / names
ARDUINO_CLI = arduino-cli

# Optional verbose compile/upload trigger
V ?= 0
VERBOSE=

# Build path -- used to store built binary and object files
BUILD_DIR=_build
SOURCE_PATH=$(PWD)/$(PROJECT_BASE)/
BUILD_PATH=$(SOURCE_PATH)$(BUILD_DIR)

ifneq ($(V), 0)
	VERBOSE=-v
endif

.PHONY: all fm-streamer program clean

all: fm-streamer

config-tools:
	$(ARDUINO_CLI) cache clean
	$(ARDUINO_CLI) config init --additional-urls $(PACKAGE_URLS) --overwrite
	$(ARDUINO_CLI) config set library.enable_unsafe_install true
	$(ARDUINO_CLI) core update-index
	$(ARDUINO_CLI) core install $(MCU_TYPE):$(MCU_TYPE)
	$(ARDUINO_CLI) lib install $(LIBRARIES)
	$(foreach lib, $(GIT_LIBRARIES), $(ARDUINO_CLI) lib install --git-url $(lib);)

fm-streamer:
	$(ARDUINO_CLI) compile $(VERBOSE) --build-path=$(BUILD_PATH) --build-cache-path=$(BUILD_PATH) -b $(BOARD_TYPE)$(BOARD_OPTIONS) $(PROJECT_BASE)/$(PROJECT)

program: all stop-serial
	$(ARDUINO_CLI) upload $(VERBOSE) -p $(SERIAL_PORT) --fqbn $(BOARD_TYPE)$(BOARD_OPTIONS) --input-dir=$(BUILD_PATH)

stop-serial:
	screen -ls | grep Detached | cut -d. -f1 | awk '{print $1}' | xargs kill

serial: stop-serial
	screen $(SERIAL_PORT) $(BAUDRATE)

check:
	clang-format $(SOURCE_PATH)*.ino $(SOURCE_PATH)*.h -style=file -i
	cppcheck --suppress=missingIncludeSystem --enable=all --std=c++11 --inline-suppr --language=c++ $(SOURCE_PATH)*.ino

clean:
	@rm -rf $(BUILD_PATH)
