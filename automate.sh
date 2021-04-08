
SERIAL_PORT="/dev/cu.usbserial-0001"
BAUDRATE=115200
COMMAND=$1
PROJECT="fm-streamer"
FQBN="esp8266:esp8266:nodemcuv2"

compile () {
	echo "Starting compilation..."
	arduino-cli compile $PROJECT --fqbn $FQBN
	echo "Done compiling."
}

program () {
	arduino-cli upload --port $SERIAL_PORT --fqbn $FQBN $PROJECT
}

if [[ $COMMAND == "compile" ]]; then
	compile
elif [[ $COMMAND == "program" ]]; then
	compile
	program
elif [[ $COMMAND == "serial" ]]; then
	screen $SERIAL_PORT $BAUDRATE
elif [[ $COMMAND == "setup" ]]; then
	arduino-cli core update-index
	arduino-cli core install esp8266:esp8266
fi

