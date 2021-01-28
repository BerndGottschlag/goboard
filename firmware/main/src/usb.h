
#ifndef USB_H_INCLUDED
#define USB_H_INCLUDED

void usb_init(void);
int usb_is_connected(void);
void usb_prepare_system_off(void);

void usb_poll(void);

#endif


