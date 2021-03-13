
#include "mode_switch.hpp"
#include "power_supply.hpp"

#include <init.h>
#include <power/power.h>

#include <device.h>
#include <drivers/pwm.h>

#define LED_NODE DT_ALIAS(mode_pwm_led)

#if DT_NODE_HAS_STATUS(LED_NODE, okay)
#define LED_LABEL	DT_PWMS_LABEL(LED_NODE)
#define LED_CHANNEL	DT_PWMS_CHANNEL(LED_NODE)
#define LED_FLAGS	DT_PWMS_FLAGS(LED_NODE)
#else
#error "Unsupported board: mode_pwm_led devicetree alias is not defined"
#define LED_LABEL	""
#define LED_CHANNEL	0
#define LED_FLAGS	0
#endif

/// Disable System OFF - we only ever want to enter it manually.
static int disable_system_off(const struct device *dev) {
	(void)dev;
	pm_ctrl_disable_state(POWER_STATE_DEEP_SLEEP_1);
	return 0;
}
SYS_INIT(disable_system_off, PRE_KERNEL_2, 0);


/// Main keyboard application.
///
/// The function initializes and runs the keyboard code. When the function
/// exits, the device has been prepared for System OFF mode.
static void run_keyboard() {
	// TODO: remove
	const struct device *mode_led = device_get_binding(LED_LABEL);
	if (mode_led == NULL) {
		// TODO
	}
	int pulse = 0;
	while (true) {
		pwm_pin_set_usec(mode_led, LED_CHANNEL, 10000,
		                 pulse, LED_FLAGS);
		pulse = (pulse + 100) % 1000;
		k_sleep(K_MSEC(100));
	}

	PowerSupply power_supply;
	ModeSwitch mode_switch;
	// TODO: Check whether we should immediately shut down again because the
	// battery is low.

	// TODO: Remaining keyboard code.
}

void main(void) {
	// TODO: Catch exceptions and reset the keyboard.
	run_keyboard();

	// Enter System OFF.
	printk("Switching off...\n");
	pm_power_state_force(POWER_STATE_DEEP_SLEEP_1);
	k_sleep(K_SECONDS(1));
	printk("Shutdown failed.\n");
	while (true);
}

