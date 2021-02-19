#ifndef UNIFYING_RADIO_H_INCLUDED
#define UNIFYING_RADIO_H_INCLUDED

#include "nrf_esb.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*unifying_radio_rx_handler_t)(nrf_esb_payload_t *packet); // TODO: parameter
typedef void (*unifying_radio_tx_handler_t)(bool success);

void unifying_radio_init(unifying_radio_rx_handler_t rx_handler,
                         unifying_radio_tx_handler_t tx_handler);

void unifying_radio_set_pairing_channels(void);
void unifying_radio_set_normal_channels(void);

void unifying_radio_send_packet(uint8_t pipe, uint8_t *data, size_t length);
void unifying_radio_retransmit(void);

#endif

