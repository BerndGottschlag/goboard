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
	///
	/// If the change callback is enabled, it may still be called during
	/// execution of the destructor.
	~ModeSwitch();

	/// Returns the selected mode.
	KeyboardMode get_mode();
	/// Returns the selected profile.
	KeyboardProfile get_profile();

	/// Sets a callback which is called whenever the state of the mode
	/// switch changes.
	///
	/// The callback is called from the system workqueue while preemption is
	/// disabled. The previous callback will never be called after this
	/// function returns.
	void set_callback(void (*change_callback)());
private:
	unsigned int get_position();

	static void sw0_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins);
	static void sw1_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins);

	static void static_on_callback(struct k_work *work);

	struct gpio_callback sw0_cb_data;
	struct gpio_callback sw1_cb_data;

	const struct device *sw0_gpio;
	const struct device *sw1_gpio;

	struct k_work_delayable callback;
	void (*callback_fn)();
};

#endif

