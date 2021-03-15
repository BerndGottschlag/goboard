#include "keys.hpp"

#include "scan_code.hpp"

static ScanCode key_matrix_locations[6][16] = {
	{
		KEY_PAUSE,
		KEY_SCROLL_LOCK,
		KEY_F12,
		KEY_PRINT,
		KEY_F11,
		KEY_F10,
		KEY_F8,
		KEY_F9,
		KEY_F7,
		KEY_F6,
		KEY_F5,
		KEY_F4,
		KEY_F3,
		KEY_F2,
		KEY_ESCAPE,
		KEY_F1,
	},
	{
		KEY_INSERT,
		KEY_DELETE,
		KEY_EQUAL,
		KEY_BACKSPACE,
		KEY_MINUS,
		KEY_0,
		KEY_8,
		KEY_9,
		KEY_7,
		KEY_6,
		KEY_5,
		KEY_4,
		KEY_3,
		KEY_2,
		KEY_GRAVE,
		KEY_1,
	},
	{
		KEY_INVALID,
		KEY_INVALID,
		KEY_RIGHT_BRACKET,
		KEY_RETURN,
		KEY_LEFT_BRACKET,
		KEY_P,
		KEY_I,
		KEY_O,
		KEY_U,
		KEY_Y,
		KEY_T,
		KEY_R,
		KEY_E,
		KEY_W,
		KEY_TAB,
		KEY_Q,
	},
	{
		KEY_PAGE_UP,
		KEY_INVALID,
		KEY_BACKSLASH,
		KEY_INVALID,
		KEY_QUOTE,
		KEY_SEMICOLON,
		KEY_K,
		KEY_L,
		KEY_J,
		KEY_H,
		KEY_G,
		KEY_F,
		KEY_D,
		KEY_S,
		KEY_CAPS_LOCK,
		KEY_A,
	},
	{
		KEY_PAGE_DOWN,
		KEY_UP,
		KEY_RSHIFT,
		KEY_INVALID,
		KEY_SLASH,
		KEY_PERIOD,
		KEY_M,
		KEY_COMMA,
		KEY_N,
		KEY_B,
		KEY_V,
		KEY_C,
		KEY_X,
		KEY_Z,
		KEY_LSHIFT,
		KEY_INVALID, // TODO: Fn!
	},
	{
		KEY_RIGHT,
		KEY_DOWN,
		KEY_RCTRL,
		KEY_LEFT,
		KEY_APPLICATION,
		KEY_RALT,
		KEY_INVALID,
		KEY_INVALID,
		KEY_INVALID,
		KEY_SPACE,
		KEY_INVALID,
		KEY_INVALID,
		KEY_INVALID,
		KEY_LALT,
		KEY_LCTRL,
		KEY_LMETA,
	},
};

template<class KeyMatrixType>
Keys<KeyMatrixType>::Keys(KeyMatrixType *key_matrix): key_matrix(key_matrix) {
	// TODO
}

template<class KeyMatrixType>
Keys<KeyMatrixType>::~Keys() {
	// TODO
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::get_state(KeyBitmap *state) {
	// TODO
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::poll(int interval_ms) {
	// TODO: Should poll() return a bool to signal whether any keys have
	// changed?
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
	/// Artificial key matrix which allows setting the key state as part of
	/// tests.
	class MockKeyMatrix {
	public:
		MockKeyMatrix() {
		}

		void set_single_key(int row, int column) {
			for (int i = 0; i < 6; i++) {
				matrix_state[i] = 0;
			}
			matrix_state[row] = 1 << column;
		}

		void enable() {
			enabled = true;
			// The input shift registers lost their content.
			input_reg_state = 0xdead;
			output_reg_state = 0xa5;
			selected_row = -1;
		}

		void disable() {
			enabled = true;
		}

		void select_row() {
			if (!enabled) {
				zassert_true(false,
				             "select_row() on inactive matrix");
			}
			if (__builtin_popcount(output_reg_state) != 1) {
				zassert_true(false,
				             "not exactly one row selected");
			}
			selected_row = __builtin_ctz(output_reg_state);
		}

		void load_input() {
			if (!enabled) {
				zassert_true(false,
				             "load_input() on inactive matrix");
			}
			if (selected_row == 0) {
				zassert_true(false,
				             "transfer() on inactive matrix");
			}
			input_reg_state = matrix_state[selected_row];
		}

		uint16_t transfer(uint16_t out) {
			if (!enabled) {
				zassert_true(false,
				             "transfer() on inactive matrix");
			}
			// TODO: Check endianess! The upper 8 bits stay in the
			// register!
			output_reg_state = out & 0x3f;
			uint16_t result = input_reg_state;
			input_reg_state = 0xdead;
			return result;
		}
	private:
		bool enabled = false;

		uint16_t matrix_state[6] = {0};
		uint16_t input_reg_state = 0;
		uint8_t output_reg_state = 0;
		int selected_row = -1;
	};

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

		// Test bit_is_set().
		// TODO
	}

	void key_mapping_test(void) {
		for (int row = 0; row < 6; row++) {
			for (int column = 0; column < 16; column++) {
				ScanCode scan_code = key_matrix_locations[row][column];
				MockKeyMatrix key_matrix;
				key_matrix.set_single_key(row, column);
				Keys<MockKeyMatrix> keys(&key_matrix);
				keys.poll(1);
				KeyBitmap pressed;
				keys.get_state(&pressed);
				for (size_t key = 0; key < sizeof(pressed.keys) * 8; key++) {
					if (pressed.bit_is_set(key)) {
						zassert_true(key == scan_code,
						             "%d is pressed (should be %d)",
						             key,
						             scan_code);
					}
					if (!pressed.bit_is_set(key)) {
						zassert_true(key != scan_code,
						             "%d is not pressed",
						             scan_code);
					}
				}
			}
		}
		// TODO
	}

	void key_debouncing_test(void) {
		// TODO
	}

	void numpad_test(void) {
		// TODO
	}

	void keys_tests() {
		ztest_test_suite(keys,
			ztest_unit_test(key_bitmap_test),
			ztest_unit_test(key_mapping_test),
			ztest_unit_test(key_debouncing_test),
			ztest_unit_test(numpad_test)
		);
		ztest_run_test_suite(keys);
	}
	RegisterTests tests(keys_tests);
}
#endif
