#ifndef MODE_SWITCH_HPP_INCLUDED
#define MODE_SWITCH_HPP_INCLUDED

#include <drivers/gpio.h>

#include <stdint.h>

enum KeyboardMode {
	MODE_OFF_USB,
	MODE_BLUETOOTH,
	MODE_UNIFYING,
};

enum KeyboardProfile {
	PROFILE_1,
	PROFILE_2,
};

/// Switch to switch between the three main modes of the device.
class ModeSwitch {
public:
	ModeSwitch();
	/// Destructor which prepares the mode switch for System OFF mode.
	~ModeSwitch();

	/// Returns the selected mode.
	KeyboardMode get_mode();
	/// Returns the selected profile.
	KeyboardProfile get_profile();

	// Sets a callback which is called whenever the state of the mode
	// switch changes.
	//
	// The callback is called from an interrupt handler. The previous
	// callback may still be called during the call to this function, but
	// will never be called after this function returns. Similarly, the
	// destructor may still call the callback registered at the time.
	void set_callback(void (*change_callback)());
private:
	static void sw0_gpio_callback(struct device *port, struct gpio_callback *cb, uint32_t pins);
	static void sw1_gpio_callback(struct device *port, struct gpio_callback *cb, uint32_t pins);
	void gpio_callback();

	struct gpio_callback sw0_cb_data;
	struct gpio_callback sw1_cb_data;
};

#endif

