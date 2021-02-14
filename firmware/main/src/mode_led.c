
#include "mode_led.h"

#include "app_timer.h"
#include "nrf_gpio.h"

/* TODO: use the correct pins */
#define MODE_LED NRF_GPIO_PIN_MAP(1, 15)

APP_TIMER_DEF(blink_timer);

#define SLOW_PERIOD (APP_TIMER_TICKS(1000))
#define FAST_PERIOD (APP_TIMER_TICKS(250))
#define FLASH_PERIOD (APP_TIMER_TICKS(100))

static bool is_on = false;
static unsigned int blink_period = 0;

static void blink_timer_handler(void *ctx) {
	UNUSED_VARIABLE(ctx);

	if (is_on) {
		is_on = false;
		nrf_gpio_pin_clear(MODE_LED);
		app_timer_start(blink_timer, blink_period, NULL);
	} else {
		is_on = true;
		nrf_gpio_pin_set(MODE_LED);
		app_timer_start(blink_timer, FLASH_PERIOD, NULL);
	}
}

void mode_led_init() {
	ret_code_t ret;

	nrf_gpio_cfg_output(MODE_LED);

	ret = app_timer_create(&blink_timer,
	                       APP_TIMER_MODE_SINGLE_SHOT,
	                       blink_timer_handler);
	APP_ERROR_CHECK(ret);
}

void mode_led_set(enum mode_led_status status) {
	switch (status) {
		case MODE_LED_CONNECTED:
			app_timer_stop(blink_timer);
			nrf_gpio_pin_set(MODE_LED);
			break;
		case MODE_LED_CHARGING:
			app_timer_stop(blink_timer);
			nrf_gpio_pin_set(MODE_LED);
			break;
		case MODE_LED_NOT_CONNECTED:
			app_timer_stop(blink_timer);
			nrf_gpio_pin_set(MODE_LED);
			is_on = true;
			blink_period = SLOW_PERIOD;
			app_timer_start(blink_timer, FLASH_PERIOD, NULL);
			break;
		case MODE_LED_PAIRING:
			app_timer_stop(blink_timer);
			nrf_gpio_pin_set(MODE_LED);
			is_on = true;
			blink_period = SLOW_PERIOD;
			app_timer_start(blink_timer, FLASH_PERIOD, NULL);
			break;
	}
}

void mode_led_prepare_system_off(void) {
	nrf_gpio_pin_clear(MODE_LED);
}

