#include "bluetooth.hpp"

#include "exception.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <settings/settings.h>

BluetoothKeyboard::BluetoothKeyboard(Keys<KeyMatrix> *keys, Leds *leds):
		keys(keys), leds(leds) {

	// Enable bluetooth.
	if (bt_enable(static_on_bt_ready) != 0) {
		throw InitializationFailed("bt_enable failed");
	}

	bt_conn_cb_register(&conn_callbacks);
	// TODO
}

BluetoothKeyboard::~BluetoothKeyboard() {
	// TODO
}

KeyboardProfile BluetoothKeyboard::get_profile() {
	// TODO
	return PROFILE_1;
}

void BluetoothKeyboard::set_profile(KeyboardProfile profile) {
	// If we are currently advertising, we need to stop.
	// TODO

	// TODO
	(void)profile;
}

void BluetoothKeyboard::static_on_bt_ready(int err) {
	// TODO
	(void)err;

	// We only use one identity for the two profiles. Devices for the
	// inactive profile stay connected. The identity needs to be persistent,
	// so we load it here and, if none was loaded, create a new random
	// identity.
	settings_load();
	size_t addr_count = 0;
	bt_id_get(NULL, &addr_count);
	printk("%d BT addresses\n", addr_count);
	if (addr_count == 0) {
		// TODO: Error checking?
		bt_id_create(NULL, NULL);
	}
	bt_addr_le_t addr = { .type = 0 };
	addr_count = 1;
	bt_id_get(&addr, &addr_count);
	printk("address: %d/%02x:%02x:%02x:%02x:%02x:%02x\n",
	       addr.type,
	       addr.a.val[0],
	       addr.a.val[1],
	       addr.a.val[2],
	       addr.a.val[3],
	       addr.a.val[4],
	       addr.a.val[5]);

	// TODO
}
void BluetoothKeyboard::static_on_connected(struct bt_conn *conn,
                                            uint8_t err) {
	(void)conn;
	(void)err;
	// TODO
}

void BluetoothKeyboard::static_on_disconnected(struct bt_conn *conn,
                                               uint8_t reason) {
	(void)conn;
	(void)reason;
	// TODO
}

void BluetoothKeyboard::static_on_security_changed(struct bt_conn *conn,
                                                   bt_security_t level,
                                                   enum bt_security_err err) {
	(void)conn;
	(void)level;
	(void)err;
	// TODO
}

BluetoothKeyboard *BluetoothKeyboard::instance = NULL;

