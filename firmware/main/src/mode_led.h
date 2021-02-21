/**
 * @defgroup mode_led Mode LED
 * @addtogroup mode_led
 * LED to display the current state of the device.
 * @{
 */
#ifndef MODE_LED_H_INCLUDED
#define MODE_LED_H_INCLUDED

/**
 * State of the mode LED.
 */
enum mode_led_status {
	/**
	 * The mode LED signals that the device is connected to a computer.
	 */
	MODE_LED_CONNECTED,
	/**
	 * The mode LED signals that the device is charging its batteries.
	 */
	MODE_LED_CHARGING,
	/**
	 * The mode LED signals that the device is switched on, but is not
	 * connected to a computer.
	 *
	 * This mode is only used in wireless modes. In USB/off mode, the device
	 * is simply switched off completely.
	 */
	MODE_LED_NOT_CONNECTED,
	/**
	 * The mode LED signals that the device is being paired to a computer.
	 */
	MODE_LED_PAIRING,
};

/**
 * Initialized the mode LED.
 */
void mode_led_init();
/**
 * Changes the indication of the mode LED.
 *
 * @param status New mode signalled by the LED.
 */
void mode_led_set(enum mode_led_status status);

/**
 * Prepares the mode LED for "system off" mode.
 *
 * Timers used for the LED must not wake the system up.
 */
void mode_led_prepare_system_off(void);

#endif
/**
 * @}
 */

