#include "unifying.hpp"

#include "leds.hpp"

#include <settings/settings.h>
#include <string.h>

#define STACK_SIZE 1024
#define PRIORITY -1

static const uint8_t DEVICE_WPID[2] = {0x40, 0x03}; // K270

K_THREAD_STACK_DEFINE(unifying_stack, STACK_SIZE);

struct UnifyingDeviceInfo {
	uint8_t valid;
	uint8_t pairing_address[5];
	uint8_t pseudo_device_address[5];
	uint8_t device_serial[4];
};

struct UnifyingPairingInfo {
	uint8_t valid;
	uint8_t device_address[5];
	uint8_t pairing_device_nonce[4];
	uint8_t pairing_dongle_nonce[4];
	uint8_t dongle_wpid[2];
};

static UnifyingDeviceInfo device_info[2] = {{0}};
static UnifyingPairingInfo pairing_info[2] = {{0}};
static uint8_t device_key[2][16] = {{0}};

static void derive_device_key(int profile) {
	UnifyingPairingInfo *info = &pairing_info[profile];
	uint8_t material[16];
	material[0] = info->device_address[4];
	material[1] = info->device_address[3];
	material[2] = info->device_address[2];
	material[3] = info->device_address[1];
	memcpy(&material[4], DEVICE_WPID, 2);
	memcpy(&material[6], info->dongle_wpid, 2);
	memcpy(&material[8], info->pairing_device_nonce, 4);
	memcpy(&material[12], info->pairing_dongle_nonce, 4);

	device_key[profile][2] = material[0];
	device_key[profile][1] = material[1] ^ 0xff;
	device_key[profile][5] = material[2] ^ 0xff;
	device_key[profile][3] = material[3];
	device_key[profile][14] = material[4];
	device_key[profile][11] = material[5];
	device_key[profile][9] = material[6];
	device_key[profile][0] = material[7];
	device_key[profile][8] = material[8];
	device_key[profile][6] = material[9] ^ 0x55;
	device_key[profile][4] = material[10];
	device_key[profile][15] = material[11];
	device_key[profile][10] = material[12] ^ 0xff;
	device_key[profile][12] = material[13];
	device_key[profile][7] = material[14];
	device_key[profile][13] = material[15] ^ 0x55;
}

static int unifying_settings_get(const char *name,
                                 char *val,
                                 int val_len_max) {
	static const char *names[2] = { "pairing_info_1", "pairing_info_2" };
	for (int i = 0; i < 2; i++) {
		const char *next;
		if (settings_name_steq(name, names[i], &next) && !next) {
			val_len_max = MIN(val_len_max,
			                  (int)sizeof(pairing_info[i]));
			memcpy(val, &pairing_info[i], val_len_max);
			return val_len_max;
		}
	}

	return -ENOENT;
}

static int unifying_profile_settings_set(int profile,
                                         const char *name,
                                         size_t len,
                                         settings_read_cb read_cb,
                                         void *cb_arg) {
	const char *next;
	size_t name_len = settings_name_next(name, &next);
	if (next) {
		return -ENOENT;
	}
	if (!strncmp(name, "pairing_info", name_len)) {
		int ret = -EINVAL;
		if (len == sizeof(pairing_info[profile])) {
			ret = read_cb(cb_arg,
			              &pairing_info[profile],
			              sizeof(pairing_info[profile]));
		}
		if (ret >= 0) {
			derive_device_key(i);
			return 0;
		} else {
			// Error, reset the pairing info.
			memset(&pairing_info[profile],
			       0,
			       sizeof(pairing_info[profile]));
			memset(&device_key[profile],
			       0,
			       sizeof(device_key[profile]));
			return ret;
		}
	}

	return -ENOENT;
}

static int unifying_settings_set(const char *name,
                                 size_t len,
                                 settings_read_cb read_cb,
                                 void *cb_arg) {
	const char *next;
	size_t name_len = settings_name_next(name, &next);
	if (next) {
		return -ENOENT;
	}

	static const char *names[2] = { "pairing_info_1", "pairing_info_2" };
	for (int i = 0; i < 2; i++) {
		if (!strncmp(name, names[i], name_len)) {
			int ret = -EINVAL;
			if (len == sizeof(pairing_info[i])) {
				ret = read_cb(cb_arg,
				              &pairing_info[i],
				              sizeof(pairing_info[i]));
			}
			if (ret >= 0) {
				derive_device_key(i);
				return 0;
			} else {
				// Error, reset the pairing info.
				memset(&pairing_info[i],
				       0,
				       sizeof(pairing_info[i]));
				memset(&device_key[i],
				       0,
				       sizeof(device_key[i]));
				return ret;
			}
		}
	}

	return -ENOENT;
}

static int unifying_settings_commit(void) {
	return 0;
}

static int unifying_settings_export(int (*cb)(const char *name,
                                              const void *value,
                                              size_t val_len)) {
	(void)cb("unifying/0/device_info",
	         &device_info[0],
	         sizeof(device_info[0]));
	(void)cb("unifying/0/pairing_info",
	         &pairing_info[0],
	         sizeof(pairing_info[0]));
	(void)cb("unifying/1/device_info",
	         &device_info[1],
	         sizeof(device_info[1]));
	(void)cb("unifying/1/pairing_info",
	         &pairing_info[1],
	         sizeof(pairing_info[1]));
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(beta,
                               "unifying",
                               unifying_settings_get,
                               unifying_settings_set,
                               unifying_settings_commit,
                               unifying_settings_export);

UnifyingKeyboard::UnifyingKeyboard(Keys<KeyMatrix> *keys,
                                   Leds *leds,
				   KeyboardProfile profile):
		keys(keys), leds(leds), profile(profile),
		actual_profile(profile) {
	k_sched_lock();
	if (instance == NULL) {
		instance = this;
	}
	k_sched_unlock();

	// Determine the initial state - if we have a key, we want to reconnect.
	// TODO
	state = UNIFYING_IDLE;

	// To simplify control flow, the keyboard runs a separate thread.
	k_sem_init(&wakeup, 0, 1);
	tid = k_thread_create(&thread, unifying_stack,
	                      K_THREAD_STACK_SIZEOF(unifying_stack),
	                      static_thread_entry,
	                      NULL, NULL, NULL,
	                      PRIORITY, 0, K_NO_WAIT);
}

UnifyingKeyboard::~UnifyingKeyboard() {
	stop = true;
	k_sem_give(&wakeup);
	k_thread_join(&thread, K_FOREVER);

	instance = NULL;
}

KeyboardProfile UnifyingKeyboard::get_profile() {
	return profile;
}

void UnifyingKeyboard::set_profile(KeyboardProfile profile) {
	// Concurrent access, the thread reads this variable. We could
	// potentially use an atomic pointer instead?
	k_sched_lock();
	this->profile = profile;
	k_sched_unlock();

	k_sem_give(&wakeup);
}

void UnifyingKeyboard::static_thread_entry(void *arg1, void *arg2, void *arg3) {
	instance->thread_entry(arg1, arg2, arg3);
}

void UnifyingKeyboard::thread_entry(void *arg1, void *arg2, void *arg3) {
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		switch (state) {
		case UNIFYING_IDLE:
			state = idle();
			break;
		case UNIFYING_PAIRING:
			state = pairing();
			break;
		case UNIFYING_RECONNECTING:
			state = reconnecting();
			break;
		case UNIFYING_CONNECTED:
			state = connected();
			break;
		case UNIFYING_STOPPING:
			break;
		}
	}
}

UnifyingState UnifyingKeyboard::idle() {
	leds->set_mode(MODE_LED_DISCONNECTED);
	while (true) {
		// Poll the keyboard once every 50ms.
		UnifyingState next_state;
		KeyBitmap key_bitmap;
		if (poll_keyboard(50, &next_state, &key_bitmap)) {
			return next_state;
		}

		if (process_fn_keys(&key_bitmap, &next_state)) {
			return next_state;
		}
	}
}

UnifyingState UnifyingKeyboard::pairing() {
	leds->set_mode(MODE_LED_PAIRING);
	// Try to pair the keyboard once, and then transition to connected or
	// idle.
	// TODO
	return UNIFYING_STOPPING;
}

UnifyingState UnifyingKeyboard::reconnecting() {
	leds->set_mode(MODE_LED_RECONNECTING);
	static const int KEY_INTERVAL = 50;
	int reconnect_counter = 0;
	while (true) {
		// Poll the keyboard once every 50ms.
		UnifyingState next_state;
		KeyBitmap key_bitmap;
		if (poll_keyboard(KEY_INTERVAL, &next_state, &key_bitmap)) {
			return next_state;
		}

		if (process_fn_keys(&key_bitmap, &next_state)) {
			return next_state;
		}

		// Once every second, try to reconnect using the existing
		// pairing information.
		reconnect_counter++;
		if (reconnect_counter >= 1000 / KEY_INTERVAL) {
			// TODO
		}
	}
}

UnifyingState UnifyingKeyboard::connected() {
	leds->set_mode(MODE_LED_CONNECTED);
	while (true) {
		// Poll the keyboard once every 10ms.
		UnifyingState next_state;
		KeyBitmap key_bitmap;
		if (poll_keyboard(10, &next_state, &key_bitmap)) {
			return next_state;
		}

		// If the pressed keys changed, send a keyboard report. This
		// should be done before evaluating any FN keys to allow the
		// keyboard to reset all previously pressed keys.
		// TODO

		// Else, send a keepalive packet.
		// TODO

		// If we received anything from the host, process the packets.
		// TODO

		if (process_fn_keys(&key_bitmap, &next_state)) {
			return next_state;
		}

		// If a sufficient number of packets in a row failed to be
		// delivered, try to reconnect.
		// TODO
	}
}

bool UnifyingKeyboard::poll_keyboard(int timeout,
                                     UnifyingState *next_state,
                                     KeyBitmap *key_bitmap) {
	if (k_sem_take(&wakeup, K_MSEC(timeout)) != 0) {
		// Timeout.
		return false;
	}
	if (stop) {
		*next_state = UNIFYING_STOPPING;
		return true;
	}
	// If the profile was changed, select the appropriate state.
	k_sched_lock();
	bool profile_changed = false;
	if (profile != actual_profile) {
		profile_changed = true;
		actual_profile = profile;
	}
	k_sched_unlock();

	if (profile_changed) {
		// TODO
		*next_state = state;
		return true;
	}

	keys->poll(timeout);
	keys->get_state(key_bitmap);

	return false;
}

bool UnifyingKeyboard::process_fn_keys(KeyBitmap *key_bitmap,
                                       UnifyingState *next_state) {
	// If the FN combination for bluetooth was pressed, trigger a mode
	// change.
	if (key_bitmap->bit_is_set(FN_KEY_BLUETOOTH)) {
		// TODO: Notify the main thread.
		*next_state = UNIFYING_STOPPING;
		return true;
	}

	// If the FN combination for pairing or removal of all pairing
	// information was pressed, change the state.
	if (key_bitmap->bit_is_set(FN_KEY_PAIR)) {
		// TODO: Forget existing key.
		*next_state = UNIFYING_PAIRING;
		return true;
	}
	if (key_bitmap->bit_is_set(FN_KEY_UNPAIR_ALL)) {
		// TODO: Forget existing key.
		*next_state = UNIFYING_IDLE;
		return true;
	}
	return false;
}

UnifyingKeyboard *UnifyingKeyboard::instance = NULL;

