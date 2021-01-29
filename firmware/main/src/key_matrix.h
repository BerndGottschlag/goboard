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
	uint8_t modifiers;
	uint8_t key;
	uint16_t pressed;
};

void key_matrix_init(void);

size_t key_matrix_read(struct key_event *buffer, size_t length);
void key_matrix_reset(void);


#endif

