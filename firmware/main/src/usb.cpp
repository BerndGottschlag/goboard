#include "usb.hpp"

#include "exception.hpp"

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

// TODO: This should be a member function.
static enum usb_dc_status_code usb_status;
static void status_cb(enum usb_dc_status_code status, const uint8_t *param) {
	// TODO: The status should maybe influence charging.
	usb_status = status;
}

// TODO: Proper NKRO descriptor.
static const uint8_t hid_report_descriptor[] = HID_KEYBOARD_REPORT_DESC();

// TODO: Boot protocol support.

UsbKeyboard::UsbKeyboard() {
	// Initialize USB.
	const struct device *hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		throw InitializationFailed("USB HID device not found");
	}
	usb_hid_register_device(hid_dev,
	                        hid_report_descriptor,
	                        sizeof(hid_report_descriptor),
	                        NULL);
	usb_hid_init(hid_dev);
	int ret = usb_enable(status_cb);
	if (ret != 0) {
		// TODO: Unregister device?
		throw InitializationFailed("failed to enable USB");
	}

	// Once USB has been initialized, we can start a thread that polls the
	// key matrix.
	// TODO
}

UsbKeyboard::~UsbKeyboard() {
	// Stop the thread.
	// TODO

	// Disable USB again.
	usb_disable();
	// TODO: Document that this class cannot be called again as USB HID is
	// still initialized.
}

KeyboardProfile UsbKeyboard::get_profile() {
	return PROFILE_1;
}

void UsbKeyboard::set_profile(KeyboardProfile profile) {
	// There is only one USB connection, nothing to do here.
	(void)profile;
}
