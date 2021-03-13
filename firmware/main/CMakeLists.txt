cmake_minimum_required(VERSION 3.18)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(SRC
	src/mode_switch.cpp
	src/mode_switch.hpp
	src/power_supply.cpp
	src/power_supply.hpp
)

if("${BOARD}" MATCHES "native_posix_64")
	# Testing-only files.
	set(SRC
		${SRC}
		src/main_testing.cpp
	)
else()
	# Device-only files.
	set(SRC
		${SRC}
		src/main.cpp
	)
endif()

target_sources(app PRIVATE ${SRC})
