#include "serial_receiver.h"

#include "serial_hdlc.h"

#include "loglevels.h"
#define __MODUUL__ "serial_data_receiver"
#define __LOG_LEVEL__ (LOG_LEVEL_sensor_sim & BASE_LOG_LEVEL)
#include "log.h"

#include "result.h"


static bool serial_receiver_receive(uint8_t dispatch, const uint8_t data[], uint8_t length, void * user);
static bool serial_receiver_snoop(uint8_t dispatch, const uint8_t data[], uint8_t length, void * user);
static void serial_receiver_send_done(uint8_t dispatch, const uint8_t data[], uint8_t length, bool acked, void *user);

int serial_receiver_init(serial_receiver_t * const serial_receiver,
			uint8_t protocol_id,
			serial_data_receive_f receive_callback)
{
	serial_receiver->receive_callback = receive_callback;

	// Initialize the serial port and protocol
	if(serial_protocol_init(&serial_receiver->serial_protocol, serial_hdlc_send, serial_receiver_snoop) == true)
	{
		serial_hdlc_init(serial_protocol_receive_generic, (void*)&serial_receiver->serial_protocol);
		serial_hdlc_enable();

		// Sensor data RX, TX and serial_senddone not used (but probably required)
		if(serial_protocol_add_dispatcher(&serial_receiver->serial_protocol, protocol_id, &serial_receiver->serial_dispatcher,
							serial_receiver_receive, serial_receiver_send_done, (void*)serial_receiver) == true)
		{
			return SUCCESS;
		}
	}
	return FAILURE;
}


bool serial_receiver_receive(uint8_t dispatch, const uint8_t data[], uint8_t length, void *user)
{
	serial_receiver_t *serial_receiver = (serial_receiver_t*)user;

	if(serial_receiver->receive_callback(data, length) == SUCCESS)
	{
		return true;
	}
	return false;
}

bool serial_receiver_snoop(uint8_t dispatch, const uint8_t data[], uint8_t length, void *user)
{
	debugb1("sdata", data, length);

	return true;
}

void serial_receiver_send_done(uint8_t dispatch, const uint8_t data[], uint8_t length, bool acked, void *user)
{
	debugb1("snt(a:%d) %02X", data, length, (int)acked, (unsigned int)dispatch);

	return;
}

