#ifndef MODE_LED_H_INCLUDED
#define MODE_LED_H_INCLUDED

enum mode_led_status {
	MODE_LED_CONNECTED,
	MODE_LED_CHARGING,
	MODE_LED_NOT_CONNECTED,
	MODE_LED_PAIRING,
};

void mode_led_init();
void mode_led_set(enum mode_led_status status);

#endif

