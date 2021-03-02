
#include "mode_switch.h"

#include "config.h"

#include "app_timer.h"
#include "nrf_drv_gpiote.h"

#define DEBOUNCE_MS 250

/* TODO: use the correct pins */
#define BLUETOOTH1_PIN NRF_GPIO_PIN_MAP(0, 27)
#define BLUETOOTH2_PIN NRF_GPIO_PIN_MAP(0, 6)

static mode_switch_change_listener_t change_listener;

static bool bluetooth1_selected;
static bool bluetooth2_selected;
static enum mode_switch_mode current_mode;

APP_TIMER_DEF(debounce_timer);

static enum mode_switch_mode mode_from_pins(bool bt1, bool bt2) {
	if (bt1) {
		return CONFIG_FIRST_PROFILE;
	} else if (bt2) {
		return CONFIG_FIRST_PROFILE;
	} else {
		return MODE_SWITCH_OFF_USB;
	}
}

static void debounce_timer_handler(void *ctx) {
	enum mode_switch_mode new_mode;

	UNUSED_VARIABLE(ctx);

	CRITICAL_REGION_ENTER();
	new_mode = mode_from_pins(bluetooth1_selected, bluetooth2_selected);
	CRITICAL_REGION_EXIT();
	if (new_mode != current_mode) {
		current_mode = new_mode;
		change_listener();
	}
}

static void pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
	UNUSED_VARIABLE(pin);
	UNUSED_VARIABLE(action);

	CRITICAL_REGION_ENTER();
	bluetooth1_selected = !nrf_drv_gpiote_in_is_set(BLUETOOTH1_PIN);
	bluetooth2_selected = !nrf_drv_gpiote_in_is_set(BLUETOOTH2_PIN);
	CRITICAL_REGION_EXIT();

	/* restart the debouncing timer */
	app_timer_stop(debounce_timer);
	app_timer_start(debounce_timer, APP_TIMER_TICKS(DEBOUNCE_MS), NULL);
}

void mode_switch_init(mode_switch_change_listener_t listener) {
	ret_code_t ret;
	nrf_drv_gpiote_in_config_t config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
	config.pull = NRF_GPIO_PIN_NOPULL;
	ret = nrf_drv_gpiote_in_init(BLUETOOTH1_PIN, &config, pin_handler);
	APP_ERROR_CHECK(ret);
	ret = nrf_drv_gpiote_in_init(BLUETOOTH2_PIN, &config, pin_handler);
	APP_ERROR_CHECK(ret);

	change_listener = listener;

	/* sample_the mode once */
	bluetooth1_selected = !nrf_drv_gpiote_in_is_set(BLUETOOTH1_PIN);
	bluetooth2_selected = !nrf_drv_gpiote_in_is_set(BLUETOOTH2_PIN);
	current_mode = mode_from_pins(bluetooth1_selected, bluetooth2_selected);

	/* to debounce the switch, we just wait for 250ms after any pin change -
	 * if there is another pin change during this time, we restart the timer
	 */
	ret = app_timer_create(&debounce_timer, APP_TIMER_MODE_SINGLE_SHOT, debounce_timer_handler);
	APP_ERROR_CHECK(ret);

	nrf_drv_gpiote_in_event_enable(BLUETOOTH1_PIN, true);
	nrf_drv_gpiote_in_event_enable(BLUETOOTH2_PIN, true);
}

enum mode_switch_mode mode_switch_get(void) {
	return current_mode;
}

void mode_switch_prepare_system_off(void) {
	/* nothing to do here, we just wake up on every  */
}

