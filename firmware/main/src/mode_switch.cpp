#include "mode_switch.hpp"

#include "exception.hpp"

#define MODESW0_NODE DT_ALIAS(modesw0)
#define MODESW1_NODE DT_ALIAS(modesw1)

#if !DT_NODE_HAS_STATUS(MODESW0_NODE, okay) || !DT_NODE_HAS_STATUS(MODESW1_NODE, okay)
//#error No mode switch defined in device tree, did you select the right board?
#endif


#define MODESW0_GPIO_LABEL DT_GPIO_LABEL(MODESW0_NODE, gpios)
#define MODESW0_GPIO_PIN   DT_GPIO_PIN(MODESW0_NODE, gpios)
#define MODESW0_GPIO_FLAGS (GPIO_INPUT | DT_GPIO_FLAGS(MODESW0_NODE, gpios))

#define MODESW1_GPIO_LABEL DT_GPIO_LABEL(MODESW1_NODE, gpios)
#define MODESW1_GPIO_PIN   DT_GPIO_PIN(MODESW1_NODE, gpios)
#define MODESW1_GPIO_FLAGS (GPIO_INPUT | DT_GPIO_FLAGS(MODESW1_NODE, gpios))

ModeSwitch::ModeSwitch() {
	/*const struct device *sw0 = device_get_binding(MODESW0_GPIO_LABEL);
	if (sw0 == NULL) {
		throw InitializationFailed("Did not find modesw0.");
	}
	const struct device *sw1 = device_get_binding(MODESW1_GPIO_LABEL);
	if (sw1 == NULL) {
		throw InitializationFailed("Did not find modesw0.");
	}

	int ret = gpio_pin_configure(sw0, MODESW0_GPIO_PIN, MODESW0_GPIO_FLAGS);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw0.");
	}
	ret = gpio_pin_configure(sw1, MODESW1_GPIO_PIN, MODESW1_GPIO_FLAGS);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw1.");
	}

	ret = gpio_pin_interrupt_configure(sw0,
	                                   MODESW0_GPIO_PIN,
	                                   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw0 interrupt.");
	}
	ret = gpio_pin_interrupt_configure(sw1,
	                                   MODESW1_GPIO_PIN,
	                                   GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		throw InitializationFailed("Failed to configure modesw1 interrupt.");
	}

	gpio_init_callback(&this->sw0_cb_data,
	                   ModeSwitch::sw0_gpio_callback,
	                   BIT(MODESW0_GPIO_PIN));
	gpio_add_callback(sw0, &this->sw0_cb_data);
	gpio_init_callback(&this->sw1_cb_data,
	                   ModeSwitch::sw1_gpio_callback,
	                   BIT(MODESW1_GPIO_PIN));
	gpio_add_callback(sw1, &this->sw1_cb_data);*/

	// TODO
}

ModeSwitch::~ModeSwitch() {
	// TODO
}

KeyboardMode ModeSwitch::get_mode() {
	// TODO
	return MODE_OFF_USB;
}

KeyboardProfile ModeSwitch::get_profile() {
	// TODO
	return PROFILE_1;
}

void ModeSwitch::set_callback(void (*change_callback)()) {
	(void)change_callback;
	// TODO
}

void ModeSwitch::sw0_gpio_callback(struct device *port, struct gpio_callback *cb, uint32_t pins) {
	(void)port;
	(void)pins;
	ModeSwitch *ms = CONTAINER_OF(cb, ModeSwitch, sw0_cb_data);
	ms->gpio_callback();
}

void ModeSwitch::sw1_gpio_callback(struct device *port, struct gpio_callback *cb, uint32_t pins) {
	(void)port;
	(void)pins;
	ModeSwitch *ms = CONTAINER_OF(cb, ModeSwitch, sw0_cb_data);
	ms->gpio_callback();
}

void ModeSwitch::gpio_callback() {
	// TODO
}

