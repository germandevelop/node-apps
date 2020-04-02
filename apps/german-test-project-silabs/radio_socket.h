#ifndef RADIO_SOCKET_H_
#define RADIO_SOCKET_H_

#include "radio.h"

#pragma pack(push, 1)
typedef struct radio_message
{
	uint8_t command_id;
	uint8_t data[];

} radio_message_t;
#pragma pack(pop)

typedef void(*radio_message_receive_f)(radio_message_t const * const message, uint8_t data_size, void *user);

typedef struct radio_socket
{
	comms_layer_t *comms_radio;
	comms_receiver_t comms_receiver;
	comms_msg_t comms_message;

	radio_message_receive_f receive_callback;

} radio_socket_t;


int radio_socket_init(radio_socket_t * const radio_socket, radio_message_receive_f receive_callback);
void radio_socket_is_valid(radio_socket_t const * const radio_socket, bool * const is_valid);

int radio_socket_alloc_message(radio_socket_t * const radio_socket, radio_message_t **radio_message, uint8_t * const max_data_size);
int radio_socket_send_message(radio_socket_t * const radio_socket, uint8_t data_size, void *user);

#endif

