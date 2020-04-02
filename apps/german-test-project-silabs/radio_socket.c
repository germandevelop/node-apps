#include "radio_socket.h"

#include "loglevels.h"
#define __MODUUL__ "radio_socket"
#define __LOG_LEVEL__ (LOG_LEVEL_sensor_sim & BASE_LOG_LEVEL)
#include "log.h"

#include "result.h"


#define PAN_ID 0x22
#define AM_ID 0xE1


static void radio_socket_started(comms_layer_t *layer, comms_status_t status, void *user);
static void radio_socket_message_received(comms_layer_t *layer, const comms_msg_t *msg, void *user);
static void radio_socket_send_done(comms_layer_t *comms, comms_msg_t *msg, comms_error_t result, void *user);

int radio_socket_init(radio_socket_t * const radio_socket, radio_message_receive_f receive_callback)
{
	radio_socket->receive_callback = receive_callback;

    	radio_socket->comms_radio = radio_init(DEFAULT_RADIO_CHANNEL, PAN_ID, DEFAULT_AM_ADDR);

    	if(radio_socket->comms_radio != NULL)
    	{
    		if(comms_start(radio_socket->comms_radio, radio_socket_started, (void*)radio_socket) == COMMS_SUCCESS)
    		{
        	    	debug1("radio rdy");

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

int radio_socket_send_message(radio_socket_t * const radio_socket, uint8_t data_size, void *user)
{
	const uint8_t message_size = sizeof(radio_message_t) + data_size;

	comms_set_packet_type(radio_socket->comms_radio, &radio_socket->comms_message, AM_ID);
	comms_am_set_destination(radio_socket->comms_radio, &radio_socket->comms_message, AM_BROADCAST_ADDR);
	comms_set_payload_length(radio_socket->comms_radio, &radio_socket->comms_message, message_size);

	if(comms_send(radio_socket->comms_radio, &radio_socket->comms_message, radio_socket_send_done, user) == COMMS_SUCCESS)
	{
		logger(LOG_DEBUG1, "snd %u", result);

		return SUCCESS;
	}
	else
	{
		logger(LOG_WARN1, "snd %u", result);
		err1("pckt");
	}
	return FAILURE;
}

void radio_socket_started(comms_layer_t *layer, comms_status_t status, void *user)
{
	radio_socket_t *radio_socket = (radio_socket_t*)user;

	if(comms_register_recv(layer, &radio_socket->comms_receiver, radio_socket_message_received, (void*)radio_socket, AM_ID) == COMMS_SUCCESS)
	{
    		debug1("started %d", status);
	}
	return;
}

void radio_socket_send_done(comms_layer_t *comms, comms_msg_t *msg, comms_error_t result, void *user)
{
	debug1("send done");

	return;
}

void radio_socket_message_received(comms_layer_t *layer, const comms_msg_t *msg, void *user)
{
	const uint8_t message_size = comms_get_payload_length(layer, msg);

    	if(message_size >= sizeof(radio_message_t))
    	{
		radio_socket_t *radio_socket = (radio_socket_t*)user;

        	radio_message_t *raw_radio_message = (radio_message_t*)comms_get_payload(layer, msg, message_size);

		const uint8_t data_size = message_size - sizeof(radio_message_t);

		radio_socket->receive_callback(raw_radio_message, data_size, user);
	}
    	else
    	{
		warn1("size %d", (unsigned int)message_size);
    	}
	return;
}

