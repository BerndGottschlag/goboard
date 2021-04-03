#ifndef UNIFYING_HPP_INCLUDED
#define UNIFYING_HPP_INCLUDED

#include "mode_switch.hpp"
#include "key_matrix.hpp"
#include "keys.hpp"

#include <kernel.h>

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
	UnifyingKeyboard(Keys<KeyMatrix> *keys,
	                 Leds *leds,
	                 KeyboardProfile profile);
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

	bool poll_keyboard(int timeout,
	                   UnifyingState *next_state,
	                   KeyBitmap *key_bitmap);
	bool process_fn_keys(KeyBitmap *key_bitmap, UnifyingState *next_state);

	Keys<KeyMatrix> *keys;
	Leds *leds;

	/// Profile as seen from the main thread (i.e., as returned by
	/// `get_profile()`).
	KeyboardProfile profile;
	bool stop = false;

	/// Semaphore to interrupt sleeping in the thread.
	k_sem wakeup;
	/// Profile used by the thread.
	KeyboardProfile actual_profile;

	struct k_thread thread;
	k_tid_t tid;
	
	UnifyingState state;

	// There can only be one instance of the unifying keyboard, and the
	// callbacks need a pointer to it.
	static UnifyingKeyboard *instance;
};

#endif

