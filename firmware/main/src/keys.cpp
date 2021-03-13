#include "keys.hpp"

template<class KeyMatrixType>
Keys<KeyMatrixType>::Keys(KeyMatrixType &key_matrix): key_matrix(key_matrix) {
	// TODO
}

template<class KeyMatrixType>
Keys<KeyMatrixType>::~Keys() {
	// TODO
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::get_state(KeyBitmap &state) {
	// TODO
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::poll(int interval_ms) {
	// TODO
}

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
#include "key_matrix.hpp"
template class Keys<KeyMatrix>;
#endif

// Tests:
// - Key bitmap
// - Debouncing
// - Correct switch/scan code conversion
//

#ifndef CONFIG_BOARD_GOBOARD_NRF52840
#include "tests.hpp"
#include <ztest.h>
namespace tests {
	void key_bitmap_test(void) {
		// Test the initial state.
		KeyBitmap bitmap;
		for (size_t i = 0; i < ARRAY_SIZE(bitmap.keys); i++) {
			zassert_true(bitmap.keys[i] == 0,
			             "bitmap is initially empty");
		}

		// Test setting bits.
		bitmap.set_bit(0);
		bitmap.set_bit(29);
		bitmap.set_bit(31);
		bitmap.set_bit(33);
		bitmap.set_bit(255);
		bitmap.set_bit(100000);
		uint32_t good[8] = {0xa0000001, 0x2, 0x0, 0x0,
		                    0x0, 0x0, 0x0, 0x80000000};
		for (size_t i = 0; i < ARRAY_SIZE(bitmap.keys); i++) {
			zassert_true(bitmap.keys[i] == good[i],
			             "correct bits are set");
		}

		// Test clearing bits.
		bitmap.keys[1] = 0xdeadc0de;
		bitmap.clear_bit(35);
		zassert_true(bitmap.keys[1] == 0xdeadc0d6,
		             "correct bit was cleared");
		bitmap.clear_bit(29);
		zassert_true(bitmap.keys[0] == 0x80000001,
		             "correct bit was cleared");
		bitmap.set_bit(100000);
	}

	void keys_test(void) {
		// Test correct mapping between switches and key codes.
		// TODO

		// Test correct debouncing.
		// TODO
	}

	void keys_tests() {
		ztest_test_suite(keys,
			ztest_unit_test(key_bitmap_test),
			ztest_unit_test(keys_test)
		);
		ztest_run_test_suite(keys);
	}
	RegisterTests tests(keys_tests);
}
#endif
