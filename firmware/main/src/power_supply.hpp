#ifndef POWER_SUPPLY_HPP_INCLUDED
#define POWER_SUPPLY_HPP_INCLUDED

#include <stdint.h>
#include <stddef.h>

/// Current mode of the power supply system.
enum PowerSupplyMode {
	/// The batteries are not low, and we are not charging the batteries.
	POWER_SUPPLY_NORMAL,
	/// We are currently charging the batteries.
	POWER_SUPPLY_CHARGING,
	/// We are not charging the batteries, and the voltage is low. In this
	/// state, the system should transition into system-off mode.
	POWER_SUPPLY_LOW
};

/// Power supply and battery management.
template<class PowerSupplyPinType> class PowerSupply {
public:
	PowerSupply(PowerSupplyPinType *pins);
	~PowerSupply();

	/// Returns the current mode of the power supply.
	PowerSupplyMode get_mode();

	/// Returns the estimated battery charge in percent.
	uint8_t get_battery_charge();

	// Sets a callback which is called whenever the state of the power
	// supply (mode or charge percentage) changes.
	//
	// The callback is called from a separate thread, so the code has to be
	// thread-safe. The previous callback may still be called during the
	// call to this function, but will never be called after this function
	// returns. Similarly, the destructor may still call the callback
	// registered at the time.
	void set_callback(void (*change_callback)());
private:
	PowerSupplyPinType *pins;
	void (*change_callback)() = NULL;
};

#endif

