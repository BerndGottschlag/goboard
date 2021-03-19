#ifndef POWER_SUPPLY_PINS_HPP_INCLUDED
#define POWER_SUPPLY_PINS_HPP_INCLUDED

#include <stdint.h>

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
};

#endif

