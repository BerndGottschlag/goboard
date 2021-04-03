#include "unifying.hpp"

#include "leds.hpp"

#define STACK_SIZE 1024
#define PRIORITY -1

K_THREAD_STACK_DEFINE(unifying_stack, STACK_SIZE);

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

