/**
 * @defgroup usb USB
 * @addtogroup usb
 * USB keyboard implementation.
 * @{
 */
#ifndef USB_H_INCLUDED
#define USB_H_INCLUDED

#include <stdbool.h>

/**
 * Initializes the USB keyboard.
 *
 * This function must not be called when the device is in wireless mode.
 */
void usb_init(void);
/**
 * Returns whether USB is connected.
 *
 * @returns True if USB is connected.
 */
bool usb_is_connected(void);
/**
 * Prepares the USB code for "system off".
 */
void usb_prepare_system_off(void);

/**
 * Polls the USB stack.
 */
void usb_poll(void);
/* TODO: how often/when is usb_poll() supposed to be called? */

#endif
/**
 * @}
 */

