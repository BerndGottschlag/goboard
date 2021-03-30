#ifndef KEYS_HPP_INCLUDED
#define KEYS_HPP_INCLUDED

#include <sys/util.h>

#include <stdint.h>
#include <stddef.h>

/// Bitmap containing the state of all keys indexed by HID scan code.
class KeyBitmap {
public:
	/// Creates an empty bitmap with no bits set.
	KeyBitmap() {}

	/// Marks a single key as pressed.
	void set_bit(size_t scan_code) {
		uint32_t word = scan_code >> 5;
		if (word >= ARRAY_SIZE(keys)) {
			return;
		}
		uint32_t bit = scan_code & 0x1f;
		this->keys[word] |= 1 << bit;
	}

	/// Marks a single key as released.
	void clear_bit(size_t scan_code) {
		uint32_t word = scan_code >> 5;
		if (word >= ARRAY_SIZE(keys)) {
			return;
		}
		uint32_t bit = scan_code & 0x1f;
		this->keys[word] &= ~(1 << bit);
	}

	/// Tests whether a single key is pressed.
	bool bit_is_set(size_t scan_code) {
		uint32_t word = scan_code >> 5;
		if (word >= ARRAY_SIZE(keys)) {
			return false;
		}
		uint32_t bit = scan_code & 0x1f;
		return (this->keys[word] & (1 << bit)) != 0;
	}

	/// Finds the next pressed key, starting at the specified position.
	///
	/// The position specified by `start` is included by the search. If no
	/// pressed key is found, the function returns -1.
	int next_set_bit(unsigned int start) {
		while (start < sizeof(keys) * 8) {
			uint32_t word = start >> 5;
			uint32_t bit = start & 0x1f;
			uint32_t mask = ~((1 << bit) - 1);
			uint32_t bits = keys[word] & mask;
			uint32_t set_bit = __builtin_ffs(bits);
			if (set_bit != 0) {
				return (word << 5) + set_bit - 1;
			}
			// No bit found in this word, try the next.
			start = (word + 1) << 5;
		}
		return -1;
	}

	bool operator==(const KeyBitmap &other) {
		for (size_t i = 0; i < ARRAY_SIZE(keys); i++) {
			if (keys[i] != other.keys[i]) {
				return false;
			}
		}
		return true;
	}

	/// Bitmap containing the key state.
	///
	/// A set bit indicates a pressed key.
	uint32_t keys[8] = {0};
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

