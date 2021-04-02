#ifndef UNIFYING_HPP_INCLUDED
#define UNIFYING_HPP_INCLUDED

#include "mode_switch.hpp"
#include "key_matrix.hpp"
#include "keys.hpp"

class Leds;

enum UnifyingState {
	UNIFYING_STOPPING,
	UNIFYING_IDLE,
	UNIFYING_PAIRING,
	UNIFYING_RECONNECTING,
	UNIFYING_CONNECTED,
};

/// Logitech Unifying keyboard implementation.
class UnifyingKeyboard {
public:
	UnifyingKeyboard(Keys<KeyMatrix> *keys, Leds *leds);
	~UnifyingKeyboard();

	KeyboardProfile get_profile();
	void set_profile(KeyboardProfile profile);
private:
	static void static_thread_entry(void *arg1, void *arg2, void *arg3);
	void thread_entry(void *arg1, void *arg2, void *arg3);

	UnifyingState idle();
	UnifyingState pairing();
	UnifyingState reconnecting();
	UnifyingState connected();

	Keys<KeyMatrix> *keys;
	Leds *leds;

	struct k_thread thread;
	k_tid_t tid;
	
	UnifyingState state;

	// There can only be one instance of the unifying keyboard, and the
	// callbacks need a pointer to it.
	static UnifyingKeyboard *instance;
};

#endif


