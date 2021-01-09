#ifndef SERIAL_RECEIVER_H_
#define SERIAL_RECEIVER_H_

#include "serial_protocol.h"

typedef int(*serial_data_receive_f)(const uint8_t *data, uint8_t size);

typedef struct serial_receiver
{
	serial_protocol_t serial_protocol;
	serial_dispatcher_t serial_dispatcher;

	serial_data_receive_f receive_callback;

} serial_receiver_t;


int serial_receiver_init(serial_receiver_t * const serial_receiver,
			uint8_t protocol_id,
			serial_data_receive_f receive_callback);

#endif
