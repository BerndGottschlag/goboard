/**
 * @defgroup keys Keys
 * @addtogroup keys
 * Key matrix polling and debouncing, numpad connection handling.
 *
 * The switches used on the keyboard have a maximum bounce time of five
 * milliseconds. The code therefore polls the key matrix once per millisecond
 * and only changes the assumed key state if the value of a key has been
 * identical for five samples in a row.
 * @{
 */
#ifndef KEYS_H_INCLUDED
#define KEYS_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "keycodes.h"

/**
 * Bitmap containing the state of all keys indexed by HID scan code.
 */
struct key_bitmap {
	/**
	 * Bitmap containing the key state.
	 *
	 * A set bit indicates a pressed key.
	 */
	uint32_t keys[8];
};
/**
 * Initializer which creates an empty key_bitmap with no keys set.
 */
#define EMPTY_KEY_BITMAP { .keys = { 0 } }


/**
 * Marks a single key as pressed.
 *
 * @param bitmap Bitmap where the key is marked.
 * @param scancode Code of the key.
 */
void key_bitmap_set(struct key_bitmap *bitmap, int scancode);

/**
 * Marks a single key as released.
 *
 * @param bitmap Bitmap where the key is marked.
 * @param scancode Code of the key.
 */
void key_bitmap_clear(struct key_bitmap *bitmap, int scancode);

/**
 * Legacy HID key information (modifiers + six scan codes).
 */
struct legacy_key_list {
	/** Modifier byte as defined by USB HID. */
	uint8_t modifiers;
	/**
	 * Up to six keys that are pressed.
	 *
	 * Elements with a zero value indicate fewer pressed keys. The entries
	 * are filled in order - a zero element is never followed by a valid key
	 * scan code.
	 */
	uint8_t scancodes[6];
};

/**
 * Converts a NKRO key bitset into a legacy key list.
 *
 * The function retrieves the first six bits that are set in the bitmap as well
 * as the modifier keys.
 *
 * @param bitmap Bitmap which is scanned for keys.
 * @param list List that is filled with the pressed keys.
 */
void legacy_key_list_from_bitmap(const struct key_bitmap *bitmap,
                                 struct legacy_key_list *list);

/**
 * Initializes the key matrix and the connection to the numpad.
 */
void keys_init(void);

/**
 * Supplies power to the key matrix.
 */
void keys_activate(void);

/**
 * Stops supplying power to the key matrix.
 */
void keys_deactivate(void);

/**
 * Activates the numpad connection.
 *
 * After this function is called, the code periodically samples the ID line of
 * the numpad connection. Depending on the state of the line, power to the
 * numpad is enabled or disabled and the code starts to listen for keystrokes
 * on the numpad UART.
 *
 * This function must be called after `keys_activate()` and before
 * `keys_deactivate()`.
 */
void keys_activate_numpad(void);

/**
 * Deactivates the numpad connection.
 */
void keys_deactivate_numpad(void);

/**
 * Changes the state of the num lock LED.
 *
 * The state is memorized even when the numpad is not attached.
 *
 * @param state True if the num lock LED shall be lit.
 */
void keys_set_num_lock_led(bool state);

/**
 * Copies the (debounced) state of the keys into the specified bitmap.
 *
 * @param bitmap Bitmap that is filled by the function.
 */
void keys_get_state(struct key_bitmap *bitmap);

/**
 * Polls the key matrix and checks for numpad input.
 *
 * This function normally has to be called once per millisecond. If the firmware
 * wants to conserve power and if no key is pressed, the caller is allowed to
 * wait at least five milliseconds between calls and has to set pause to true.
 * In this case, the code performs modified debouncing.
 *
 * @param pause True if there has been a pause of at least five milliseconds
 *              since the last call.
 *
 * @returns True if the state of the keys has changed since the last call, i.e.,
 *          keys have been pressed or released.
 */
bool keys_poll(bool pause);

/**
 * Prepares the keys for "system off" mode.
 *
 * The numpad is deactivated so that it does not wake the device up.
 */
void keys_prepare_system_off(void);

#endif
/**
 * @}
 */


