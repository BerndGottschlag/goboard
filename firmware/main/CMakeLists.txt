cmake_minimum_required(VERSION 3.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

project(goboard-main C ASM)

set(SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/nrf5-sdk")
set(MDK_PATH "${SDK_PATH}/modules/nrfx/mdk")

add_compile_definitions(
	NRF52840_XXAA
	FLOAT_ABI_HARD
	SOFTDEVICE_PRESENT
	S140
	TEST_SETUP
)

# configure the compiler for the Cortex-M4F
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb -mabi=aapcs -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")

# optimization
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -flto")

set(SDK_SRC
	${SDK_PATH}/components/ble/ble_advertising/ble_advertising.c
	${SDK_PATH}/components/ble/ble_link_ctx_manager/ble_link_ctx_manager.c
	${SDK_PATH}/components/ble/ble_services/ble_bas/ble_bas.c
	${SDK_PATH}/components/ble/ble_services/ble_hids/ble_hids.c
	${SDK_PATH}/components/ble/common/ble_advdata.c
	${SDK_PATH}/components/ble/common/ble_conn_params.c
	${SDK_PATH}/components/ble/common/ble_conn_state.c
	${SDK_PATH}/components/ble/common/ble_srv_common.c
	${SDK_PATH}/components/ble/nrf_ble_gatt/nrf_ble_gatt.c
	${SDK_PATH}/components/ble/nrf_ble_qwr/nrf_ble_qwr.c
	${SDK_PATH}/components/ble/peer_manager/auth_status_tracker.c
	${SDK_PATH}/components/ble/peer_manager/gatt_cache_manager.c
	${SDK_PATH}/components/ble/peer_manager/gatts_cache_manager.c
	${SDK_PATH}/components/ble/peer_manager/id_manager.c
	${SDK_PATH}/components/ble/peer_manager/nrf_ble_lesc.c
	${SDK_PATH}/components/ble/peer_manager/peer_database.c
	${SDK_PATH}/components/ble/peer_manager/peer_data_storage.c
	${SDK_PATH}/components/ble/peer_manager/peer_id.c
	${SDK_PATH}/components/ble/peer_manager/peer_manager.c
	${SDK_PATH}/components/ble/peer_manager/peer_manager_handler.c
	${SDK_PATH}/components/ble/peer_manager/pm_buffer.c
	${SDK_PATH}/components/ble/peer_manager/security_dispatcher.c
	${SDK_PATH}/components/ble/peer_manager/security_manager.c
	${SDK_PATH}/components/libraries/atomic/nrf_atomic.c
	${SDK_PATH}/components/libraries/atomic_fifo/nrf_atfifo.c
	${SDK_PATH}/components/libraries/atomic_flags/nrf_atflags.c
	${SDK_PATH}/components/libraries/balloc/nrf_balloc.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_aead.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_aes.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_aes_shared.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_ecc.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_ecdh.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_ecdsa.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_eddsa.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_error.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_hash.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_hkdf.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_hmac.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_init.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_rng.c
	${SDK_PATH}/components/libraries/crypto/nrf_crypto_shared.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_aes_aead.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_aes.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_chacha_poly_aead.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_ecc.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_ecdh.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_ecdsa.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_eddsa.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_hash.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_hmac.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_init.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_mutex.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_rng.c
	${SDK_PATH}/components/libraries/crypto/backend/cc310/cc310_backend_shared.c
	${SDK_PATH}/components/libraries/experimental_section_vars/nrf_section_iter.c
	${SDK_PATH}/components/libraries/fds/fds.c
	${SDK_PATH}/components/libraries/fstorage/nrf_fstorage.c
	${SDK_PATH}/components/libraries/fstorage/nrf_fstorage_nvmc.c
	${SDK_PATH}/components/libraries/fstorage/nrf_fstorage_sd.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_flash.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_rtt.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_serial.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_backend_uart.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_default_backends.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_frontend.c
	${SDK_PATH}/components/libraries/log/src/nrf_log_str_formatter.c
	${SDK_PATH}/components/libraries/memobj/nrf_memobj.c
	${SDK_PATH}/components/libraries/pwr_mgmt/nrf_pwr_mgmt.c
	${SDK_PATH}/components/libraries/ringbuf/nrf_ringbuf.c
	${SDK_PATH}/components/libraries/scheduler/app_scheduler.c
	${SDK_PATH}/components/libraries/strerror/nrf_strerror.c
	${SDK_PATH}/components/libraries/util/app_error.c
	${SDK_PATH}/components/libraries/util/app_error_handler_gcc.c
	${SDK_PATH}/components/libraries/util/app_error_weak.c
	${SDK_PATH}/components/libraries/util/app_util_platform.c
	${SDK_PATH}/components/libraries/util/nrf_assert.c
	${SDK_PATH}/components/libraries/util/sdk_mapped_flags.c
	${SDK_PATH}/components/libraries/usbd/app_usbd.c
	${SDK_PATH}/components/libraries/usbd/app_usbd_core.c
	${SDK_PATH}/components/libraries/usbd/app_usbd_serial_num.c
	${SDK_PATH}/components/libraries/usbd/app_usbd_string_desc.c
	${SDK_PATH}/components/libraries/usbd/class/hid/app_usbd_hid.c
	${SDK_PATH}/components/libraries/usbd/class/hid/kbd/app_usbd_hid_kbd.c
	${SDK_PATH}/components/proprietary_rf/esb/nrf_esb.c
	${SDK_PATH}/components/softdevice/common/nrf_sdh_ble.c
	${SDK_PATH}/components/softdevice/common/nrf_sdh.c
	${SDK_PATH}/components/softdevice/common/nrf_sdh_soc.c
	${SDK_PATH}/components/libraries/timer/app_timer.c
	${SDK_PATH}/components/libraries/timer/drv_rtc.c

	${SDK_PATH}/external/fprintf/nrf_fprintf.c
	${SDK_PATH}/external/fprintf/nrf_fprintf_format.c

	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_clock.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_power.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_ppi.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_rng.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_spi.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_spis.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_swi.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_twi.c
	${SDK_PATH}/integration/nrfx/legacy/nrf_drv_uart.c

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
	${SDK_PATH}/modules/nrfx/drivers/src/prs/nrfx_prs.c
	${SDK_PATH}/modules/nrfx/soc/nrfx_atomic.c
)

add_library(nrf52_sdk STATIC ${SDK_SRC})
set_property(TARGET nrf52_sdk PROPERTY C_STANDARD 11)

set(SRC
	src/keys.c
	src/keys.h
	src/main.c
	src/mode_led.c
	src/mode_led.h
	src/mode_switch.c
	src/mode_switch.h
	src/power_supply.c
	src/power_supply.h
	src/unifying.c
	src/unifying.h
	src/unifying_radio.c
	src/unifying_radio.h
	src/usb.c
	src/usb.h

	${MDK_PATH}/gcc_startup_nrf52840.S
	${MDK_PATH}/system_nrf52840.c
)

include_directories(
	src
	${SDK_PATH}/components/ble/common
	${SDK_PATH}/components/ble/ble_advertising
	${SDK_PATH}/components/ble/ble_link_ctx_manager
	${SDK_PATH}/components/ble/ble_services/ble_bas
	${SDK_PATH}/components/ble/ble_services/ble_hids
	${SDK_PATH}/components/ble/nrf_ble_gatt
	${SDK_PATH}/components/ble/nrf_ble_qwr
	${SDK_PATH}/components/ble/peer_manager
	${SDK_PATH}/components/libraries/atomic
	${SDK_PATH}/components/libraries/atomic_fifo
	${SDK_PATH}/components/libraries/atomic_flags
	${SDK_PATH}/components/libraries/balloc
	${SDK_PATH}/components/libraries/crypto
	${SDK_PATH}/components/libraries/crypto/backend/cc310
	${SDK_PATH}/components/libraries/crypto/backend/cc310_bl
	${SDK_PATH}/components/libraries/crypto/backend/cifra
	${SDK_PATH}/components/libraries/crypto/backend/mbedtls
	${SDK_PATH}/components/libraries/crypto/backend/micro_ecc
	${SDK_PATH}/components/libraries/crypto/backend/nrf_hw
	${SDK_PATH}/components/libraries/crypto/backend/nrf_sw
	${SDK_PATH}/components/libraries/crypto/backend/oberon
	${SDK_PATH}/components/libraries/crypto/backend/optiga
	${SDK_PATH}/components/libraries/delay
	${SDK_PATH}/components/libraries/experimental_section_vars
	${SDK_PATH}/components/libraries/fds
	${SDK_PATH}/components/libraries/fstorage
	${SDK_PATH}/components/libraries/log
	${SDK_PATH}/components/libraries/log/src
	${SDK_PATH}/components/libraries/memobj
	${SDK_PATH}/components/libraries/mutex
	${SDK_PATH}/components/libraries/pwr_mgmt
	${SDK_PATH}/components/libraries/queue
	${SDK_PATH}/components/libraries/ringbuf
	${SDK_PATH}/components/libraries/scheduler
	${SDK_PATH}/components/libraries/stack_info
	${SDK_PATH}/components/libraries/timer
	${SDK_PATH}/components/libraries/util
	${SDK_PATH}/components/libraries/usbd
	${SDK_PATH}/components/libraries/usbd/class/hid
	${SDK_PATH}/components/libraries/usbd/class/hid/kbd
	${SDK_PATH}/components/libraries/strerror
	${SDK_PATH}/components/proprietary_rf/esb
	${SDK_PATH}/components/softdevice/common
	${SDK_PATH}/components/softdevice/s140/headers
	${SDK_PATH}/components/softdevice/s140/headers/nrf52
	${SDK_PATH}/components/toolchain/cmsis/include
	${SDK_PATH}/external/fprintf
	${SDK_PATH}/external/mbedtls/include
	${SDK_PATH}/external/nrf_cc310/include
	${SDK_PATH}/external/utf_converter
	${SDK_PATH}/integration/nrfx
	${SDK_PATH}/integration/nrfx/legacy
	${SDK_PATH}/modules/nrfx
	${SDK_PATH}/modules/nrfx/drivers/include
	${SDK_PATH}/modules/nrfx/hal
	${SDK_PATH}/modules/nrfx/mdk
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
link_directories(
	"${MDK_PATH}"
	"${SDK_PATH}/external/nrf_cc310/lib/cortex-m4/hard-float"
)
add_executable(goboard-main ${SRC} ${LINKER_SCRIPT})
set_property(TARGET goboard-main PROPERTY C_STANDARD 11)
target_compile_options(goboard-main PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-expansion-to-defined)
target_link_libraries(goboard-main nrf52_sdk nrf_cc310_0.9.13 gcc)
set_target_properties(goboard-main PROPERTIES LINK_FLAGS "-T ${LINKER_SCRIPT}")
