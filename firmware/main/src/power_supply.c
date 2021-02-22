
#include "power_supply.h"

#include "app_timer.h"
#include "nrf_drv_saadc.h"
#include "nrf_gpio.h"

/* TODO: use a suitable pin on the breadboard prototype */
#define MEASURE_VBAT NRF_GPIO_PIN_MAP(0, 30)
#define VBAT_INPUT NRF_SAADC_INPUT_AIN7
#define VMID_INPUT NRF_SAADC_INPUT_AIN0


static void adc_callback(const nrf_drv_saadc_evt_t *event) {
	/* TODO */
}

static void power_supply_timer_handler(void *ctx) {
	/* TODO */
}

void power_supply_init(power_supply_state_change_listener_t listener) {
#if 0
	ret_code_t ret;

	/* the resistor divider to measure battery voltage is power-gated to
	 * conserve energy */
	nrf_gpio_cfg_output(MEASURE_VBAT);
	nrf_gpio_pin_set(MEASURE_VBAT);

	/* TODO: gain setting */
	nrf_saadc_channel_config_t vbat_config =
			NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(VBAT_INPUT);
	nrf_saadc_channel_config_t vmid_config =
			NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(VMID_INPUT);

	ret = nrf_drv_saadc_init(NULL, adc_callback);
	APP_ERROR_CHECK(ret);

	ret = nrf_drv_saadc_channel_init(0, &vbat_config);
	APP_ERROR_CHECK(ret);
	ret = nrf_drv_saadc_channel_init(1, &vmid_config);
	APP_ERROR_CHECK(ret);

	/* we sample the battery voltage every 5 seconds */
	ret = app_timer_create(&power_supply_timer,
	                       APP_TIMER_MODE_SINGLE_SHOT,
	                       power_supply_timer_handler);
	APP_ERROR_CHECK(ret);

	poll_battery();
#endif
}

enum power_supply_mode power_supply_get_mode(void) {
	/* TODO */
	return POWER_SUPPLY_NORMAL;
}

uint8_t battery_charge(void) {
	/* TODO */
	return 100;
}

void power_supply_prepare_system_off(void) {
	/* TODO */
}

