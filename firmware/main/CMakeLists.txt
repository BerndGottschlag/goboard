cmake_minimum_required(VERSION 3.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(goboard-main C ASM)

set(SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nrf5-sdk")
set(MDK_PATH "${SDK_PATH}/modules/nrfx/mdk")

add_compile_definitions(NRF52840_XXAA)

# configure the compiler for the Cortex-M4F
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")

# enable warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-expansion-to-defined")

# optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -flto")

set(SDK_SRC
	${SDK_PATH}/components/libraries/atomic/nrf_atomic.c
	${SDK_PATH}/components/libraries/atomic_fifo/nrf_atfifo.c
	${SDK_PATH}/components/libraries/balloc/nrf_balloc.c
#	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_flash.c
#	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_rtt.c
#	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_serial.c
#	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_uart.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_default_backends.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_frontend.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_str_formatter.c
	${SDK_PATH}/components/libraries/memobj/nrf_memobj.c
	${SDK_PATH}/components/libraries/ringbuf/nrf_ringbuf.c
	${SDK_PATH}/components/libraries/util/app_error.c
	${SDK_PATH}/components/libraries/util/app_error_handler_gcc.c
	${SDK_PATH}/components/libraries/util/app_error_weak.c
	${SDK_PATH}/components/libraries/util/app_util_platform.c
	${SDK_PATH}/components/libraries/util/nrf_assert.c
	${SDK_PATH}/components/libraries/util/sdk_mapped_flags.c
	${SDK_PATH}/components/libraries/usbd/app_usbd.c

	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_adc.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_clock.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_comp.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_dppi.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_gpiote.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_i2s.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_lpcomp.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_nfct.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_nvmc.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_pdm.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_power.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_ppi.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_pwm.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_qdec.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_qspi.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_rng.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_rtc.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_saadc.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_spi.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_spim.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_spis.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_swi.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_systick.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_temp.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_timer.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_twi.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_twim.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_twis.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_twi_twim.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_uart.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_uarte.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_usbd.c
	${SDK_PATH}/modules/nrfx/drivers/src/nrfx_wdt.c
	${SDK_PATH}/modules/nrfx/soc/nrfx_atomic.c
)

add_library(nrf52_sdk STATIC ${SDK_SRC})

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
)

include_directories(
	src
	${SDK_PATH}/components/libraries/atomic
	${SDK_PATH}/components/libraries/atomic_fifo
	${SDK_PATH}/components/libraries/balloc
	${SDK_PATH}/components/libraries/experimental_section_vars
	${SDK_PATH}/components/libraries/log
	${SDK_PATH}/components/libraries/log/src
	${SDK_PATH}/components/libraries/memobj
	${SDK_PATH}/components/libraries/queue
	${SDK_PATH}/components/libraries/ringbuf
	${SDK_PATH}/components/libraries/util
	${SDK_PATH}/components/libraries/usbd
	${SDK_PATH}/components/libraries/strerror
	${SDK_PATH}/components/softdevice/s140/headers
	${SDK_PATH}/components/toolchain/cmsis/include
	${SDK_PATH}/external/fprintf
	${SDK_PATH}/integration/nrfx
	${SDK_PATH}/integration/nrfx/legacy
	${SDK_PATH}/modules/nrfx
	${SDK_PATH}/modules/nrfx/drivers/include
	${SDK_PATH}/modules/nrfx/hal
	${SDK_PATH}/modules/nrfx/mdk
)

#set(LINKER_SCRIPT "${MDK_PATH}/nrf52840_xxaa.ld")
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
link_directories("${MDK_PATH}")
add_executable(goboard-main ${SRC} ${LINKER_SCRIPT})
target_link_libraries(goboard-main nrf52_sdk gcc)
set_target_properties(goboard-main PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT}")
