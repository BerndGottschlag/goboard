/**
 * @defgroup mode_switch Mode Switch
 * @addtogroup mode_switch
 * Switch to switch between the three main modes of the device.
 * @{
 */
#ifndef MODE_SWITCH_H_INCLUDED
#define MODE_SWITCH_H_INCLUDED

/**
 * Selected mode.
 */
enum mode_switch_mode {
	MODE_SWITCH_OFF_USB,
	MODE_SWITCH_BT1,
	MODE_SWITCH_BT2,
	MODE_SWITCH_UNIFYING1,
	MODE_SWITCH_UNIFYING2,
};

/**
 * Listener for mode changes.
 *
 * The listener is called in the scheduler context.
 */
typedef void (*mode_switch_change_listener_t)(void);

/**
 * Initializes the mode switch.
 *
 * @param listener Listener which is called when the mode switch is toggled.
 */
void mode_switch_init(mode_switch_change_listener_t listener);
/**
 * Returns the current state of the mode switch.
 *
 * @returns State of the mode switch.
 */
enum mode_switch_mode mode_switch_get(void);
/**
 * Prepares the mode switch GPIOs for "system off" mode.
 *
 * Changes to the mode switch will cause the device to resume operation.
 */
void mode_switch_prepare_system_off(void);

#endif
/**
 * @}
 */

