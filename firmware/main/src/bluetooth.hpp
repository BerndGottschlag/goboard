#ifndef BLUETOOTH_HPP_INCLUDED
#define BLUETOOTH_HPP_INCLUDED

#include "mode_switch.hpp"
#include "key_matrix.hpp"
#include "keys.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

class Leds;

/// Bluetooth HIDS keyboard implementation.
class BluetoothKeyboard {
public:
	BluetoothKeyboard(Keys<KeyMatrix> *keys, Leds *leds);
	~BluetoothKeyboard();

	KeyboardProfile get_profile();
	void set_profile(KeyboardProfile profile);
private:
	static void static_on_bt_ready(int err);
	static void static_on_connected(struct bt_conn *conn,
	                                uint8_t err);
	static void static_on_disconnected(struct bt_conn *conn,
	                                   uint8_t reason);
	static void static_on_security_changed(struct bt_conn *conn,
	                                       bt_security_t level,
	                                       enum bt_security_err err);

	Keys<KeyMatrix> *keys;
	Leds *leds;

	// There can only be one instance of the BT keyboard, and the BT
	// callbacks need a pointer to it.
	static BluetoothKeyboard *instance;

	struct bt_conn_cb conn_callbacks = {
		.connected = static_on_connected,
		.disconnected = static_on_disconnected,
		.security_changed = static_on_security_changed,
	};
};

#endif

