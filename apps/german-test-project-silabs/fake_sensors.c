#include "fake_sensors.h"

#include "endianness.h"

#include "loglevels.h"
#define __MODUUL__ "fake_sensors"
#define __LOG_LEVEL__ (LOG_LEVEL_fake_sensors)
#include "log.h"

#include "serial_receiver.h"
#include "motion_data.h"
#include "result.h"


// CONFIGURABLE VALUES
/************************************************************************************************/
#define SERIAL_PROTOCOL_ID 0x5D
/************************************************************************************************/


#pragma pack(push, 1)
typedef struct fake_data {

	int32_t speed;
	int32_t energy;
	bool is_movement;

} fake_data_t;
#pragma pack(pop)


static serial_receiver_t serial_receiver;

static bool is_radar_enabled; // ATOMIC ?????

static movement_detect_t detect_fake_movement_callback;
static motion_data_receive_t receive_fake_motion_data_callback;


static int fake_sensors_receive(const uint8_t *data, uint8_t length);

int fake_sensors_init(movement_detect_t detect_movement_callback,
			motion_data_receive_t receive_motion_data_callback)
{
	is_radar_enabled = false;

	detect_fake_movement_callback = detect_movement_callback;
	receive_fake_motion_data_callback = receive_motion_data_callback;

	if(serial_receiver_init(&serial_receiver, SERIAL_PROTOCOL_ID, fake_sensors_receive) == SUCCESS)
	{
		return SUCCESS;
	}
	return FAILURE;
}

void fake_sensors_turn_on_radar()
{
	is_radar_enabled = true;

	return;
}

void fake_sensors_turn_off_radar()
{
	is_radar_enabled = false;

	return;
}


int fake_sensors_receive(const uint8_t *data, uint8_t size)
{
	int ret_val = FAILURE;

	debugb1("data", data, size);

	if(size == sizeof(fake_data_t))
	{
		fake_data_t const * const raw_data = (fake_data_t*)data;

		fake_data_t fake_message;
		fake_message.speed = ntoh32(raw_data->speed);
		fake_message.energy = ntoh32(raw_data->energy);
		fake_message.is_movement = raw_data->is_movement;

		// process fake motion sensor
		detect_fake_movement_callback(fake_message.is_movement);
			
		// process fake radar
		if(is_radar_enabled == true)
		{
			motion_data_t motion_data;
			motion_data.speed = ((double)fake_message.speed) / 1000.0;
			motion_data.energy = ((double)fake_message.energy) / 1000.0;

			receive_fake_motion_data_callback(&motion_data);
		}

		ret_val = SUCCESS; 
	}
	else
	{
		err1("length %d != %d", (int)length, sizeof(sensor_data_t));
	}
	return ret_val;
}

