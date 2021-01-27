
#include "battery.h"
#include "mode_switch.h"
#include "usb.h"

#include "nrf_gpio.h"

/**
 * Checks whether the system should be placed in "System OFF" mode (because the
 * battery is low or because the device was switched off via the mode switch).
 */
static int should_shut_off(void) {
	if (battery_is_low() && !usb_is_connected()) {
		return 1;
	}
	if (mode_switch_is_off() && !usb_is_connected()) {
		return 1;
	}
	return 0;
}

/**
 * Switches the device to "System OFF" mode.
 *
 * This function does not ensure that all peripherals are in a state where they
 * can wake the system up again.
 */
static void switch_off(void) {
	/* TODO */
	for (;;) {}
}

int main(void) {
	/* initialize battery, USB and mode switch - we need to initialize at
	 * least everything that is required to wake up again */
	battery_init();
	mode_switch_init();
	usb_init(); /* TODO: required? */

	/* shut down again if the battery is low and USB is not connected */
	if (should_shut_off()) {
		battery_prepare_system_off();
		mode_switch_prepare_system_off();
		usb_prepare_system_off();
		switch_off();
	}
	/* TODO */

	/* initialize everything else */
	/* TODO */

	#define LED_PIN NRF_GPIO_PIN_MAP(1, 15)
	nrf_gpio_cfg_output(LED_PIN);   // Configure pin as output

	nrf_gpio_pin_set(LED_PIN);
	//nrf_gpio_pin_clear(LED_PIN);    // Set low
	/*nrf_gpio_pin_set(LED_PIN);      // Set high
	nrf_gpio_pin_clear(LED_PIN);    // Set low
	nrf_gpio_pin_toggle(LED_PIN);   // Toggle state*/

	for (;;) {
		/* TODO */
	}
}
