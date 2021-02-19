#include "unifying_radio.h"

#include "app_error.h"
#include "nrf_log.h"

#include <string.h>

static const uint8_t PAIRING_CHANNELS[] = {62, 8, 35, 65, 14, 41, 71, 17, 44, 74, 5};
static const uint8_t NORMAL_CHANNELS[] = {5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77};

static const uint8_t *channels = NULL;
static size_t channel_count = 0;
static size_t current_channel = 0;

static nrf_esb_payload_t tx_payload;
static nrf_esb_payload_t rx_payload;

static unifying_radio_rx_handler_t rx_handler;
static unifying_radio_tx_handler_t tx_handler;

static uint8_t failover_start_channel;
static uint8_t failover_loop;

static const uint8_t FAILOVER_LOOP_COUNT = 2;

static void unifying_radio_tx_failover(void) {
	uint32_t ret;

	/* try all channels FAILOVER_LOOP_COUNT times */
	current_channel = (current_channel + 1) % channel_count;
	nrf_esb_set_rf_channel(channels[current_channel]);

	if (current_channel == failover_start_channel) {
		failover_loop += 1;
		if (failover_loop == FAILOVER_LOOP_COUNT) {
			NRF_LOG_INFO("T- %x", tx_payload.data[1]);
			tx_handler(false);
			return;
		}
	}

	ret = nrf_esb_write_payload(&tx_payload);
	APP_ERROR_CHECK(ret);
}

static void unifying_radio_event_handler(const nrf_esb_evt_t *event) {
	switch (event->evt_id) {
		case NRF_ESB_EVENT_TX_SUCCESS:
			NRF_LOG_INFO("T+ %x", tx_payload.data[1]);
			if (tx_payload.data[1] == 0xd3) {
				NRF_LOG_HEXDUMP_INFO(tx_payload.data, tx_payload.length);
			}
			tx_handler(true);
			break;
		case NRF_ESB_EVENT_TX_FAILED:
			nrf_esb_flush_tx();
			unifying_radio_tx_failover();
			break;
		case NRF_ESB_EVENT_RX_RECEIVED:
			while (nrf_esb_read_rx_payload(&rx_payload) == NRF_SUCCESS) {
				NRF_LOG_INFO("R");
				NRF_LOG_HEXDUMP_INFO(rx_payload.data, rx_payload.length);
				rx_handler(&rx_payload);
			}
			break;
	}
}

void unifying_radio_init(unifying_radio_rx_handler_t rx_handler_,
                         unifying_radio_tx_handler_t tx_handler_) {
	uint32_t ret;
	nrf_esb_config_t nrf_esb_config = NRF_ESB_DEFAULT_CONFIG;
	nrf_esb_config.retransmit_count = 1;
	nrf_esb_config.retransmit_delay = 250;
	nrf_esb_config.selective_auto_ack = false;
	nrf_esb_config.protocol = NRF_ESB_PROTOCOL_ESB_DPL;
	nrf_esb_config.bitrate = NRF_ESB_BITRATE_2MBPS;
	nrf_esb_config.event_handler = unifying_radio_event_handler;
	nrf_esb_config.mode = NRF_ESB_MODE_PTX;
	nrf_esb_config.crc = NRF_ESB_CRC_16BIT;
	nrf_esb_config.tx_output_power = NRF_ESB_TX_POWER_8DBM; /* TODO */

	/* initialize the radio */
	ret = nrf_esb_init(&nrf_esb_config);
	APP_ERROR_CHECK(ret);
	ret = nrf_esb_set_address_length(5);
	APP_ERROR_CHECK(ret);

	rx_handler = rx_handler_;
	tx_handler = tx_handler_;
}

static void unifying_radio_set_channels(const uint8_t *new_channels, size_t count) {
	channels = new_channels;
	channel_count = count;
	current_channel = 0;

	nrf_esb_set_rf_channel(channels[current_channel]);
}

void unifying_radio_set_pairing_channels(void) {
	unifying_radio_set_channels(PAIRING_CHANNELS, sizeof(PAIRING_CHANNELS));
}

void unifying_radio_set_normal_channels(void) {
	unifying_radio_set_channels(NORMAL_CHANNELS, sizeof(NORMAL_CHANNELS));
}

void unifying_radio_send_packet(uint8_t pipe, uint8_t *data, size_t length) {
	tx_payload.pipe = pipe;
	tx_payload.length = length;
	memcpy(tx_payload.data, data, length);

	unifying_radio_retransmit();
}

void unifying_radio_retransmit(void) {
	uint32_t ret;

	failover_start_channel = current_channel;
	failover_loop = 0;

	ret = nrf_esb_write_payload(&tx_payload);
	APP_ERROR_CHECK(ret);
}

