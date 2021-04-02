#include "unifying.hpp"

#define STACK_SIZE 1024
#define PRIORITY -1

K_THREAD_STACK_DEFINE(unifying_stack, STACK_SIZE);

UnifyingKeyboard::UnifyingKeyboard(Keys<KeyMatrix> *keys, Leds *leds):
		keys(keys), leds(leds) {
	k_sched_lock();
	if (instance == NULL) {
		instance = this;
	}
	k_sched_unlock();

	// Determine the initial state - if we have a key, we want to reconnect.
	// TODO
	state = UNIFYING_IDLE;

	// To simplify control flow, the keyboard runs a separate thread.
	tid = k_thread_create(&thread, unifying_stack,
	                      K_THREAD_STACK_SIZEOF(unifying_stack),
	                      static_thread_entry,
	                      NULL, NULL, NULL,
	                      PRIORITY, 0, K_NO_WAIT);
}

UnifyingKeyboard::~UnifyingKeyboard() {
	// TODO

	instance = NULL;
}

KeyboardProfile UnifyingKeyboard::get_profile() {
	// TODO
}

void UnifyingKeyboard::set_profile(KeyboardProfile profile) {
	// TODO
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
	// Poll the keyboard once every 50ms.
	// TODO
	// If the profile was changed, select the appropriate state.
	// TODO
	// If the FN combination for bluetooth was pressed, trigger a mode
	// change.
	// TODO
	// If the FN combination for pairing or removal of all pairing
	// information was pressed, change the state.
	// TODO
	return UNIFYING_STOPPING;
}

UnifyingState UnifyingKeyboard::pairing() {
	// Try to pair the keyboard once, and then transition to connected or
	// idle.
	// TODO
	return UNIFYING_STOPPING;
}

UnifyingState UnifyingKeyboard::reconnecting() {
	// Poll the keyboard once every 50ms.
	// TODO
	// If the profile was changed, select the appropriate state.
	// TODO
	// If the FN combination for bluetooth was pressed, trigger a mode
	// change.
	// TODO
	// If the FN combination for pairing or removal of all pairing
	// information was pressed, change the state.
	// TODO
	// Once every second, try to reconnect using the existing pairing
	// information.
	// TODO
	return UNIFYING_STOPPING;
}

UnifyingState UnifyingKeyboard::connected() {
	// Poll the keyboard once every 10ms.
	// TODO
	// If the profile was changed, select the appropriate state.
	// TODO
	// If the FN combination for bluetooth was pressed, trigger a mode
	// change.
	// TODO
	// If the FN combination for pairing or removal of all pairing
	// information was pressed, change the state.
	// TODO
	// If the pressed keys changed, send a keyboard report.
	// TODO
	// Else, send a keepalive packet.
	// TODO
	// If we received anything from the host, process the packets.
	// TODO
	// If a sufficient number of packets in a row failed to be delivered,
	// try to reconnect.
	// TODO
	return UNIFYING_STOPPING;
}

UnifyingKeyboard *UnifyingKeyboard::instance = NULL;

