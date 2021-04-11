
#include "bluetooth.hpp"
#include "exception.hpp"
#include "key_matrix.hpp"
#include "keys.hpp"
#include "leds.hpp"
#include "mode_switch.hpp"
#include "power_supply.hpp"
#include "power_supply_pins.hpp"
#include "unifying.hpp"
#include "usb.hpp"

#include <init.h>
#include <power/power.h>
#include <power/reboot.h>
#include <settings/settings.h>

/*#include <device.h>
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
#endif*/

/// Disable System OFF - we only ever want to enter it manually.
static int disable_system_off(const struct device *dev) {
	(void)dev;
	pm_constraint_set(PM_STATE_SOFT_OFF);
	return 0;
}
SYS_INIT(disable_system_off, PRE_KERNEL_2, 0);

static K_SEM_DEFINE(main_loop_event, 0, 1);

void power_supply_mode_switch_handler() {
	// Let the main loop do the work.
	k_sem_give(&main_loop_event);
}

/// Returns true if the keyboard should immediately shut down to save energy.
bool want_shutdown(PowerSupply<PowerSupplyPins> *power_supply,
                   ModeSwitch *mode_switch) {
	if (power_supply->get_mode() == POWER_SUPPLY_LOW) {
		printk("low power, shutting down.\n");
		return true;
	} else if (!power_supply->has_usb_connection() && mode_switch->get_mode() == MODE_OFF_USB) {
		printk("no USB connection, shutting down.\n");
		return true;
	} else {
		return false;
	}
}

enum PowerAction {
	SHUTDOWN,
	REBOOT,
};

template<class KeyboardType>
PowerAction main_loop(KeyboardType *keyboard,
                      KeyboardMode mode,
                      PowerSupply<PowerSupplyPins> *power_supply,
                      ModeSwitch *mode_switch) {
	// We use the main thread to wait for power supply and mode switch
	// changes.
	while (true) {
		k_sem_take(&main_loop_event, K_FOREVER);
		if (want_shutdown(power_supply, mode_switch)) {
			return SHUTDOWN;
		}
		if (mode_switch->get_mode() != mode) {
			printk("selected mode changed from %d to %d, resetting...\n",
			       mode,
			       mode_switch->get_mode());
			return REBOOT;
		}
		if (mode_switch->get_profile() != keyboard->get_profile()) {
			keyboard->set_profile(mode_switch->get_profile());
		}
	}
}

/// Main keyboard application.
///
/// The function initializes and runs the keyboard code. When the function
/// exits, the device has been prepared for System OFF mode.
static PowerAction run_keyboard() {
	/*// TODO: remove
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
	}*/

	PowerSupplyPins power_supply_pins;
	PowerSupply<PowerSupplyPins> power_supply(&power_supply_pins);
	ModeSwitch mode_switch;

	// If the battery is low or if USB is not connected and the mode switch
	// is off, we want to immediately shut down again.
	if (want_shutdown(&power_supply, &mode_switch)) {
		return SHUTDOWN;
	}

	// The profiles selected by the mode switch are configurable at runtime
	// using FN key combinations and the selection is stored in flash, so we
	// need to load the settings.
	settings_subsys_init();
	// TODO

	// Register power supply and mode switch change listener.
	power_supply.set_callback(power_supply_mode_switch_handler);
	mode_switch.set_callback(power_supply_mode_switch_handler);

	// Initialize key matrix and mode LED.
	KeyMatrix key_matrix;
	Keys<KeyMatrix> keys(&key_matrix);
	Leds leds;

	// Run different initialization and main loop depending on the selected
	// mode.
	if (mode_switch.get_mode() == MODE_OFF_USB) {
		printk("Initializing USB keyboard...\n");
		UsbKeyboard keyboard(&keys, &leds);
		return main_loop<UsbKeyboard>(&keyboard,
		                              MODE_OFF_USB,
		                              &power_supply,
		                              &mode_switch);
	} else if (mode_switch.get_mode() == MODE_BLUETOOTH) {
		printk("Initializing bluetooth keyboard...\n");
		BluetoothKeyboard keyboard(&keys, &leds);
		return main_loop<BluetoothKeyboard>(&keyboard,
		                                    MODE_BLUETOOTH,
		                                    &power_supply,
		                                    &mode_switch);
	} else if (mode_switch.get_mode() == MODE_UNIFYING) {
		printk("Initializing unifying keyboard...\n");
		UnifyingKeyboard keyboard(&keys,
		                          &leds,
		                          mode_switch.get_profile());
		return main_loop<UnifyingKeyboard>(&keyboard,
		                                   MODE_UNIFYING,
		                                   &power_supply,
		                                   &mode_switch);
	} else {
		// This must never happen.
		throw InvalidState("invalid mode");
	}

}

void main(void) {
	PowerAction action;
	// TODO: Catch exceptions and reset the keyboard.
	try {
		action = run_keyboard();
	} catch (Exception &e) {
		printk("Exception: %s\n", e.what());
		printk("Resetting...\n");
		action = REBOOT;
	}

	if (action == REBOOT) {
		sys_reboot(SYS_REBOOT_COLD);
		printk("Reboot failed.\n");
	}
	// Enter System OFF.
	// TODO: Check again that everything was unpowered!
	printk("Switching off...\n");
	pm_power_state_force((struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
	k_sleep(K_SECONDS(1));
	printk("Shutdown failed.\n");
	while (true);
}

