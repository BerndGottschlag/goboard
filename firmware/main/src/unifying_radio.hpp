#ifndef UNIFYING_RADIO_HPP_INCLUDED
#define UNIFYING_RADIO_HPP_INCLUDED

#include "esb.h"

#include <stdint.h>
#include <stddef.h>

class UnifyingRadio {
public:
	UnifyingRadio();
	~UnifyingRadio();

	bool send_packet(const struct esb_payload *send,
	                 struct esb_payload *ack_payload);

	/// Causes all current and future operations to fail immediately.
	void shutdown();
private:

	const uint8_t *channels = NULL;
};

#endif

