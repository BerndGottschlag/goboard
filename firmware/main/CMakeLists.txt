cmake_minimum_required(VERSION 3.18)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(DTS_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(SRC
	src/exception.hpp
	src/keys.cpp
	src/keys.hpp
	src/power_supply.cpp
	src/power_supply.hpp
	src/scan_code.hpp
)

if("${BOARD}" MATCHES "native_posix_64")
	# Testing-only files.
	set(SRC
		${SRC}
		src/main_testing.cpp
		src/tests.cpp
		src/tests.hpp
	)
else()
	# Device-only files.
	set(SRC
		${SRC}
		src/bluetooth.cpp
		src/bluetooth.hpp
		src/key_matrix.cpp
		src/key_matrix.hpp
		src/leds.cpp
		src/leds.hpp
		src/main.cpp
		src/mode_switch.cpp
		src/mode_switch.hpp
		src/power_supply_pins.hpp
		src/power_supply_pins.cpp
		src/unifying.hpp
		src/unifying.cpp
		src/usb.hpp
		src/usb.cpp
	)
endif()

target_sources(app PRIVATE ${SRC})
target_compile_options(app PRIVATE -Wno-missing-field-initializers)
