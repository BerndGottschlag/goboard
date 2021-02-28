#include "keys.h"

#include "app_timer.h"
#include "nrf_gpio.h"
#include "nrf_drv_spi.h"

#include <string.h>

#ifdef TEST_SETUP

#define TEST_KEY NRF_GPIO_PIN_MAP(1, 2)

#else

#define SPI_SCK_PIN NRF_GPIO_PIN_MAP(1, 0)
#define SPI_MISO_PIN NRF_GPIO_PIN_MAP(0, 4)
#define SPI_MOSI_PIN NRF_GPIO_PIN_MAP(0, 5)
#define LOAD_PIN NRF_GPIO_PIN_MAP(0, 7)
#define STORE_PIN NRF_GPIO_PIN_MAP(0, 12)

#define NUMPAD_RX_PIN NRF_GPIO_PIN_MAP(0, 21)
#define NUMPAD_TX_PIN NRF_GPIO_PIN_MAP(0, 19)
#define NUMPAD_ID_PIN NRF_GPIO_PIN_MAP(1, 4)

#define ENABLE_KEY_POWER_PIN NRF_GPIO_PIN_MAP(0, 22)
#define ENABLE_NUMPAD_POWER_PIN NRF_GPIO_PIN_MAP(1, 4)

/**
 * SPI for the key matrix shift registers.
 */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(0);

#endif

/**
 * Timer for periodic sampling of the numpad ID line.
 */
APP_TIMER_DEF(numpad_id_timer);

static struct key_bitmap key_state = EMPTY_KEY_BITMAP;

void key_bitmap_set(struct key_bitmap *bitmap, int scancode) {
	uint32_t word = scancode >> 5;
	uint32_t bit = scancode & 0x1f;
	bitmap->keys[word] |= 1 << bit;
}

void key_bitmap_clear(struct key_bitmap *bitmap, int scancode) {
	uint32_t word = scancode >> 5;
	uint32_t bit = scancode & 0x1f;
	bitmap->keys[word] &= ~(1 << bit);
}

void legacy_key_list_from_bitmap(const struct key_bitmap *bitmap,
                                 struct legacy_key_list *list) {
	/* TODO */
}

static void numpad_id_timer_handler(void *context) {
	/* TODO */
}

void keys_init(void) {
	ret_code_t ret;

	ret = app_timer_create(&numpad_id_timer,
	                       APP_TIMER_MODE_REPEATED,
	                       numpad_id_timer_handler);
	APP_ERROR_CHECK(ret);

	/* TODO */
}

void keys_activate(void) {
	/* TODO */
}

void keys_deactivate(void) {
	/* TODO */
}

void keys_activate_numpad(void) {
	/* TODO */
}

void keys_deactivate_numpad(void) {
	/* TODO */
}

void keys_set_num_lock_led(bool state) {
	/* TODO */
}

void keys_get_state(struct key_bitmap *bitmap) {
	memcpy(bitmap, &key_state, sizeof(key_state));
}

bool keys_poll(bool pause) {
	struct key_bitmap previous_key_state = key_state;

#ifdef TEST_SETUP
	bool test_key_state = nrf_gpio_pin_read(TEST_KEY);
	if (test_key_state) {
		key_bitmap_set(&key_state, KEY_K);
	} else {
		key_bitmap_clear(&key_state, KEY_K);
	}
	/* TODO: debouncing, pause */
#else
	/* TODO */
#endif
	return memcmp(&key_state, &previous_key_state, sizeof(key_state)) != 0;
}

void keys_prepare_system_off(void) {
	/* TODO */
}

