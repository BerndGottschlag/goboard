/**
 * @mainpage GoBoard USB/Wireless Keyboard
 *
 * This project implements the firmware for the main keyboard PCB. The firmware
 * implements an [USB keyboard](@ref usb) as well as two wireless keyboard
 * configurations, selectable with a [mode switch](@ref mode_switch).
 *
 * See the [list of modules](modules.html) for documentation of the different
 * parts of the firmware.
 */

#include "keys.h"
#include "mode_led.h"
#include "mode_switch.h"
#include "power_supply.h"
#include "usb.h"
#include "unifying.h"

#include "app_scheduler.h"
#include "app_timer.h"
#include "nrf_crypto.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_gpiote.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"

#define SCHED_MAX_EVENT_DATA_SIZE APP_TIMER_SCHED_EVENT_DATA_SIZE
#define SCHED_QUEUE_SIZE 20

/* TODO: mode LED */

/*
 * mode LED states:
 * - off: keyboard off
 * - on: keyboard connected or charging
 * - slow blinking: not connected
 * - fast blinking: pairing
 */

static enum mode_switch_mode current_mode;

static void basic_init(void) {
	ret_code_t ret;

	ret = NRF_LOG_INIT(NULL);
	APP_ERROR_CHECK(ret);
	NRF_LOG_DEFAULT_BACKENDS_INIT();

	ret = nrf_drv_clock_init();
	APP_ERROR_CHECK(ret);
	nrf_drv_clock_lfclk_request(NULL);
	while (!nrf_drv_clock_lfclk_is_running()) {}
	nrf_drv_clock_hfclk_request(NULL);
	while (!nrf_drv_clock_hfclk_is_running()) {}

	ret = app_timer_init();
	APP_ERROR_CHECK(ret);

	ret = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(ret);

	ret = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(ret);

	APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/**
 * Switches the device to "System OFF" mode.
 *
 * This function does not ensure that all peripherals are in a state where they
 * can wake the system up again.
 */
static void switch_off(void) {
	NRF_LOG_INFO("switching off");
	power_supply_prepare_system_off();
	mode_switch_prepare_system_off();
	mode_led_prepare_system_off();
	nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_GOTO_SYSOFF);
}

static void reset(void) {
	nrf_pwr_mgmt_shutdown(NRF_PWR_MGMT_SHUTDOWN_RESET);
}

static void check_power_off_reset(void) {
	enum power_supply_mode ps = power_supply_get_mode();
	enum mode_switch_mode mode = mode_switch_get();

	/* we should power off if the battery is low and we are not charging */
	if (ps == POWER_SUPPLY_LOW) {
		switch_off();
	}
	/* we should power off if the mode is off/USB and USB is not connected */
	if (mode == MODE_SWITCH_OFF_USB && !usb_is_connected()) {
		switch_off();
	}
	/* we should reset if the mode changed between USB and bluetooth */
	if (current_mode == MODE_SWITCH_OFF_USB && mode != MODE_SWITCH_OFF_USB) {
		reset();
	}
	if (current_mode != MODE_SWITCH_OFF_USB && mode == MODE_SWITCH_OFF_USB) {
		reset();
	}
}

static void update_mode_led(void) {
	/* TODO: check USB/bluetooth connection and pairing state */
	if (mode_switch_get() == MODE_SWITCH_BT1 ||
			mode_switch_get() == MODE_SWITCH_BT2) {
		mode_led_set(MODE_LED_CONNECTED);
	} else if (power_supply_get_mode() == POWER_SUPPLY_CHARGING) {
		mode_led_set(MODE_LED_CHARGING);
	}

}

static void on_power_supply_state_change(void) {
	check_power_off_reset();

	/* report the new battery charge via bluetooth */
	/* TODO */

	update_mode_led();
}

static void on_mode_switch_change(void) {
	check_power_off_reset();

	/* if we just switched between the two bluetooth device sets, we need to
	 * notify the bluetooth keyboard code */
	/* TODO */

	update_mode_led();
}

/**
 * Main entry point of the firmware.
 */
int main(void) {
	ret_code_t ret;

	basic_init();
	NRF_LOG_INFO("goboard starting");

	/* initialize power supply and mode switch - we need to initialize at
	 * least everything here that is required to wake up again */
	power_supply_init(on_power_supply_state_change);
	mode_switch_init(on_mode_switch_change);
	/* we need the current mode to be valid during check_power_off_reset()
	 * already */
	current_mode = mode_switch_get();

	check_power_off_reset();

	/* initialize everything else */
	mode_led_init();
	update_mode_led();
	keys_init();

	ret = nrf_crypto_init();
	APP_ERROR_CHECK(ret);
	unifying_init();
#if 0
	key_matrix_init();
	/* TODO */

	for (;;) {
		usb_poll();
		app_sched_execute();
		if (NRF_LOG_PROCESS() == false) {
			nrf_pwr_mgmt_run();
		}
	}
#endif
	for (;;) {
		app_sched_execute();
		if (NRF_LOG_PROCESS() == false) {
			nrf_pwr_mgmt_run();
		}
	}
}

