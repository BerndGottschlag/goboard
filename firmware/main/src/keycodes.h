/**
 * @defgroup keycodes Key Codes
 * @addtogroup keycodes
 * Keycodes for USB HID.
 * @{
 */
#ifndef KEYCODES_H_INCLUDED
#define KEYCODES_H_INCLUDED

/**
 * Modifier bits in the HID modifier byte.
 */
enum modifier_key {
	/** Left control key. */
	KEY_MOD_LCTRL = 0x1,
	/** Left shift key. */
	KEY_MOD_LSHIFT = 0x2,
	/** Left alt key. */
	KEY_MOD_LALT = 0x4,
	/** Left meta key. */
	KEY_MOD_LMETA = 0x8,
	/** Right control key. */
	KEY_MOD_RCTRL = 0x10,
	/** Right shift key. */
	KEY_MOD_RSHIFT = 0x20,
	/** Right alt key. */
	KEY_MOD_RALT = 0x40,
	/** Right meta key. */
	KEY_MOD_RMETA = 0x80
};

/**
 * HID key scan codes.
 */
enum key_code {
	KEY_A = 0x4,
	KEY_B = 0x5,
	KEY_C = 0x6,
	KEY_D = 0x7,
	KEY_E = 0x8,
	KEY_F = 0x9,
	KEY_G = 0xa,
	KEY_H = 0xb,
	KEY_I = 0xc,
	KEY_J = 0xd,
	KEY_K = 0xe,
	KEY_L = 0xf,
	KEY_M = 0x10,
	KEY_N = 0x11,
	KEY_O = 0x12,
	KEY_P = 0x13,
	KEY_Q = 0x14,
	KEY_R = 0x15,
	KEY_S = 0x16,
	KEY_T = 0x17,
	KEY_U = 0x18,
	KEY_V = 0x19,
	KEY_W = 0x1a,
	KEY_X = 0x1b,
	KEY_Y = 0x1c,
	KEY_Z = 0x1d,
	KEY_1 = 0x1e,
	KEY_2 = 0x1f,
	KEY_3 = 0x20,
	KEY_4 = 0x21,
	KEY_5 = 0x22,
	KEY_6 = 0x23,
	KEY_7 = 0x24,
	KEY_8 = 0x25,
	KEY_9 = 0x26,
	KEY_0 = 0x27,
	KEY_RETURN = 0x28,
	KEY_ESCAPE = 0x29,
	KEY_BACKSPACE = 0x2a,
	KEY_TAB = 0x2b,
	KEY_SPACE = 0x2c,
	KEY_MINUS = 0x2d,
	KEY_EQUAL = 0x2e,
	KEY_LEFT_BRACKET = 0x2f,
	KEY_RIGHT_BRACKET = 0x30,
	KEY_BACKSLASH = 0x31,
	KEY_INT2 = 0x32, /* # and ~ */
	KEY_SEMICOLON = 0x33,
	KEY_QUOTE = 0x34,
	KEY_GRAVE = 0x35,
	KEY_COMMA = 0x36,
	KEY_PERIOD = 0x37,
	KEY_SLASH = 0x38,
	KEY_CAPS_LOCK = 0x39,
	KEY_F1 = 0x3a,
	KEY_F2 = 0x3b,
	KEY_F3 = 0x3c,
	KEY_F4 = 0x3d,
	KEY_F5 = 0x3e,
	KEY_F6 = 0x3f,
	KEY_F7 = 0x40,
	KEY_F8 = 0x41,
	KEY_F9 = 0x42,
	KEY_F10 = 0x43,
	KEY_F11 = 0x44,
	KEY_F12 = 0x45,
	KEY_PRINT = 0x46,
	KEY_SCROLL_LOCK = 0x47,
	KEY_PAUSE = 0x48,
	KEY_INSERT = 0x49,
	KEY_HOME = 0x4a,
	KEY_PAGE_UP = 0x4b,
	KEY_DELETE = 0x4c,
	KEY_END = 0x4d,
	KEY_PAGE_DOWN = 0x4e,
	KEY_RIGHT = 0x4f,
	KEY_LEFT = 0x50,
	KEY_DOWN = 0x51,
	KEY_UP = 0x52,
	KEY_NUM_LOCK = 0x53,
	KEY_KP_SLASH = 0x54,
	KEY_KP_ASTERISK = 0x55,
	KEY_KP_MINUS = 0x56,
	KEY_KP_PLUS = 0x57,
	KEY_KP_ENTER = 0x58,
	KEY_KP_1 = 0x59,
	KEY_KP_2 = 0x5a,
	KEY_KP_3 = 0x5b,
	KEY_KP_4 = 0x5c,
	KEY_KP_5 = 0x5d,
	KEY_KP_6 = 0x5e,
	KEY_KP_7 = 0x5f,
	KEY_KP_8 = 0x60,
	KEY_KP_9 = 0x61,
	KEY_KP_0 = 0x62,
	KEY_KP_PERIOD = 0x63,
	KEY_KP_INT1 = 0x64,
	KEY_APPLICATION = 0x65,
	KEY_POWER = 0x66,
	KEY_KP_EQUALS = 0x67,
	KEY_F13 = 0x68,
	KEY_F14 = 0x69,
	KEY_F15 = 0x6a,
	KEY_F16 = 0x6b,
	KEY_F17 = 0x6c,
	KEY_F18 = 0x6d,
	KEY_F19 = 0x6e,
	KEY_F20 = 0x6f,
	KEY_F21 = 0x70,
	KEY_F22 = 0x71,
	KEY_F23 = 0x72,
	KEY_F24 = 0x73,
	KEY_EXECUTE = 0x74,
	KEY_HELP = 0x75,
	KEY_UNDO = 0x7a,
	KEY_CUT = 0x7b,
	KEY_COPY = 0x7c,
	KEY_PASTE = 0x7d,
	KEY_MUTE = 0x7f,
	KEY_VOLUME_UP = 0x80,
	KEY_VOLUME_DOWN = 0x81,

	KEY_LCTRL = 0xe0,
	KEY_LSHIFT = 0xe1,
	KEY_LALT = 0xe2,
	KEY_LMETA = 0xe3,
	KEY_RCTRL = 0xe4,
	KEY_RSHIFT = 0xe5,
	KEY_RALT = 0xe6,
	KEY_RMETA = 0xe7,
};

#endif
/**
 * @}
 */

