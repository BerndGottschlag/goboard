cmake_minimum_required(VERSION 3.18)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD goboard_nrf52840)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")

set(SRC
	src/main.cpp
)
target_sources(app PRIVATE ${SRC})
