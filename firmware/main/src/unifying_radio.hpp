#ifndef UNIFYING_RADIO_HPP_INCLUDED
#define UNIFYING_RADIO_HPP_INCLUDED

#include <stdint.h>
#include <stddef.h>

class UnifyingRadio {
public:
	UnifyingRadio();
	~UnifyingRadio();
private:

	const uint8_t *channels = NULL;
};

#endif

