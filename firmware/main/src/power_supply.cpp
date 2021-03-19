#include "power_supply.hpp"

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
// On the real hardware, charge the batteries for 10 seconds at a time.
#define CHARGING_DURATION K_SECONDS(10)
// On the real hardware, wait for 1000ms to let the battery voltage recover
// before measurements.
#define RECOVERY_DURATION K_SECONDS(1)
#else
// For tests, drastically reduce the times above to reduce test durations.
#define CHARGING_DURATION K_MSEC(10)
#define RECOVERY_DURATION K_MSEC(1)
#endif

template<class PowerSupplyPinType>
PowerSupply<PowerSupplyPinType>::PowerSupply(PowerSupplyPinType *pins):
		pins(pins) {
	// TODO
}

template<class PowerSupplyPinType>
PowerSupply<PowerSupplyPinType>::~PowerSupply() {
	// TODO
}

template<class PowerSupplyPinType>
PowerSupplyMode PowerSupply<PowerSupplyPinType>::get_mode() {
	// TODO
}

template<class PowerSupplyPinType>
uint8_t PowerSupply<PowerSupplyPinType>::get_battery_charge() {
	// TODO
}

template<class PowerSupplyPinType>
void set_callback(void (*change_callback)()) {
	// TODO
}

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
#include "power_supply_pins.hpp"
template class PowerSupply<PowerSupplyPins>;
#endif

#ifndef CONFIG_BOARD_GOBOARD_NRF52840
#include "tests.hpp"
#include <ztest.h>
namespace tests {
	// TODO
}
#endif

