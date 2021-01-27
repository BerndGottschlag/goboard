cmake_minimum_required(VERSION 3.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(goboard-main C ASM)

set(SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nrf5-sdk")
set(MDK_PATH "${SDK_PATH}/modules/nrfx/mdk")

set(SRC
	src/battery.c
	src/battery.h
	src/main.c
	src/mode_switch.c
	src/mode_switch.h
	src/usb.c
	src/usb.h

	${MDK_PATH}/gcc_startup_nrf52840.S
	${MDK_PATH}/system_nrf52840.c
	${MDK_PATH}/system_nrf52840.h
)

include_directories(
	src
	${SDK_PATH}/components/libraries/util
	${SDK_PATH}/components/softdevice/s140/headers
	${SDK_PATH}/components/toolchain/cmsis/include
	${SDK_PATH}/integration/nrfx
	${SDK_PATH}/modules/nrfx
	${SDK_PATH}/modules/nrfx/hal
	${SDK_PATH}/modules/nrfx/mdk
)

add_compile_definitions(NRF52840_XXAA)

# configure the compiler for the Cortex-M4F
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")

# enable warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -flto")

#set(LINKER_SCRIPT "${MDK_PATH}/nrf52840_xxaa.ld")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
link_directories("${MDK_PATH}")
add_executable(goboard-main ${SRC} ${LINKER_SCRIPT})
target_link_libraries(goboard-main gcc)
set_target_properties(goboard-main PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT}")
