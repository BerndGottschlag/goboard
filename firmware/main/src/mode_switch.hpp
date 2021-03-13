#ifndef MODE_SWITCH_HPP_INCLUDED
#define MODE_SWITCH_HPP_INCLUDED

#include <drivers/gpio.h>

#include <stdint.h>

/// Switch to switch between the three main modes of the device.
class ModeSwitch {
public:
	ModeSwitch();
	/// Destructor which prepares the mode switch for System OFF mode.
	~ModeSwitch();

	// TODO: Functions to get the state of the switch and to register a callback.
private:
	static void sw0_gpio_callback(struct device *port, struct gpio_callback *cb, uint32_t pins);
	static void sw1_gpio_callback(struct device *port, struct gpio_callback *cb, uint32_t pins);
	void gpio_callback();

	struct gpio_callback sw0_cb_data;
	struct gpio_callback sw1_cb_data;
};

#endif

