
#include "mode_switch.hpp"
#include "power_supply.hpp"

#include <init.h>
#include <power/power.h>

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

