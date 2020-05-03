#ifndef RADIO_SOCKET_H_
#define RADIO_SOCKET_H_

#include "mist_comm_am.h"

#pragma pack(push, 1)
typedef struct radio_message
{
	uint8_t seq;
	uint8_t frags: 4; // Comes second, but specified first because of endianness
	uint8_t frag: 4;  // Comes first, but specified second because of endianness

	uint8_t data[];

} radio_message_t;
#pragma pack(pop)

typedef void(*radio_message_receive_f)(radio_message_t const * const message, uint8_t data_size, void *user_data);
typedef void(*radio_sending_error_catch_f)(void *user_data);

typedef struct radio_socket
{
	comms_layer_t *comms_radio;
	am_addr_t am_destination_address;
	am_id_t am_port;

	comms_msg_t comms_message;
	radio_sending_error_catch_f catch_error_callback;
	void *send_user_data;

	comms_receiver_t comms_receiver;
	radio_message_receive_f receive_callback;
	void *receive_user_data;

} radio_socket_t;

void radio_socket_init(radio_socket_t * const radio_socket,
			comms_layer_t *comms_radio,
			am_addr_t am_destination_address,
			am_id_t am_port,
			radio_sending_error_catch_f catch_error_callback);

int radio_socket_set_receiver(radio_socket_t * const radio_socket,
				radio_message_receive_f receive_callback,
				void *user_data);

void radio_socket_is_valid(radio_socket_t const * const radio_socket, bool * const is_valid);

int radio_socket_alloc_message(radio_socket_t * const radio_socket, radio_message_t **radio_message, uint8_t * const max_data_size);
int radio_socket_send_message(radio_socket_t * const radio_socket, uint8_t data_size, void *user_data);

#endif

