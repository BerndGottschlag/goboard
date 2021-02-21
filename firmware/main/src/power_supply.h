/**
 * @defgroup power_supply Power Supply
 * @addtogroup power_supply
 * Power supply and battery management.
 * @{
 */
#ifndef POWER_SUPPLY_H_INCLUDED
#define POWER_SUPPLY_H_INCLUDED

#include <stdint.h>

/**
 * Current mode of the power supply system.
 */
enum power_supply_mode {
	/**
	 * The batteries are not low, and we are not charging the batteries.
	 */
	POWER_SUPPLY_NORMAL,
	/**
	 * We are currently charging the batteries.
	 */
	POWER_SUPPLY_CHARGING,
	/**
	 * We are not charging the batteries, and the voltage is low. In this
	 * state, the system should transition into system-off mode.
	 */
	POWER_SUPPLY_LOW
};

/**
 * State of the power supply system.
 */
struct power_supply_state {
	/**
	 * Current battery management mode.
	 */
	enum power_supply_mode mode;
	/**
	 * Current estimated battery charge in percent.
	 */
	uint8_t charge;
};

/**
 * Listener for power supply state changes.
 *
 * The listener is called in the scheduler context.
 */
typedef void (*power_supply_state_change_listener_t)(void);

/**
 * Initializes the battery management system.
 *
 * @param listener Listener that is called when the state of the battery
 * management system changes.
 */
void power_supply_init(power_supply_state_change_listener_t listener);
/**
 * Returns the current mode of the battery management system.
 *
 * @returns Current mode of the battery management system.
 */
enum power_supply_mode power_supply_get_mode(void);
/**
 * Returns the state of charge of the battery.
 *
 * @returns Charge left in the battery as a value between 0 (empty) and 100 (full).
 */
uint8_t battery_charge(void);
/**
 * Prepares the power management system for "system off" mode.
 *
 * The function ensures that the system wakes up when USB is connected.
 */
void power_supply_prepare_system_off(void);

#endif
/**
 * @}
 */

