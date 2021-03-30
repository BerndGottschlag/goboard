#include "keys.hpp"

#include "kernel.h"

#define ROWS 6
#define COLUMNS 16

static ScanCode key_matrix_locations[ROWS][COLUMNS] = {
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
	key_matrix->enable();
	// Pre-select the first row for the next poll() call.
	key_matrix->transfer(0x1);
}

template<class KeyMatrixType>
Keys<KeyMatrixType>::~Keys() {
	// TODO
	key_matrix->disable();
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::get_state(KeyBitmap *state) {
	// TODO
	for (uint8_t i = 0; i < 8; i++) {
		state->keys[i] = bitmap_debounced.keys[i];
	}
}

template<class KeyMatrixType>
void Keys<KeyMatrixType>::poll(int interval_ms) {
	// TODO: Should poll() return a bool to signal whether any keys have
	// changed?
	static uint16_t row_pattern = 0x1;
	KeyBitmap bitmap_temp;

	// Read the matrix:
	for (uint8_t i = 0; i < ROWS; i++) {
		// Set row_pattern to set the next row in the transfer:
		if (row_pattern != 0x20) {
			row_pattern = row_pattern << 1;
		} else {
			row_pattern = 0x1;
		}

		key_matrix->select_row();
		key_matrix->load_input();
		uint16_t temp = key_matrix->transfer(row_pattern);

		// Map keys.
		for (uint16_t j = 0; j < 16; j++) {
			if ((temp & (1 << j)) != 0) {
				bitmap_temp.set_bit(key_matrix_locations[i][j]);
			}
		}

	}

	uint8_t times_to_shift;
	/*
	// Perform debouncing:
	// Shift the changes.
	if (interval_ms >= 5) {
		times_to_shift = 4;
	}
	else {
		times_to_shift = interval_ms - 1;
	}
	for (i = times_to_shift; i > 0; i--)
	{
	}


	for (i = 0; i < 8; i ++){
		bitmap_debounced.keys
	}
	*/

	for (uint8_t i = 0; i < 8; i++) {
		bitmap_debounced.keys[i] = bitmap_temp.keys[i];
	}
}

#ifdef CONFIG_BOARD_GOBOARD_NRF52840
#include "key_matrix.hpp"
template class Keys<KeyMatrix>;
#endif

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
			clear();
			matrix_state[row] = 1 << column;
		}

		void set_two_keys(int row1,
		                  int column1,
		                  int row2,
		                  int column2) {
			clear();
			matrix_state[row1] = 1 << column1;
			matrix_state[row2] = 1 << column2;
		}

		void clear() {
			for (int i = 0; i < 6; i++) {
				matrix_state[i] = 0;
			}
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
			if (selected_row == -1) {
				zassert_true(false,
				             "load_input(), but no row selected");
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

	static void key_bitmap_test(void) {
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

		// Test iterating over bits.
		zassert_equal(bitmap.next_set_bit(0), 0,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(1), 29,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(28), 29,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(29), 29,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(31), 31,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(32), 33,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(33), 33,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(34), 255,
		              "iterating over keys failed");
		zassert_equal(bitmap.next_set_bit(255), 255,
		              "iterating over keys failed");

		// Test bit_is_set().
		zassert_equal(bitmap.bit_is_set(0), true, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(1), false, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(29), true, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(30), false, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(31), true, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(32), false, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(33), true, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(255), true, "bit_is_set failed");
		zassert_equal(bitmap.bit_is_set(256), false, "bit_is_set failed");

		// Test clearing bits.
		bitmap.clear_bit(255);
		zassert_true(bitmap.keys[7] == 0,
		             "correct bit was cleared");
		zassert_equal(bitmap.next_set_bit(34), -1,
		              "iterating over keys failed");
		bitmap.keys[1] = 0xdeadc0de;
		bitmap.clear_bit(35);
		zassert_true(bitmap.keys[1] == 0xdeadc0d6,
		             "correct bit was cleared");
		bitmap.clear_bit(29);
		zassert_true(bitmap.keys[0] == 0x80000001,
		             "correct bit was cleared");
		bitmap.clear_bit(100000);
	}

	struct SixKeyTest {
		uint8_t pressed;
		uint8_t released;
		uint8_t expected[8];
	};

	static void six_key_set_test(void) {
		// Test that keys are correctly collected from a bitmap.
		KeyBitmap bitmap;
		SixKeySet six_keys = bitmap.to_6kro();
		uint8_t good[8] = {0, 0, 0, 0, 0, 0, 0, 0};
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.set_bit(KEY_X);
		six_keys = bitmap.to_6kro();
		good[2] = KEY_X;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.set_bit(KEY_ESCAPE);
		bitmap.set_bit(KEY_A);
		bitmap.set_bit(KEY_C);
		six_keys = bitmap.to_6kro();
		good[2] = KEY_A;
		good[3] = KEY_C;
		good[4] = KEY_X;
		good[5] = KEY_ESCAPE;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.clear_bit(KEY_A);
		bitmap.set_bit(KEY_D);
		bitmap.set_bit(KEY_E);
		bitmap.set_bit(KEY_F);
		six_keys = bitmap.to_6kro();
		good[2] = KEY_C;
		good[3] = KEY_D;
		good[4] = KEY_E;
		good[5] = KEY_F;
		good[6] = KEY_X;
		good[7] = KEY_ESCAPE;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.set_bit(KEY_G);
		six_keys = bitmap.to_6kro();
		good[6] = KEY_G;
		good[7] = KEY_X;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.clear_bit(KEY_D);
		bitmap.clear_bit(KEY_E);
		bitmap.clear_bit(KEY_F);
		six_keys = bitmap.to_6kro();
		good[2] = KEY_C;
		good[3] = KEY_G;
		good[4] = KEY_X;
		good[5] = KEY_ESCAPE;
		good[6] = 0;
		good[7] = 0;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");

		// Test that modifier keys are only collected as part of the
		// modifier byte.
		bitmap.set_bit(KEY_LCTRL);
		six_keys = bitmap.to_6kro();
		good[0] = 0x1;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.set_bit(KEY_RALT);
		six_keys = bitmap.to_6kro();
		good[0] = 0x41;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.set_bit(KEY_RMETA);
		six_keys = bitmap.to_6kro();
		good[0] = 0xC1;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
		bitmap.clear_bit(KEY_RMETA);
		bitmap.clear_bit(KEY_RALT);
		bitmap.clear_bit(KEY_LCTRL);
		six_keys = bitmap.to_6kro();
		good[0] = 0x0;
		zassert_equal(memcmp(six_keys.data, good, 8), 0,
		              "empty bitmap produced wrong SixKeySet");
	}

	static void assert_single_key_pressed(KeyBitmap *bitmap,
	                                      ScanCode scan_code) {
		for (size_t key = 0; key < sizeof(bitmap->keys) * 8; key++) {
			if (bitmap->bit_is_set(key)) {
				zassert_true(key == scan_code,
				             "0x%x is pressed (should be 0x%x)",
				             key,
				             scan_code);
			}
			if (!bitmap->bit_is_set(key)) {
				zassert_true(key != scan_code,
				             "0x%x is not pressed",
				             scan_code);
			}
		}
	}

	static void assert_two_keys_pressed(KeyBitmap *bitmap,
	                                    ScanCode scan_code1,
	                                    ScanCode scan_code2) {
		for (size_t key = 0; key < sizeof(bitmap->keys) * 8; key++) {
			if (bitmap->bit_is_set(key)) {
				zassert_true(key == scan_code1 || key == scan_code2,
				             "0x%x is pressed (should be 0x%x or 0x%x)",
				             key,
				             scan_code1,
				             scan_code2);
			}
			if (!bitmap->bit_is_set(key)) {
				zassert_true(key != scan_code1 && key != scan_code2,
				             "0x%x or 0x%x is not pressed",
				             scan_code1, scan_code2);
			}
		}
	}

	static void assert_no_key_pressed(KeyBitmap *bitmap) {
		for (size_t i = 0; i < ARRAY_SIZE(bitmap->keys); i++) {
			zassert_true(bitmap->keys[i] == 0, "no keys pressed");
		}
	}

	static void key_mapping_test(void) {
		for (int row = 0; row < 6; row++) {
			for (int column = 0; column < 16; column++) {
				ScanCode scan_code = key_matrix_locations[row][column];
				MockKeyMatrix key_matrix;
				key_matrix.set_single_key(row, column);
				Keys<MockKeyMatrix> keys(&key_matrix);
				keys.poll(1);
				KeyBitmap pressed;
				keys.get_state(&pressed);
				assert_single_key_pressed(&pressed, scan_code);
			}
		}
	}

	static void key_debouncing_test(void) {
		int row = 2;
		int column = 5;
		ScanCode scan_code = key_matrix_locations[row][column];
		int row2 = 3;
		int column2 = 7;
		ScanCode scan_code2 = key_matrix_locations[row2][column2];

		KeyBitmap pressed;
		MockKeyMatrix key_matrix;
		Keys<MockKeyMatrix> keys(&key_matrix);

		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);

		// Correct timing when keys are changed with more than 5ms
		// inbetween changes.
		key_matrix.set_single_key(row, column);
		for (int i = 0; i < 5; i++) {
			keys.poll(1);
			keys.get_state(&pressed);
			assert_single_key_pressed(&pressed, scan_code);
		}
		key_matrix.clear();
		for (int i = 0; i < 5; i++) {
			keys.poll(1);
			keys.get_state(&pressed);
			assert_no_key_pressed(&pressed);
		}
		key_matrix.set_single_key(row, column);

		// Longer intervals.
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);
		key_matrix.clear();
		keys.poll(5);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);
		key_matrix.set_single_key(row, column);
		keys.poll(6);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);

		// Test whether bouncing is ignored.
		key_matrix.clear();
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);

		key_matrix.set_single_key(row, column);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);
		key_matrix.clear();
		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);
		key_matrix.set_single_key(row, column);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code);

		// Test whether bouncing is ignored for multiple keys.
		key_matrix.set_two_keys(row, column, row2, column2);
		keys.poll(2);
		keys.get_state(&pressed);
		assert_two_keys_pressed(&pressed, scan_code, scan_code2);
		key_matrix.clear();
		keys.poll(2);
		keys.get_state(&pressed);
		assert_two_keys_pressed(&pressed, scan_code, scan_code2);
		keys.poll(2);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code2);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_single_key_pressed(&pressed, scan_code2);
		keys.poll(1);
		keys.get_state(&pressed);
		assert_no_key_pressed(&pressed);
	}

	static void numpad_test(void) {
		// TODO
	}

	static void keys_tests() {
		ztest_test_suite(keys,
			ztest_unit_test(key_bitmap_test),
			ztest_unit_test(six_key_set_test),
			ztest_unit_test(key_mapping_test),
			ztest_unit_test(key_debouncing_test),
			ztest_unit_test(numpad_test)
		);
		ztest_run_test_suite(keys);
	}
	RegisterTests keys_tests_(keys_tests);
}
#endif
