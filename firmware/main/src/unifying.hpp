#ifndef UNIFYING_HPP_INCLUDED
#define UNIFYING_HPP_INCLUDED

#include "mode_switch.hpp"
#include "key_matrix.hpp"
#include "keys.hpp"
#include "unifying_radio.hpp"

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

	void forget_pairing_info(KeyboardProfile profile);

	struct esb_payload pairing_request_1();
	struct esb_payload pairing_request_response_1();
	struct esb_payload pairing_accept_address();
	struct esb_payload pairing_request_2();
	struct esb_payload pairing_request_response_2();
	struct esb_payload pairing_request_3();
	struct esb_payload pairing_request_response_3();

	// TODO: This function probably should take an esb_payload instead!
	static uint8_t calculate_checksum(uint8_t *buffer, size_t length);

	Keys<KeyMatrix> *keys;
	Leds *leds;

	/// Profile as seen from the main thread (i.e., as returned by
	/// `get_profile()`).
	KeyboardProfile profile;
	volatile bool stop = false;

	/// Semaphore to interrupt sleeping in the thread.
	k_sem wakeup;
	/// Profile used by the thread.
	KeyboardProfile actual_profile;


	struct k_thread thread;
	k_tid_t tid;
	
	UnifyingState state;

	UnifyingRadio radio;

	// There can only be one instance of the unifying keyboard, and the
	// callbacks need a pointer to it.
	static UnifyingKeyboard *instance;
};

#endif

