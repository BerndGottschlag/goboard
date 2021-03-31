#include "mode_switch.hpp"

#include "exception.hpp"

#include <hal/nrf_gpio.h>

#define MODESW0 DT_ALIAS(modesw0)
#define MODESW1 DT_ALIAS(modesw1)

#define MODESW0_LABEL DT_GPIO_LABEL(MODESW0, gpios)
#define MODESW0_PIN   DT_GPIO_PIN(MODESW0, gpios)
#define MODESW0_FLAGS (GPIO_INPUT | DT_GPIO_FLAGS(MODESW0, gpios))

#define MODESW1_LABEL DT_GPIO_LABEL(MODESW1, gpios)
#define MODESW1_PIN   DT_GPIO_PIN(MODESW1, gpios)
#define MODESW1_FLAGS (GPIO_INPUT | DT_GPIO_FLAGS(MODESW1, gpios))

#define DEBOUNCE_INTERVAL K_MSEC(200)

ModeSwitch::ModeSwitch() {
	// Initialize the GPIOs.
	sw0_gpio = device_get_binding(MODESW0_LABEL);
	if (sw0_gpio == NULL) {
		throw InitializationFailed("Did not find modesw0.");
	}
	sw1_gpio = device_get_binding(MODESW1_LABEL);
	if (sw1_gpio == NULL) {
		throw InitializationFailed("Did not find modesw0.");
	}

	int ret = gpio_pin_configure(sw0_gpio, MODESW0_PIN, MODESW0_FLAGS);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw0.");
	}
	ret = gpio_pin_configure(sw1_gpio, MODESW1_PIN, MODESW1_FLAGS);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw1.");
	}

	// The change callback is delayed to debounce the switch and to generate
	// clean transitions between the switch positions.
	k_work_init_delayable(&callback, static_on_callback);

	// Initialize the GPIO interrupts.
	ret = gpio_pin_interrupt_configure(sw0_gpio,
	                                   MODESW0_PIN,
	                                   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw0 interrupt.");
	}
	ret = gpio_pin_interrupt_configure(sw1_gpio,
	                                   MODESW1_PIN,
	                                   GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw1 interrupt.");
	}

	gpio_init_callback(&sw0_cb_data,
	                   ModeSwitch::sw0_gpio_callback,
	                   BIT(MODESW0_PIN));
	gpio_add_callback(sw0_gpio, &sw0_cb_data);
	gpio_init_callback(&sw1_cb_data,
	                   ModeSwitch::sw1_gpio_callback,
	                   BIT(MODESW1_PIN));
	gpio_add_callback(sw1_gpio, &sw1_cb_data);
}

ModeSwitch::~ModeSwitch() {
	int position = get_position();
	// Disable the interrupts - the callback must not called after this
	// object has been deallocated.
	gpio_add_callback(sw0_gpio, &sw0_cb_data);
	gpio_add_callback(sw1_gpio, &sw1_cb_data);
	gpio_pin_interrupt_configure(sw0_gpio,
	                             MODESW0_PIN,
	                             GPIO_INT_DISABLE);
	gpio_pin_interrupt_configure(sw1_gpio,
	                             MODESW1_PIN,
	                             GPIO_INT_DISABLE);

	// Prevent any further callbacks.
	k_work_sync sync;
	k_work_cancel_delayable_sync(&callback, &sync);

	// Configure the switch to wake the system up.
	nrf_gpio_cfg_input(MODESW0_PIN, NRF_GPIO_PIN_NOPULL);
	nrf_gpio_cfg_input(MODESW1_PIN, NRF_GPIO_PIN_NOPULL);
	if (position == 0) {
		nrf_gpio_cfg_sense_set(MODESW0_PIN, NRF_GPIO_PIN_SENSE_HIGH);
		nrf_gpio_cfg_sense_set(MODESW1_PIN, NRF_GPIO_PIN_SENSE_HIGH);
	} else if (position == 1) {
		nrf_gpio_cfg_sense_set(MODESW0_PIN, NRF_GPIO_PIN_SENSE_LOW);
		nrf_gpio_cfg_sense_set(MODESW1_PIN, NRF_GPIO_PIN_SENSE_HIGH);
	} else {
		nrf_gpio_cfg_sense_set(MODESW0_PIN, NRF_GPIO_PIN_SENSE_HIGH);
		nrf_gpio_cfg_sense_set(MODESW1_PIN, NRF_GPIO_PIN_SENSE_LOW);
	}

}

KeyboardMode ModeSwitch::get_mode() {
	// TODO: Make the radio mode configurable.
	int position = get_position();
	switch (position) {
	case 1:
		return MODE_BLUETOOTH;
	case 2:
		return MODE_BLUETOOTH;
	default:
		return MODE_OFF_USB;
	}
}

KeyboardProfile ModeSwitch::get_profile() {
	int position = get_position();
	switch (position) {
	case 2:
		return PROFILE_2;
	default:
		return PROFILE_1;
	}
}

void ModeSwitch::set_callback(void (*change_callback)()) {
	k_sched_lock();
	callback_fn = change_callback;
	k_sched_unlock();
}

unsigned int ModeSwitch::get_position() {
	int status = gpio_pin_get(sw0_gpio, MODESW0_PIN);
	if (status < 0) {
		throw HardwareError("failed to get mode switch state");
	} else if (status) {
		return 1;
	}
	status = gpio_pin_get(sw1_gpio, MODESW1_PIN);
	if (status < 0) {
		throw HardwareError("failed to get mode switch state");
	} else if (status) {
		return 2;
	}

	return 0;
}

void ModeSwitch::sw0_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
	(void)port;
	(void)pins;
	ModeSwitch *ms = CONTAINER_OF(cb, ModeSwitch, sw0_cb_data);
	// Debounce the switch by waiting for some time before calling the
	// callback.
	k_work_schedule(&ms->callback, DEBOUNCE_INTERVAL);
}

void ModeSwitch::sw1_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
	(void)port;
	(void)pins;
	ModeSwitch *ms = CONTAINER_OF(cb, ModeSwitch, sw1_cb_data);
	// Debounce the switch by waiting for some time before calling the
	// callback.
	k_work_schedule(&ms->callback, DEBOUNCE_INTERVAL);
}

void ModeSwitch::static_on_callback(struct k_work *work) {
	ModeSwitch *thisptr = CONTAINER_OF(k_work_delayable_from_work(work),
	                                   ModeSwitch,
	                                   callback);
	if (thisptr->callback_fn) {
		k_sched_lock();
		thisptr->callback_fn();
		k_sched_unlock();
	}
}

