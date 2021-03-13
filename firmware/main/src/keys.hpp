#ifndef KEYS_HPP_INCLUDED
#define KEYS_HPP_INCLUDED

#include <stdint.h>
#include <stddef.h>

/// Bitmap containing the state of all keys indexed by HID scan code.
class KeyBitmap {
	/// Creates an empty bitmap with no bits set.
	KeyBitmap(): keys{0} {}

	/// Marks a single key as pressed.
	void set_bit(size_t scan_code) {
		// TODO
	}

	/// Marks a single key as released.
	void clear_bit(size_t scan_code) {
		// TODO
	}

	///Bitmap containing the key state.
	///
	///A set bit indicates a pressed key.
	uint32_t keys[8];
};

/// Key state collection and debouncing.
///
/// This class collects all keys from the main key matrix and the numpad, and it
/// implements 5ms switch debouncing for the former.
template<class KeyMatrixType> class Keys {
public:
	// TODO: Numpad connection.
	Keys(KeyMatrixType &key_matrix);
	~Keys();

	/// Returns the current (debounced) state of all keys.
	void get_state(KeyBitmap &state);
private:
	KeyMatrixType &key_matrix;
};

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
class KeyMatrix;
extern template class Keys<KeyMatrix>;
#endif

#endif

