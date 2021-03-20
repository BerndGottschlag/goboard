#ifndef USB_HPP_INCLUDED
#define USB_HPP_INCLUDED

#include "mode_switch.hpp"

/// USB HID keyboard implementation.
class UsbKeyboard {
public:
	UsbKeyboard();
	~UsbKeyboard();

	KeyboardProfile get_profile();
	void set_profile(KeyboardProfile profile);
};

#endif

