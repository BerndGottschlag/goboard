#ifndef KEYS_HPP_INCLUDED
#define KEYS_HPP_INCLUDED

#include <sys/util.h>

#include <stdint.h>
#include <stddef.h>

/// Bitmap containing the state of all keys indexed by HID scan code.
class KeyBitmap {
public:
	/// Creates an empty bitmap with no bits set.
	KeyBitmap(): keys{0} {}

	/// Marks a single key as pressed.
	void set_bit(size_t scan_code) {
		uint32_t word = scan_code >> 5;
		if (word > ARRAY_SIZE(keys)) {
			return;
		}
		uint32_t bit = scan_code & 0x1f;
		this->keys[word] |= 1 << bit;
	}

	/// Marks a single key as released.
	void clear_bit(size_t scan_code) {
		uint32_t word = scan_code >> 5;
		if (word > ARRAY_SIZE(keys)) {
			return;
		}
		uint32_t bit = scan_code & 0x1f;
		this->keys[word] &= ~(1 << bit);
	}

	bool bit_is_set(size_t scan_code) {
		uint32_t word = scan_code >> 5;
		if (word > ARRAY_SIZE(keys)) {
			return false;
		}
		uint32_t bit = scan_code & 0x1f;
		return (this->keys[word] & (1 << bit)) != 0;
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
	Keys(KeyMatrixType *key_matrix);
	~Keys();

	/// Returns the current (debounced) state of all keys.
	void get_state(KeyBitmap *state);

	/// Polls all keys and applies debouncing.
	///
	/// @param interval_ms Milliseconds since the last call to `poll()`.
	void poll(int interval_ms);
private:
	KeyMatrixType *key_matrix;
	KeyBitmap bitmap_debounced;

	uint32_t keys_change1[8];
	uint32_t keys_change2[8];
	uint32_t keys_change3[8];
	uint32_t keys_change4[8];
	uint32_t keys_change5[8];
	uint32_t * const keys_changes[5] = {keys_change1,
	                                    keys_change2,
	                                    keys_change3,
	                                    keys_change4,
	                                    keys_change5};
};

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
class KeyMatrix;
extern template class Keys<KeyMatrix>;
#endif

#endif

