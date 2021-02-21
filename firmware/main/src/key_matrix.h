/**
 * @defgroup key_matrix Key Matrix
 * @addtogroup key_matrix
 * Key matrix polling and debouncing.
 * @{
 */
#ifndef KEY_MATRIX_H_INCLUDED
#define KEY_MATRIX_H_INCLUDED

#include <stdint.h>
#include <stddef.h>


/**
 * Single key event.
 *
 * The buffer code in key_matrix.c assumes that the size of this struct is a
 * power of two.
 */
struct key_event {
	/**
	 * Key modifiers as defined by USB HID.
	 */
	uint8_t modifiers;
	/**
	 * Single key scan code.
	 */
	uint8_t key;
	/* TODO: What type of scan code? */
	/**
	 * State of the key (1 = pressed).
	 */
	uint16_t pressed;
};

/**
 * Initializes the key matrix code.
 */
void key_matrix_init(void);

/**
 * Reads key events.
 *
 * @param buffer Buffer into which key events are written.
 * @param length Length of the buffer (maximum number of key events).
 *
 * @returns Number of key events read.
 */
size_t key_matrix_read(struct key_event *buffer, size_t length);
/**
 * Resets the key matrix.
 *
 * After the reset, all keys are assumed to not be pressed.
 */
void key_matrix_reset(void);


#endif
/**
 * @}
 */

