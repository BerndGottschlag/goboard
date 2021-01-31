
#ifndef MODE_SWITCH_H_INCLUDED
#define MODE_SWITCH_H_INCLUDED

/**
 * Selected mode.
 */
enum mode_switch_mode {
	MODE_SWITCH_OFF_USB,
	MODE_SWITCH_BT1,
	MODE_SWITCH_BT2,
};

/**
 * Listener for mode changes.
 *
 * The listener is called in the scheduler context.
 */
typedef void (*mode_switch_change_listener_t)(void);

void mode_switch_init(mode_switch_change_listener_t listener);
enum mode_switch_mode mode_switch_get(void);
void mode_switch_prepare_system_off(void);

#endif


