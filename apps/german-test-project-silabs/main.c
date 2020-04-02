#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "retargetserial.h"

#include "cmsis_os2.h"

#include "platform.h"

#include "SignatureArea.h"
#include "DeviceSignature.h"

#include "loggers_ext.h"
#if defined(LOGGER_LDMA)
#include "logger_ldma.h"
#elif defined(LOGGER_FWRITE)
#include "logger_fwrite.h"
#endif//LOGGER_*

#include "loglevels.h"
#define __MODUUL__ "main"
#define __LOG_LEVEL__ (LOG_LEVEL_main & BASE_LOG_LEVEL)
#include "log.h"

// Include the information header binary
#include "incbin.h"
INCBIN(Header, "header.bin");

#include "platform_push_button.h"
#include "fake_sensors.h"
#include "tracking_partner.h"
#include "tracking_session.h"
#include "radar_data.h"
#include "tracking_report.h"
#include "result.h"


#define MOTION_SENSOR_FLAG_MASK 		0B00000001
#define START_TRACKING_REQUEST_FLAG_MASK 	0B00000010
#define STOP_TRACKING_REQUEST_FLAG_MASK 	0B00000100

#define PUSH_BUTTON_MASK 0B00000001



static osThreadId_t tracking_state_thread_id;

static osThreadId_t speed_calculation_thread_id;
static osMessageQueueId_t radar_data_queue;

static osMutexId_t tracking_system_mutex;

static osThreadId_t main_thread_id;

static tracking_partner_t tracking_partner;
static tracking_session_t tracking_session;


static void on_sensor_movement_detetcted_ISR()
{
	info1("SENSOR ISR");

	osThreadFlagsSet(tracking_state_thread_id, MOTION_SENSOR_FLAG_MASK);	

	return;
}

static void on_partner_tracking_requested_ISR(bool is_turn_on)
{
	if(is_turn_on == true)
	{
		info1("START TRACKING REQUEST ISR");

		osThreadFlagsSet(tracking_state_thread_id, START_TRACKING_REQUEST_FLAG_MASK);
	}
	else
	{
		info1("STOP TRACKING REQUEST ISR");

		osThreadFlagsSet(tracking_state_thread_id, STOP_TRACKING_REQUEST_FLAG_MASK);
	}
	return;
}

static void on_partner_report_received_ISR(tracking_report_t const * const tracking_report)
{
	info1("REPORT RECEIVED ISR: %d", tracking_report->speed);

	return;
}

static void tracking_state_thread()
{
    	while(true)
    	{
		const uint32_t flags = osThreadFlagsWait(MOTION_SENSOR_FLAG_MASK | START_TRACKING_REQUEST_FLAG_MASK | STOP_TRACKING_REQUEST_FLAG_MASK, osFlagsWaitAny, osWaitForever);

		if(osMutexAcquire(tracking_system_mutex, osWaitForever) == osOK)
		{
			if((flags & MOTION_SENSOR_FLAG_MASK) > 0U)
			{
				bool is_tracking_started;
				tracking_session_register_motion(&tracking_session, &is_tracking_started);

				if(is_tracking_started == true)
				{
					osMessageQueueReset(radar_data_queue);

					fake_sensors_turn_on_radar();
					info1("TURN ON RADAR");

					tracking_partner_turn_on_radar(&tracking_partner);
					info1("START TRACKING REQUEST IS SENT");
				}
				else
				{
					bool is_speed_detected;
					tracking_session_is_speed_detected(&tracking_session, &is_speed_detected);

					if(is_speed_detected == false)
					{
						tracking_report_t tracking_report;

						tracking_session_get_speed(&tracking_session, &tracking_report.speed);
						info1("MEASURED SPEED: %d", tracking_report.speed);

						tracking_partner_send_tracking_report(&tracking_partner, &tracking_report);
						info1("TRACKING REPORT IS SENT");
					}
					tracking_partner_turn_off_radar(&tracking_partner);
					info1("STOP TRACKING REQUEST IS SENT");

					fake_sensors_turn_off_radar();
					info1("TURN OFF RADAR");
				}
			}
			if((flags & STOP_TRACKING_REQUEST_FLAG_MASK) > 0U)
			{
				tracking_session_stop(&tracking_session);

				fake_sensors_turn_off_radar();
				info1("TURN OFF RADAR");
			}
			if((flags & START_TRACKING_REQUEST_FLAG_MASK) > 0U)
			{
				osMessageQueueReset(radar_data_queue);

				tracking_session_start(&tracking_session);

				fake_sensors_turn_on_radar();
				info1("TURN ON RADAR");
			}
			osMutexRelease(tracking_system_mutex);
		}
    	}
	return;
}


static void on_radar_data_received_ISR(radar_data_t const * const radar_data)
{
	info1("RADAR DATA ISR: %d / %d", radar_data->speed, radar_data->energy);

	osMessageQueuePut(radar_data_queue, (void*)radar_data, 0U, 0U);

	return;
}

static void speed_calculation_thread()
{
    	while(true)
    	{
		radar_data_t radar_data;

		if(osMessageQueueGet(radar_data_queue, &radar_data, NULL, osWaitForever) == osOK) 
		{
			if(osMutexAcquire(tracking_system_mutex, osWaitForever) == osOK)
			{
				tracking_session_add_radar_data(&tracking_session, &radar_data);

				bool is_speed_detected;
				tracking_session_is_speed_detected(&tracking_session, &is_speed_detected);

				if(is_speed_detected == true)
				{
					tracking_report_t tracking_report;

					tracking_session_get_speed(&tracking_session, &tracking_report.speed);
					info1("MEASURED SPEED: %d", tracking_report.speed);

					tracking_partner_send_tracking_report(&tracking_partner, &tracking_report);
					info1("TRACKING REPORT IS SENT");

					fake_sensors_turn_off_radar();
					info1("TURN OFF RADAR");
				}

				osMutexRelease(tracking_system_mutex);
			}
		}
    	}
	return;
}


static void on_button_pushed_ISR()
{
	osThreadFlagsSet(main_thread_id, PUSH_BUTTON_MASK);

	return;
}

static void main_thread()
{
    	while(true)
    	{
		osThreadFlagsWait(PUSH_BUTTON_MASK, osFlagsWaitAny, osWaitForever);

		info1("MAIN THREAD");

		// just debug
		{
			int32_t average_speed;

			if(tracking_session_get_speed(&tracking_session, &average_speed) == SUCCESS)
			{
				for(uint8_t i = 0U; i < tracking_session.speed_calculator.speed_size; ++i)
				{
					info1("SPEED ARRAY[%d] = %d", i, tracking_session.speed_calculator.speed[i]);
				}
				info1("AVERAGE SPEED = %d", average_speed);
			}
		}

		osDelay(3000);
    	}
	return;
}


int logger_fwrite_boot (const char *ptr, int len)
{
    	fwrite(ptr, len, 1, stdout);
    	fflush(stdout);
    	return len;
}


int main()
{
    	// init platform
	PLATFORM_Init();

    	PLATFORM_LedsInit();
	PLATFORM_PushButtonInit(on_button_pushed_ISR);
	PLATFORM_RadioInit(); // Radio GPIO/PRS - LNA on some MGM12P

    	// Configure debug output
    	RETARGET_SerialInit();
    	log_init(BASE_LOG_LEVEL, &logger_fwrite_boot, NULL);

	info1("GermanTestProject "VERSION_STR" (%d.%d.%d)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	// Initialize OS kernel
	osKernelInitialize();

	// Create main thread
	const osThreadAttr_t main_thread_attr = { .name = "main" };
	main_thread_id = osThreadNew(main_thread, NULL, &main_thread_attr);

	tracking_session_init(&tracking_session);

	// Create tracking state thread
	{
		const osThreadAttr_t tracking_state_thread_attr = { .name = "tracking_state" };
		tracking_state_thread_id = osThreadNew(tracking_state_thread, NULL, &tracking_state_thread_attr);
	}

	// Create speed calculation thread
	{
		radar_data_queue = osMessageQueueNew(16U, sizeof(radar_data_t), NULL);

		const osThreadAttr_t speed_calc_thread_attr = { .name = "speed_calculation" };
		speed_calculation_thread_id = osThreadNew(speed_calculation_thread, NULL, &speed_calc_thread_attr);
	}
	tracking_system_mutex = osMutexNew(NULL);


	tracking_partner_init(&tracking_partner, on_partner_tracking_requested_ISR, on_partner_report_received_ISR);

	fake_sensors_init(on_sensor_movement_detetcted_ISR, on_radar_data_received_ISR);


    	if (osKernelReady == osKernelGetState())
    	{
        	// Switch to a thread-safe logger
        	logger_fwrite_init();
        	log_init(BASE_LOG_LEVEL, &logger_fwrite, NULL);

        	// Start the kernel
        	osKernelStart();
	}
    	else
    	{
        	err1("!osKernelReady");
    	}

    	while(true);

	return 0;
}

