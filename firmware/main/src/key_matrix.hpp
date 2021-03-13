#ifndef KEY_MATRIX_HPP_INCLUDED
#define KEY_MATRIX_HPP_INCLUDED

#include <stdint.h>

/// Class which reads the key matrix via shift registers.
class KeyMatrix {
public:
	KeyMatrix();
	~KeyMatrix();

	/// Enables the key matrix.
	///
	/// This function enables power to the key matrix. The following core
	/// must not assume any specific (serial) value of the shift registers
	/// and should immediately call `transfer()` and `select_row()` to
	/// select zero or one rows.
	bool enable();

	/// Disables the key matrix and removes power from the shift registers.
	bool disable();

	/// Serial-to-parallel operation on the row selection shift register.
	void select_row();

	/// Parallel-to-serial operation on the input shift registers.
	void load_input();

	/// SPI operation which writes the specified data into the row selection
	/// shift register and returns the data from the input shift registers.
	uint16_t transfer(uint16_t out);
};

#endif

