
#include "usb.h"

#include "app_usbd.h"
#include "app_usbd_hid_kbd.h"

static int is_connected = false;

static void keyboard_event_handler(const app_usbd_class_inst_t *inst,
                                   app_usbd_hid_user_event_t event) {
	UNUSED_PARAMETER(inst);
	switch (event) {
		case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
			/* TODO: update LEDs */
			break;
		case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
			break;
		case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
			UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(inst));
			break;
		case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
			UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(inst));
			break;
		default:
			break;
	}
}

APP_USBD_HID_KBD_GLOBAL_DEF(hid_keyboard,
                            0,
                            NRF_DRV_USBD_EPIN2,
                            keyboard_event_handler,
                            APP_USBD_HID_SUBCLASS_BOOT);

static void usb_state_event_handler(app_usbd_event_type_t event) {
	switch (event) {
		case APP_USBD_EVT_DRV_SOF:
			break;
		case APP_USBD_EVT_DRV_RESET:
			break;
		case APP_USBD_EVT_DRV_SUSPEND:
			/* allow sleep mode */
			app_usbd_suspend_req();
			break;
		case APP_USBD_EVT_DRV_RESUME:
			break;
		case APP_USBD_EVT_STARTED:
			break;
		case APP_USBD_EVT_STOPPED:
			app_usbd_disable();
			break;
		case APP_USBD_EVT_POWER_DETECTED:
			is_connected = true;
			if (!nrf_drv_usbd_is_enabled()) {
				app_usbd_enable();
			}
			break;
		case APP_USBD_EVT_POWER_REMOVED:
			is_connected = false;
			app_usbd_stop();
			break;
		case APP_USBD_EVT_POWER_READY:
			app_usbd_start();
			break;
		default:
			break;
	}
}


void usb_init(void) {
	ret_code_t ret;
	static const app_usbd_config_t usbd_config = {
		.ev_state_proc = usb_state_event_handler
	};
	const app_usbd_class_inst_t *class_instance;

	/* initialize USB */
	ret = app_usbd_init(&usbd_config);
	APP_ERROR_CHECK(ret);

	/* register the HID class */
	class_instance = app_usbd_hid_kbd_class_inst_get(&hid_keyboard);
	ret = app_usbd_class_append(class_instance);
	APP_ERROR_CHECK(ret);

	/* we need to track whether USB is connected */
	ret = app_usbd_power_events_enable();
	APP_ERROR_CHECK(ret);
}

bool usb_is_connected(void) {
	return is_connected;
}

void usb_prepare_system_off(void) {
	/* TODO */
}

void usb_poll(void) {
	while (app_usbd_event_queue_process()) {}
}

