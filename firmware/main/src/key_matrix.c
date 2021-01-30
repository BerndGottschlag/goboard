
#include "key_matrix.h"

#include "nrf_drv_timer.h"
#include "nrf_gpio.h"
#include "nrf_ringbuf.h"
#include "app_usbd_hid_kbd.h"

#define KEY_POLL_INTERVAL_MS 1

#define TEST_KEY NRF_GPIO_PIN_MAP(1, 2)
#define DEBUG_LED NRF_GPIO_PIN_MAP(1, 15)
static int test_key_state = 0;

const nrf_drv_timer_t KEY_TIMER = NRF_DRV_TIMER_INSTANCE(0);
NRF_RINGBUF_DEF(key_events, 512);

static void key_matrix_poll(void) {
	int new_state = nrf_gpio_pin_read(TEST_KEY);
	if (new_state) {
		nrf_gpio_pin_set(DEBUG_LED);
		//nrf_gpio_pin_clear(DEBUG_LED);
	} else {
		nrf_gpio_pin_clear(DEBUG_LED);
	}

	CRITICAL_REGION_ENTER();
	if (new_state != test_key_state) {
		/* write the data to the ring buffer */
		const struct key_event event = {
			.modifiers = 0,
			.key = APP_USBD_HID_KBD_X,
			.pressed = new_state,
		};
		size_t length = sizeof(event);
		nrf_ringbuf_cpy_put(&key_events, (const unsigned char*)&event, &length);
		if (length == sizeof(event)) {
			/* only overwrite the global state variable if writing
			 * to the buffer was successful */
			test_key_state = new_state;
		} else if (length != 0) {
			/* this should never happen, all sizes are powers of
			 * two! */
			APP_ERROR_CHECK_BOOL(0);
		}
	}
	CRITICAL_REGION_EXIT();
}

static void key_timer_handler(nrf_timer_event_t event_type, void *context) {
	(void)context;
	switch (event_type) {
		case NRF_TIMER_EVENT_COMPARE0:
			key_matrix_poll();
			break;
		default:
			break;
	}
}

void key_matrix_init(void) {
	ret_code_t ret;
	nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
	uint32_t time_ticks;

	nrf_ringbuf_init(&key_events);

	/* initialize SPI */
	/* TODO */
	nrf_gpio_cfg_input(TEST_KEY, NRF_GPIO_PIN_PULLUP);
	nrf_gpio_cfg_output(DEBUG_LED);
	nrf_gpio_pin_set(DEBUG_LED);

	/* we want to poll the key matrix periodically using a timer */
	/*ret = nrf_drv_timer_init(&KEY_TIMER, &timer_cfg, key_timer_handler);
	APP_ERROR_CHECK(ret);

	time_ticks = nrf_drv_timer_ms_to_ticks(&KEY_TIMER, KEY_POLL_INTERVAL_MS);
	nrf_drv_timer_extended_compare(&KEY_TIMER,
	                               NRF_TIMER_CC_CHANNEL0,
	                               time_ticks,
	                               NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
	                               true);

	nrf_drv_timer_enable(&KEY_TIMER);*/
}

size_t key_matrix_read(struct key_event *buffer, size_t length) {
	length = length * sizeof(struct key_event);
	nrf_ringbuf_cpy_get(&key_events, (unsigned char*)buffer, &length);
	return length / sizeof(struct key_event);
}
void key_matrix_reset(void) {
	unsigned char buffer[8];
	size_t length;

	CRITICAL_REGION_ENTER();

	/* reset the assumed keyboard state so that we get new events for keys
	 * that are already pressed */
	/* TODO */
	test_key_state = 0;

	/* empty the ring buffer to discard outdated events */
	for (;;) {
		length = sizeof(buffer);
		nrf_ringbuf_cpy_get(&key_events, buffer, &length);
		if (length == 0) {
			break;
		}
	}

	CRITICAL_REGION_EXIT();
}

