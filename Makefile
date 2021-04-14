
# Board serial port for upload
SERIAL_PORT ?= /dev/cu.usbserial-0001
BAUDRATE = 115200

# Arduino CLI Fully Qualified Board Name (FQBN)
BOARD_TYPE ?= esp8266:esp8266:nodemcuv2
BOARD_OPTIONS ?= :xtal=160
PACKAGE_URLS = "https://arduino.esp8266.com/stable/package_esp8266com_index.json" # Add extra packages in comma-separated list

PROJECT_BASE = fm-streamer
PROJECT ?= fm-streamer

# Tool paths / names
ARDUINO_CLI = arduino-cli
SERIAL_TERM = screen

# Optional verbose compile/upload trigger
V ?= 0
VERBOSE=

# Build path -- used to store built binary and object files
BUILD_DIR=_build
BUILD_PATH=$(PWD)/$(PROJECT_BASE)/$(BUILD_DIR)

ifneq ($(V), 0)
	VERBOSE=-v
endif

.PHONY: all fm-streamer program clean

all: fm-streamer

config-tools:
	$(ARDUINO_CLI) config init --additional-urls $(PACKAGE_URLS) --overwrite
	$(ARDUINO_CLI) core update-index
	# TODO: Install any needed libraries?

fm-streamer:
	$(ARDUINO_CLI) compile $(VERBOSE) --build-path=$(BUILD_PATH) --build-cache-path=$(BUILD_PATH) -b $(BOARD_TYPE)$(BOARD_OPTIONS) $(PROJECT_BASE)/$(PROJECT)

program:
	$(ARDUINO_CLI) upload $(VERBOSE) -p $(SERIAL_PORT) --fqbn $(BOARD_TYPE)$(BOARD_OPTIONS) --input-dir=$(BUILD_PATH)

serial:
	$(SERIAL) $(SERIAL_PORT) $(BAUDRATE)

clean:
	@rm -rf $(BUILD_PATH)
	@rm $(PROJECT_BASE)/$(PROJECT)/*.elf
	@rm $(PROJECT_BASE)/$(PROJECT)/*.hex