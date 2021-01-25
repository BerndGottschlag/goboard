cmake_minimum_required(VERSION 3.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(goboard-main C ASM)

set(SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nrf5-sdk")
set(MDK_PATH "${SDK_PATH}/modules/nrfx/mdk")

set(SRC
	src/main.c

	${MDK_PATH}/gcc_startup_nrf52840.S
	${MDK_PATH}/system_nrf52840.c
	${MDK_PATH}/system_nrf52840.h
)

include_directories(
	include
	${SDK_PATH}/components/toolchain/cmsis/include
)

add_compile_definitions(NRF52840_XXAA)

# configure the compiler for the Cortex-M4F
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv5te -msoft-float -marm -mfpu=vfp")

# enable warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -flto")

set(LINKER_SCRIPT "${MDK_PATH}/nrf52840_xxaa.ld")
link_directories("${MDK_PATH}")
add_executable(goboard-main ${SRC} ${LINKER_SCRIPT})
target_link_libraries(goboard-main gcc)
set_target_properties(goboard-main PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT}")
