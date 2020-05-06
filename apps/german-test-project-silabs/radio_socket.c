#include "radio_socket.h"

#include "DeviceSignature.h"

#include "loglevels.h"
#define __MODUUL__ "radio_socket"
#define __LOG_LEVEL__ (LOG_LEVEL_radio_socket)
#include "log.h"

#include "result.h"


typedef struct socket_user_data
{
	radio_socket_t socket;
	void *user_data;

} socket_user_data_t;


static void radio_socket_message_received(comms_layer_t *layer, const comms_msg_t *msg, void *user_data);
static void radio_socket_message_sent(comms_layer_t *comms, comms_msg_t *msg, comms_error_t result, void *user_data);

void radio_socket_init(radio_socket_t * const radio_socket,
			comms_layer_t *comms_radio,
			am_addr_t am_destination_address,
			am_id_t am_port,
			radio_sending_error_catch_f catch_error_callback)
{
	radio_socket->comms_radio = comms_radio;
	radio_socket->am_destination_address = am_destination_address;
	radio_socket->am_port = am_port;

	radio_socket->catch_error_callback = catch_error_callback;
	radio_socket->send_user_data = NULL;

	radio_socket->receive_callback = NULL;
	radio_socket->receive_user_data = NULL;

	return;
}

int radio_socket_set_receiver(radio_socket_t * const radio_socket,
				radio_message_receive_f receive_callback,
				void *user_data)
{
	comms_deregister_recv(radio_socket->comms_radio, &radio_socket->comms_receiver);

	radio_socket->receive_callback = receive_callback;
	radio_socket->receive_user_data = user_data;

    	if(radio_socket->comms_radio != NULL)
    	{
		if(comms_register_recv(radio_socket->comms_radio, &radio_socket->comms_receiver,
					radio_socket_message_received, (void*)radio_socket, radio_socket->am_port) == COMMS_SUCCESS)
		{
			return SUCCESS;
		}
	}
    	return FAILURE;
}

void radio_socket_is_valid(radio_socket_t const * const radio_socket, bool * const is_valid)
{
	*is_valid = (comms_status(radio_socket->comms_radio) == COMMS_STARTED);

	return;
}

int radio_socket_alloc_message(radio_socket_t * const radio_socket, radio_message_t **radio_message, uint8_t * const max_data_size)
{
	const uint8_t max_radio_msg_size = comms_get_payload_max_length(radio_socket->comms_radio);

	if(max_radio_msg_size > sizeof(radio_message_t))
	{
		comms_init_message(radio_socket->comms_radio, &radio_socket->comms_message);

		*radio_message = (radio_message_t*)comms_get_payload(radio_socket->comms_radio, &radio_socket->comms_message, max_radio_msg_size);

		if(radio_message != NULL)
		{
			*max_data_size = max_radio_msg_size - sizeof(radio_message_t);

			return SUCCESS;
		}
	}
	return FAILURE;
}

int radio_socket_send_message(radio_socket_t * const radio_socket, uint8_t data_size, void *user_data)
{
	// try to prepare message
	const uint8_t message_size = sizeof(radio_message_t) + data_size;
	const uint8_t message_retries_count = 4U;
	const uint32_t message_timeout = 100U;

	comms_set_packet_type(radio_socket->comms_radio, &radio_socket->comms_message, radio_socket->am_port);
	comms_am_set_destination(radio_socket->comms_radio, &radio_socket->comms_message, radio_socket->am_destination_address);
	comms_set_payload_length(radio_socket->comms_radio, &radio_socket->comms_message, message_size);

	if(comms_set_retries(radio_socket->comms_radio, &radio_socket->comms_message, message_retries_count) == COMMS_SUCCESS)
	{
		if(comms_set_timeout(radio_socket->comms_radio, &radio_socket->comms_message, message_timeout) == COMMS_SUCCESS)
		{
			if(comms_set_ack_required(radio_socket->comms_radio, &radio_socket->comms_message, true) == COMMS_SUCCESS)
			{
				// try to send message
				radio_socket->send_user_data = user_data;

				if(comms_send(radio_socket->comms_radio, &radio_socket->comms_message, radio_socket_message_sent, (void*)radio_socket) == COMMS_SUCCESS)
				{
					return SUCCESS;
				}
			}
		}
	}
	// error occured
	err1("send error");

	return FAILURE;
}

void radio_socket_message_sent(comms_layer_t *comms, comms_msg_t *msg, comms_error_t result, void *user_data)
{
	if(result != COMMS_SUCCESS)
	{
		err1("send error");

		radio_socket_t *radio_socket = (radio_socket_t*)user_data;

		if(radio_socket->catch_error_callback != NULL)
		{
			radio_socket->catch_error_callback(radio_socket->send_user_data);
		}
	}
	else
	{
		debug1("send done");
	}
	return;
}

void radio_socket_message_received(comms_layer_t *layer, const comms_msg_t *msg, void *user_data)
{
	const uint8_t message_size = comms_get_payload_length(layer, msg);

    	if(message_size >= sizeof(radio_message_t))
    	{
		radio_socket_t *radio_socket = (radio_socket_t*)user_data;

		if(radio_socket->receive_callback != NULL)
		{
			radio_message_t *raw_radio_message = (radio_message_t*)comms_get_payload(layer, msg, message_size);

			const uint8_t data_size = message_size - sizeof(radio_message_t);

			radio_socket->receive_callback(raw_radio_message, data_size, radio_socket->receive_user_data);
		}
	}
    	else
    	{
		warn1("size %d", (unsigned int)message_size);
    	}
	return;
}

