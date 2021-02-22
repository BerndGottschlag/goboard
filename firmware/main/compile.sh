#!/bin/bash

set -e

# compile the main application
mkdir -p build
cd build
if [ ! -f Makefile ]
then
	cmake .. -DCMAKE_TOOLCHAIN_FILE=../Toolchain.cmake
fi
make -j`nproc`

# regenerate the documentation to show documentation warnings
(
	cd ..
	doxygen > /dev/null
)

# show the size of the application
arm-none-eabi-size goboard-main

# generate a hex from the main application
arm-none-eabi-objcopy -O ihex goboard-main goboard-main.hex

# NOTE: use "uf2conv.py goboard-main.hex -c -f 0xADA52840" to generate an UF2 file
adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application goboard-main.hex goboard-main.zip

# NOTE: use the following command to flash via the adafruit DFU bootloader
# adafruit-nrfutil dfu serial --package build/goboard-main.zip -p /dev/ttyACM0
