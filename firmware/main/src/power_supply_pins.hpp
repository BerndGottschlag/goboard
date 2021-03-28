#ifndef POWER_SUPPLY_PINS_HPP_INCLUDED
#define POWER_SUPPLY_PINS_HPP_INCLUDED

#include <stdint.h>

#include <drivers/gpio.h>

/// Hardware-specific part of the power supply code.
class PowerSupplyPins {
public:
	PowerSupplyPins();
	/// Destructor which prepares the power supply for System OFF mode.
	///
	/// The system must wake up when it is connected to USB.
	~PowerSupplyPins();

	void measure_battery_voltage(uint32_t *low, uint32_t *high);
	void configure_charging(bool active);
	void configure_discharging(bool low, bool high);
	bool has_usb_connection(void);
private:
	static const struct device *init_gpio(const char *label,
	                                      gpio_pin_t pin,
	                                      gpio_flags_t flags);

	const struct device *vbatt_power_gpio;
	const struct device *charge_gpio;
	const struct device *discharge_low_gpio;
	const struct device *discharge_high_gpio;
};

#endif

