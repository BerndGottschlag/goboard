#include "usb.hpp"

UsbKeyboard::UsbKeyboard() {
	// TODO
}

UsbKeyboard::~UsbKeyboard() {
	// TODO
}

KeyboardProfile UsbKeyboard::get_profile() {
	return PROFILE_1;
}

void UsbKeyboard::set_profile(KeyboardProfile profile) {
	// There is only one USB connection, nothing to do here.
	(void)profile;
}
