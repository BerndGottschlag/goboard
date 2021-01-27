#!/bin/bash

set -e

# compile the main application
cd build
if [ ! -f Makefile ]
then
	cmake .. -DCMAKE_TOOLCHAIN_FILE=../Toolchain.cmake
fi
make -j`nproc`

# show the size of the application
arm-none-eabi-size goboard-main

# generate a hex from the main application
arm-none-eabi-objcopy -O ihex goboard-main goboard-main.hex

# NOTE: use "uf2conv.py goboard-main.hex -c -f 0xADA52840" to generate an UF2 file
