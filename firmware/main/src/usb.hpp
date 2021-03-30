#ifndef USB_HPP_INCLUDED
#define USB_HPP_INCLUDED

#include "mode_switch.hpp"
#include "key_matrix.hpp"
#include "keys.hpp"

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>


/// USB HID keyboard implementation.
class UsbKeyboard {
public:
	UsbKeyboard(Keys<KeyMatrix> *keys);
	~UsbKeyboard();

	KeyboardProfile get_profile();
	void set_profile(KeyboardProfile profile);
private:
	static void status_cb(usb_dc_status_code status, const uint8_t *param);

	static void static_on_sof(struct k_work *work);
	void on_sof();

	static void static_on_poll_suspended(struct k_work *work);
	void on_poll_suspended();

	static void on_protocol_change(const struct device *dev, uint8_t protocol);

	struct k_work sof;
	struct k_work_delayable poll_suspended;

	Keys<KeyMatrix> *keys;

	const struct device *hid_dev;

	bool connected = false;
	atomic_t suspended = ATOMIC_INIT(0);

	bool boot_protocol = false;
	KeyBitmap prev_key_bitmap;

	// There can only be one instance of the USB keyboard, and the USB callbacks
	// need a pointer to it.
	static UsbKeyboard *instance;

	static const struct hid_ops ops;
};

#endif

