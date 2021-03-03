
#include "unifying.h"
#include "unifying_radio.h"
#include "keycodes.h"

/* TODO: still required? all radio work is done in unifying_radio.c */
#include "app_error.h"
#include "app_timer.h"
#include "nrf_crypto.h"
#include "nrf_delay.h"
#include "nrf_esb.h"
#include "nrf_gpio.h"
#include "nrf_log.h"

#include <string.h>

static const uint8_t PAIRING_ADDRESS[5] = {0x75, 0xa5, 0xdc, 0x0a, 0xbb};
static const uint8_t PSEUDO_DEVICE_ADDRESS[5] = {0x12, 0x34, 0x56, 0x78, 0x9a}; /* TODO */
static const uint8_t DEVICE_WPID[2] = {0x40, 0x03}; // K270
static const uint8_t DEVICE_SERIAL[4] = {0x12, 0x34, 0x56, 0x78}; /* TODO: generate a new serial per device */

static uint8_t device_address[5];
static uint8_t pairing_device_nonce[4];
static uint8_t pairing_dongle_nonce[4];
static uint8_t device_key[16];
static uint8_t dongle_wpid[2];

static uint32_t aes_counter = 0xdeadc0de; /* TODO: randomize */

APP_TIMER_DEF(update_timer);

enum unifying_state {
	UNPAIRED,
	PAIRING_START,
	PAIRING_REQUEST_1,
	PAIRING_RESPONSE_1,
	PAIRING_ACCEPT_ADDRESS,
	PAIRING_REQUEST_2,
	PAIRING_RESPONSE_2,
	PAIRING_REQUEST_3,
	PAIRING_RESPONSE_3,
	PAIRING_REQUEST_4,
	RECONNECTING_1,
	RECONNECTING_2,
	RECONNECTING_3,
	DISCONNECTED,
	CONNECTED,
	CONNECTED_SENDING,
};

enum device_protocol {
	PROTOCOL_UNIFYING = 0x4,
	PROTOCOL_G700 = 0x7,
	PROTOCOL_LIGHTSPEED = 0xc,
};

enum device_type {
	DEVICE_UNKNOWN = 0x0,
	DEVICE_KEYBOARD = 0x1,
	DEVICE_MOUSE = 0x2,
	DEVICE_NUMPAD = 0x3,
	DEVICE_PRESENTER = 0x4,
	DEVICE_REMOTE = 0x7,
	DEVICE_TRACKBALL = 0x8,
	DEVICE_TOUCHPAD = 0x9,
	DEVICE_TABLET = 0xa,
	DEVICE_GAMEPAD = 0xb,
	DEVICE_JOYSTICK = 0xc,
};

enum capabilities {
	CAP_LINK_ENCRYPTION = 1 << 0,
	CAP_BATTERY_STATUS = 1 << 1,
	CAP_UNIFYING_COMPATIBLE = 1 << 2,
	CAP_UNKNOWN = 1 << 3,
};

enum report_type {
	REPORT_KEYBOARD = 0x01,
	REPORT_MOUSE = 0x02,
	REPORT_MULTIMEDIA = 0x03,
	REPORT_SYSTEM_CTL = 0x04,
	REPORT_LED = 0x0e,
	REPORT_SET_KEEP_ALIVE = 0x0f,
	REPORT_HIDPP_SHORT = 0x10,
	REPORT_HIDPP_LONG = 0x11,
	REPORT_ENCRYPTED_KEYBOARD = 0x13,
	REPORT_ENCRYPTED_HIDPP_LONG = 0x1b,
	REPORT_PAIRING = 0x1f,
	REPORT_KEEP_ALIVE = 0x40,
};

enum pairing_marker {
	PAIRING_MARKER_PHASE_1 = 0xe1,
	PAIRING_MARKER_PHASE_2 = 0xe2,
	PAIRING_MARKER_PHASE_3 = 0xe3,
};

enum power_switch_location {
	POWER_SWITCH_RESERVED = 0x0,
	POWER_SWITCH_BASE = 0x1,
	POWER_SWITCH_TOP_CASE = 0x2,
	POWER_SWITCH_EDGE_OF_TOP_RIGHT_CORNER = 0x3,
	POWER_SWITCH_OTHER = 0x4,
	POWER_SWITCH_TOP_LEFT_CORNER = 0x5,
	POWER_SWITCH_BOTTOM_LEFT_CORNER = 0x6,
	POWER_SWITCH_TOP_RIGHT_CORNER = 0x7,
	POWER_SWITCH_BOTTOM_RIGHT_CORNER = 0x8,
	POWER_SWITCH_TOP_EDGE = 0x9,
	POWER_SWITCH_RIGHT_EDGE = 0xa,
	POWER_SWITCH_LEFT_EDGE = 0xb,
	POWER_SWITCH_BOTTOM_EDGE = 0xc,
};

static enum unifying_state state;

static uint8_t calculate_checksum(uint8_t *buffer, size_t length) {
	uint8_t sum = 0;
	size_t i;
	for (i = 0; i < length; i++) {
		sum -= buffer[i];
	}
	return sum;
}

static void unifying_pairing_send_accept_address(void);
static void unifying_pairing_send_request_2(void);
static void unifying_pairing_send_request_3(void);
static void unifying_pairing_send_request_4(void);

static void unifying_pairing_send_request_1(void) {
	uint8_t buffer[22];

	state = PAIRING_REQUEST_1;

	buffer[0] = PAIRING_MARKER_PHASE_1; /* TODO: randomize */
	buffer[1] = REPORT_PAIRING | REPORT_KEEP_ALIVE;
	buffer[2] = 1;
	memcpy(&buffer[3], PSEUDO_DEVICE_ADDRESS, 5);
	buffer[8] = 0x14; /* 20ms keep-alive */
	buffer[9] = DEVICE_WPID[0];
	buffer[10] = DEVICE_WPID[1];
	buffer[11] = PROTOCOL_UNIFYING;
	buffer[12] = 0x0; // Unknown, 0x2 for some devices.
	buffer[13] = DEVICE_KEYBOARD;
	buffer[14] = CAP_LINK_ENCRYPTION | CAP_BATTERY_STATUS | CAP_UNIFYING_COMPATIBLE | CAP_UNKNOWN;
	/* TODO: encryption */
	//buffer[14] = CAP_BATTERY_STATUS | CAP_UNIFYING_COMPATIBLE | CAP_UNKNOWN;
	buffer[15] = 0x0; /* unknown */
	buffer[16] = 0x0;
	buffer[17] = 0x0;
	buffer[18] = 0x0;
	buffer[19] = 0x0;
	buffer[20] = 0x1a; /* unknown */
	buffer[21] = calculate_checksum(buffer, 21);

	unifying_radio_send_packet(0, buffer, 22);
}

static void unifying_pairing_request_response_1(void) {
	uint8_t buffer[5];

	state = PAIRING_RESPONSE_1;

	buffer[0] = PAIRING_MARKER_PHASE_1;
	buffer[1] = REPORT_KEEP_ALIVE;
	buffer[2] = 1;
	buffer[3] = PSEUDO_DEVICE_ADDRESS[0];
	buffer[4] = calculate_checksum(buffer, 4);

	unifying_radio_send_packet(0, buffer, 5);
}

static void unifying_pairing_parse_response_1(nrf_esb_payload_t *packet) {
	uint8_t prefixes[2];
	int i;

	/* TODO: parse the checksum */

	if (packet->length != 22) {
		return;
	}
	if (packet->data[0] != PAIRING_MARKER_PHASE_1) {
		return;
	}
	if (packet->data[1] != REPORT_PAIRING) {
		return;
	}
	if (packet->data[2] != 1) {
		return;
	}
	for (i = 0; i < 5; i++) {
		device_address[i] = packet->data[3 + 4 - i];
	}
	dongle_wpid[0] = packet->data[9];
	dongle_wpid[1] = packet->data[10];

	/* pipe 1 is the assigned device address */
	/* TODO: move code to unifying_radio.c */
	nrf_esb_enable_pipes(0x00);
	nrf_esb_set_base_address_0(&PAIRING_ADDRESS[1]);
	prefixes[0] = PAIRING_ADDRESS[0];
	nrf_esb_set_base_address_1(&device_address[1]);
	prefixes[1] = device_address[0];
	nrf_esb_set_prefixes(prefixes, 2);
	nrf_esb_enable_pipes(0x03);
	/* TODO */

	nrf_delay_ms(1);
	unifying_pairing_send_accept_address();
}

static void unifying_pairing_send_accept_address(void) {
	/* TODO: this is probably not needed and the logitacker code only contains it
	 * to generate an artificial delay? */
	uint8_t buffer[5];

	state = PAIRING_ACCEPT_ADDRESS;

	buffer[0] = PAIRING_MARKER_PHASE_1;
	buffer[1] = REPORT_KEEP_ALIVE;
	buffer[2] = 1;
	buffer[3] = PSEUDO_DEVICE_ADDRESS[0];
	buffer[4] = calculate_checksum(buffer, 4);

	unifying_radio_send_packet(0, buffer, 5);
}

static void unifying_pairing_send_request_2(void) {
	uint8_t buffer[22] = {0};
	uint32_t report_types;

	state = PAIRING_REQUEST_2;

	buffer[0] = PAIRING_MARKER_PHASE_2;
	buffer[1] = REPORT_PAIRING | REPORT_KEEP_ALIVE;
	buffer[2] = 2;
	/* TODO: random nonce */
	pairing_device_nonce[0] = 0x12;
	pairing_device_nonce[1] = 0x34;
	pairing_device_nonce[2] = 0x56;
	pairing_device_nonce[3] = 0x78;
	memcpy(&buffer[3], pairing_device_nonce, 4);
	memcpy(&buffer[7], DEVICE_SERIAL, 4);
	report_types = 1 << REPORT_KEYBOARD;
	memcpy(&buffer[11], &report_types, 4); /* little endian */
	buffer[15] = POWER_SWITCH_EDGE_OF_TOP_RIGHT_CORNER;
	buffer[21] = calculate_checksum(buffer, 21);

	unifying_radio_send_packet(1, buffer, 22);
}

static void unifying_pairing_request_response_2(void) {
	uint8_t buffer[5];

	state = PAIRING_RESPONSE_2;

	buffer[0] = PAIRING_MARKER_PHASE_2;
	buffer[1] = REPORT_KEEP_ALIVE;
	buffer[2] = 2;
	//buffer[3] = device_address[0];
	buffer[3] = 0x12; /* TODO: ? */
	buffer[4] = calculate_checksum(buffer, 4);

	unifying_radio_send_packet(1, buffer, 5);
}

static void unifying_pairing_derive_key(void) {
	uint8_t material[16];
	/*memcpy(&material[0], device_address, 4);*/
	material[0] = device_address[4];
	material[1] = device_address[3];
	material[2] = device_address[2];
	material[3] = device_address[1];
	memcpy(&material[4], DEVICE_WPID, 2);
	memcpy(&material[6], dongle_wpid, 2);
	memcpy(&material[8], pairing_device_nonce, 4);
	memcpy(&material[12], pairing_dongle_nonce, 4);

	device_key[2] = material[0];
	device_key[1] = material[1] ^ 0xff;
	device_key[5] = material[2] ^ 0xff;
	device_key[3] = material[3];
	device_key[14] = material[4];
	device_key[11] = material[5];
	device_key[9] = material[6];
	device_key[0] = material[7];
	device_key[8] = material[8];
	device_key[6] = material[9] ^ 0x55;
	device_key[4] = material[10];
	device_key[15] = material[11];
	device_key[10] = material[12] ^ 0xff;
	device_key[12] = material[13];
	device_key[7] = material[14];
	device_key[13] = material[15] ^ 0x55;
}

static void unifying_pairing_parse_response_2(nrf_esb_payload_t *packet) {
	/* TODO: parse the checksum */

	if (packet->length != 22) {
		return;
	}
	if (packet->data[0] != PAIRING_MARKER_PHASE_2) {
		return;
	}
	if (packet->data[1] != REPORT_PAIRING) {
		return;
	}
	if (packet->data[2] != 2) {
		return;
	}
	if (packet->pipe != 1) {
		return;
	}

	/* TODO: check device serial number */

	memcpy(pairing_dongle_nonce, &packet->data[3], 4);
	unifying_pairing_derive_key();

	nrf_delay_ms(1);
	unifying_pairing_send_request_3();
}

static void unifying_pairing_send_request_3(void) {
	uint8_t buffer[22] = {0};

	state = PAIRING_REQUEST_3;

	buffer[0] = PAIRING_MARKER_PHASE_3;
	buffer[1] = REPORT_PAIRING | REPORT_KEEP_ALIVE;
	buffer[2] = 3;
	buffer[3] = 1; /* number of packets for the device name? */
	buffer[4] = strlen("goboard");
	strcpy((char*)&buffer[5], "goboard");
	buffer[21] = calculate_checksum(buffer, 21);

	unifying_radio_send_packet(1, buffer, 22);
}

static void unifying_pairing_request_response_3(void) {
	uint8_t buffer[5];

	state = PAIRING_RESPONSE_3;

	buffer[0] = PAIRING_MARKER_PHASE_3;
	buffer[1] = REPORT_KEEP_ALIVE;
	buffer[2] = 3;
	buffer[3] = 1;
	buffer[4] = calculate_checksum(buffer, 4);

	unifying_radio_send_packet(1, buffer, 5);
}

static void unifying_pairing_parse_response_3(nrf_esb_payload_t *packet) {
	/* TODO: parse the checksum */

	if (packet->length != 10) {
		return;
	}
	if (packet->data[0] != PAIRING_MARKER_PHASE_3) {
		return;
	}
	if (packet->data[1] != 0xf) {
		return;
	}
	if (packet->data[2] != 6) {
		return;
	}
	if (packet->pipe != 1) {
		return;
	}

	/* TODO: check rest of the packet */

	nrf_delay_ms(1);
	unifying_pairing_send_request_4();
}

static void unifying_pairing_send_request_4(void) {
	uint8_t buffer[10] = {0};

	state = PAIRING_REQUEST_4;

	buffer[0] = PAIRING_MARKER_PHASE_2;
	buffer[1] = 0xf | REPORT_KEEP_ALIVE;
	buffer[2] = 6;
	buffer[3] = 1;
	buffer[9] = calculate_checksum(buffer, 9);

	unifying_radio_send_packet(1, buffer, 10);
}

static void unifying_start_pairing(void) {
	unifying_radio_set_pairing_channels();

	/* TODO: move code to unifying_radio.c */
	nrf_esb_enable_pipes(0x00);
	nrf_esb_set_base_address_0(&PAIRING_ADDRESS[1]);
	nrf_esb_set_prefixes(&PAIRING_ADDRESS[0], 1);
	nrf_esb_enable_pipes(0x01);

	state = PAIRING_REQUEST_1;
	unifying_pairing_send_request_1();
}

static void unifying_pairing_error(void) {
	NRF_LOG_INFO("pairing error");
	/* TODO: other error handling? */
	state = UNPAIRED;
}

static void unifying_pairing_rx_handler(nrf_esb_payload_t *packet) {
	if (state == PAIRING_RESPONSE_1) {
		unifying_pairing_parse_response_1(packet);
	} else if (state == PAIRING_RESPONSE_2) {
		unifying_pairing_parse_response_2(packet);
	} else if (state == PAIRING_RESPONSE_3) {
		unifying_pairing_parse_response_3(packet);
	}
}

static void unifying_pairing_tx_handler(bool success) {
	if (!success) {
		unifying_pairing_error();
		return;
	}

	if (state == PAIRING_REQUEST_1) {
		nrf_delay_ms(10);
		unifying_pairing_request_response_1();
	} else if (state == PAIRING_RESPONSE_1) {
		/* nothing do to here, we are waiting for the ACK payload */
	} else if (state == PAIRING_ACCEPT_ADDRESS) {
		nrf_delay_ms(1);
		unifying_pairing_send_request_2();
	} else if (state == PAIRING_REQUEST_2) {
		nrf_delay_ms(10);
		unifying_pairing_request_response_2();
	} else if (state == PAIRING_RESPONSE_2) {
		/* nothing do to here, we are waiting for the ACK payload */
	} else if (state == PAIRING_REQUEST_3) {
		nrf_delay_ms(10);
		unifying_pairing_request_response_3();
	} else if (state == PAIRING_RESPONSE_3) {
		/* nothing do to here, we are waiting for the ACK payload */
	} else if (state == PAIRING_REQUEST_4) {
		/* pairing is finished */
		state = CONNECTED;
		return;
	}
}

static void unifying_calculate_frame_key(uint8_t *frame_key, uint32_t counter_be) {
	ret_code_t ret;
	static nrf_crypto_aes_context_t aes_ctx;
	size_t length;

	/* generate the counter value */
	uint8_t counter_block[16] = {
		0x04, 0x14, 0x1d, 0x1f, 0x27, 0x28, 0x0d, 0x00,
		0x00, 0x00, 0x00, 0x0a, 0x0d, 0x13, 0x26, 0x0e
	};
	memcpy(&counter_block[7], &counter_be, 4);

	ret = nrf_crypto_aes_crypt(&aes_ctx,
	                           &g_nrf_crypto_aes_ecb_128_info,
	                           NRF_CRYPTO_ENCRYPT,
	                           device_key,
	                           NULL,
	                           counter_block,
	                           16,
	                           frame_key,
	                           &length);
	APP_ERROR_CHECK(ret);
}

/* TODO: remove */
#define TEST_KEY NRF_GPIO_PIN_MAP(1, 2)
static int test_key_state = 0;
static int prev_test_key_state = 0;
static int test_key_code = KEY_A;

static void unifying_send_encrypted_keyboard_report(void) {
	uint8_t plain_report[8] = {0};
	uint8_t encrypted_report[22] = {0};
	uint32_t counter_be = __builtin_bswap32(aes_counter);
	uint8_t frame_key[16];
	int i;

	if (!test_key_state && prev_test_key_state) {
		/* select the next key */
		test_key_code = test_key_code + 1;
		if (test_key_code == KEY_RETURN) {
			test_key_code = KEY_A;
		}
	}
	prev_test_key_state = test_key_state;

	plain_report[1] = test_key_state ? test_key_code : 0;
	plain_report[7] = 0xc9;

	encrypted_report[0] = 0;
	encrypted_report[1] = REPORT_ENCRYPTED_KEYBOARD | REPORT_KEEP_ALIVE | 0x80;
	memcpy(&encrypted_report[10], &counter_be, 4);

	unifying_calculate_frame_key(frame_key, counter_be);
	aes_counter += 1; /* TODO: only increase when the packet has been received */

	for (i = 0; i < 8; i++) {
		encrypted_report[2 + i] = plain_report[i] ^ frame_key[i];
	}

	encrypted_report[21] = calculate_checksum(encrypted_report, 21);

	unifying_radio_send_packet(1, encrypted_report, 22);
}

static void unifying_send_keep_alive(void) {
	int new_state = !nrf_gpio_pin_read(TEST_KEY);
	if (test_key_state != new_state) {
		test_key_state = new_state;

		/*uint8_t buffer[10] = {0};

		buffer[0] = 0;
		buffer[1] = REPORT_KEYBOARD | REPORT_KEEP_ALIVE;
		buffer[2] = 0;
		buffer[3] = test_key_state ? 0x1b : 0;
		buffer[9] = calculate_checksum(buffer, 9);

		unifying_radio_send_packet(1, buffer, 10);*/
		unifying_send_encrypted_keyboard_report();
	} else {
		uint8_t buffer[5] = {0};

		buffer[0] = 0;
		buffer[1] = REPORT_KEEP_ALIVE;
		buffer[2] = 0;
		buffer[3] = 0x14;
		buffer[4] = calculate_checksum(buffer, 4);

		unifying_radio_send_packet(1, buffer, 5);
	}
}

static void unifying_rx_handler(nrf_esb_payload_t *packet) {
	if (state >= PAIRING_START && state <= PAIRING_REQUEST_4) {
		unifying_pairing_rx_handler(packet);
	}
	/* TODO */
}

static void unifying_tx_handler(bool success) {
	if (state >= PAIRING_START && state <= PAIRING_REQUEST_4) {
		unifying_pairing_tx_handler(success);
	}
	/* TODO */
}

static void update_timer_handler(void *ctx) {
	if (state == CONNECTED) {
		unifying_send_keep_alive();
		/* TODO */
	}
}

void unifying_init(void) {
	ret_code_t ret;

	nrf_gpio_cfg_input(TEST_KEY, NRF_GPIO_PIN_PULLUP);

	ret = app_timer_create(&update_timer,
	                       APP_TIMER_MODE_REPEATED,
	                       update_timer_handler);
	APP_ERROR_CHECK(ret);
	ret = app_timer_start(update_timer, APP_TIMER_TICKS(10), NULL);
	APP_ERROR_CHECK(ret);

	unifying_radio_init(unifying_rx_handler, unifying_tx_handler);

	/* TODO: load existing pairing information */

	/* TODO: we just start pairing here, without any information */
	unifying_start_pairing();
}

