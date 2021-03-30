#include "usb.hpp"

#include "exception.hpp"
#include "scan_code.hpp"

#define POLL_SUSPENDED_INTERVAL_MS 10
#define POLL_SUSPENDED_INTERVAL K_MSEC(POLL_SUSPENDED_INTERVAL_MS)

// TODO: This code is really overcomplicated, the USB stack can just be
// configured to use the system workqueue.

// TODO: Proper NKRO descriptor.
static const uint8_t hid_report_descriptor[] = HID_KEYBOARD_REPORT_DESC();

// TODO: Keyboard LED support.

UsbKeyboard::UsbKeyboard(Keys<KeyMatrix> *keys): keys(keys) {
	k_sched_lock();
	if (instance == NULL) {
		instance = this;
	}
	k_sched_unlock();

	k_work_init_delayable(&poll_suspended, static_on_poll_suspended);
	k_work_init(&sof, static_on_sof);

	// Initialize USB.
	hid_dev = device_get_binding("HID_0");
	if (hid_dev == NULL) {
		throw InitializationFailed("USB HID device not found");
	}
	usb_hid_register_device(hid_dev,
	                        hid_report_descriptor,
	                        sizeof(hid_report_descriptor),
	                        &ops);
	usb_hid_init(hid_dev);
	int ret = usb_enable(status_cb);
	if (ret != 0) {
		// TODO: Unregister device?
		throw InitializationFailed("failed to enable USB");
	}
}

UsbKeyboard::~UsbKeyboard() {
	// Stop the workqueue entries and make sure that they are not restarted.
	// TODO

	// Disable USB again.
	usb_disable();
	// TODO: Document that this class cannot be called again as USB HID is
	// still initialized.
	instance = NULL;
}

KeyboardProfile UsbKeyboard::get_profile() {
	return PROFILE_1;
}

void UsbKeyboard::set_profile(KeyboardProfile profile) {
	// There is only one USB connection, nothing to do here.
	(void)profile;
}

void UsbKeyboard::status_cb(enum usb_dc_status_code status,
                            const uint8_t *param) {
	ARG_UNUSED(param);

	switch (status) {
	case USB_DC_CONFIGURED:
		instance->connected = true;
		break;
	case USB_DC_DISCONNECTED:
		instance->connected = false;
		break;
	case USB_DC_SUSPEND:
		// While the device is suspended, we want to periodically poll
		// the key matrix to check whether we should wake the system up.
		// TODO: We should stop charging the batteries here?
		atomic_set(&instance->suspended, 1);
		k_work_schedule(&instance->poll_suspended,
		                POLL_SUSPENDED_INTERVAL);
		break;
	case USB_DC_RESUME:
		// We have to stop the work started on USB_DC_SUSPEND.
		atomic_set(&instance->suspended, 0);
		break;
	case USB_DC_SOF:
		if (instance->connected) {
			// If we are connected, we want to poll the key matrix
			// once per SOF, i.e. every millisecond.
			k_work_submit(&instance->sof);
		}
		break;
	default:
		break;
	}
}

void UsbKeyboard::static_on_sof(struct k_work *work) {
	UsbKeyboard *thisptr = CONTAINER_OF(work, UsbKeyboard, sof);
	thisptr->on_sof();
}

void UsbKeyboard::on_sof() {
	// We always assume that this code is called once per SOF, i.e. once per
	// millisecond. If the code is not called for some time, the only
	// negative sideeffect is that the latency caused by debouncing is
	// increased.

	keys->poll(1);

	// Check for newly pressed/released keys.
	KeyBitmap key_bitmap;
	keys->get_state(&key_bitmap);
	if (key_bitmap == prev_key_bitmap) {
		return;
	}

	// Send a keyboard report.
	if (boot_protocol) {
		SixKeySet six_keys = key_bitmap.to_6kro();
		hid_int_ep_write(hid_dev,
		                 six_keys.data,
		                 sizeof(six_keys.data),
		                 NULL);
	} else {
		// TODO: Proper NKRO
		SixKeySet six_keys = key_bitmap.to_6kro();
		hid_int_ep_write(hid_dev,
		                 six_keys.data,
		                 sizeof(six_keys.data),
		                 NULL);
	}
}

void UsbKeyboard::static_on_poll_suspended(struct k_work *work) {
	UsbKeyboard *thisptr = CONTAINER_OF(k_work_delayable_from_work(work),
	                                    UsbKeyboard,
	                                    poll_suspended);
	thisptr->on_poll_suspended();
}

void UsbKeyboard::on_poll_suspended() {
	if (atomic_get(&suspended) == 0) {
		return;
	}

	keys->poll(POLL_SUSPENDED_INTERVAL_MS);
	KeyBitmap key_bitmap;
	keys->get_state(&key_bitmap);
	if (key_bitmap.bit_is_set(KEY_ESCAPE)) {
		usb_wakeup_request();
	}

	k_work_schedule(&poll_suspended, POLL_SUSPENDED_INTERVAL);
}

void UsbKeyboard::on_protocol_change(const struct device *dev,
                                     uint8_t protocol) {
	ARG_UNUSED(dev);

	instance->boot_protocol = protocol == HID_PROTOCOL_BOOT;
}

UsbKeyboard *UsbKeyboard::instance = NULL;
const struct hid_ops UsbKeyboard::ops = {
	 .protocol_change = on_protocol_change,
};

