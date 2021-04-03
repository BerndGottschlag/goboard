#ifndef LEDS_HPP_INCLUDED
#define LEDS_HPP_INCLUDED

#include <drivers/gpio.h>

enum ModeLed {
	MODE_LED_OFF,
	MODE_LED_CHARGING,
	MODE_LED_DISCONNECTED,
	MODE_LED_PAIRING,
	MODE_LED_RECONNECTING,
	MODE_LED_CONNECTED,
};

class Leds {
public:
	Leds();
	~Leds();

	void set_mode(ModeLed mode);
	void set_caps_lock(bool caps_lock);
	void set_scroll_lock(bool scroll_lock);
private:
};

#endif


