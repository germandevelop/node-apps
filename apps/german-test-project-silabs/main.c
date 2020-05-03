#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "retargetserial.h"

#include "cmsis_os2.h"

#include "platform.h"

#include "SignatureArea.h"
#include "DeviceSignature.h"
#include "radio.h"

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
#include "motion_data.h"
#include "tracking_summary.h"
#include "result.h"


#define PAN_ID 0x22
#define AM_ID 0xE1

#define UNIT_MOTION_DETECTED_FLAG_MASK 		0B00000001
#define UNIT_SESSION_COMPUTED_FLAG_MASK 	0B00000010
#define UNIT_FAILURE_OCCURED_FLAG_MASK 		0B00000100
#define UNIT_RADIO_FAILED_FLAG_MASK 		0B00001000
#define PARTNER_START_REQUESTED_FLAG_MASK 	0B00010000
#define PARTNER_STOP_REQUESTED_FLAG_MASK 	0B00100000
#define PARTNER_SUMMARY_RECEIVED_FLAG_MASK 	0B01000000
#define PARTNER_FAILURE_OCCURED_FLAG_MASK 	0B10000000


#define PUSH_BUTTON_MASK 0B00000001


static osThreadId_t main_thread_id;

static osThreadId_t tracking_control_thread_id;
static osThreadId_t speed_detection_thread_id;
static osMutexId_t tracking_unit_mutex;
static osSemaphoreId_t tracking_partner_semaphore;
static osMessageQueueId_t motion_data_queue;

static int32_t tracking_session_id;
static int32_t partner_session_id;
static double partner_speed;
static tracking_session_t tracking_session;
static tracking_partner_t tracking_partner;


static comms_layer_t* radio_setup(am_addr_t radio_address);
static int logger_fwrite_boot(const char *ptr, int len);


static void on_sensor_movement_detetcted_ISR()
{
	info1("MOTION SENSOR ISR");

	osThreadFlagsSet(tracking_control_thread_id, UNIT_MOTION_DETECTED_FLAG_MASK);	

	return;
}

static void on_partner_tracking_requested_ISR(int32_t session_id, bool is_session_requested)
{
	if(is_session_requested == true)
	{
		info1("START TRACKING REQUEST ISR: session_id = %d", session_id);

		if(osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever) == osOK)
		{
			if(tracking_session_id != session_id)
			{
				partner_session_id = session_id;

				osThreadFlagsSet(tracking_control_thread_id, PARTNER_START_REQUESTED_FLAG_MASK);
			}
			else
			{
				osThreadFlagsSet(tracking_control_thread_id, UNIT_FAILURE_OCCURED_FLAG_MASK);
			}
			osSemaphoreRelease(tracking_partner_semaphore);
		}
	}
	else
	{
		info1("STOP TRACKING REQUEST ISR: session_id = %d", session_id);

		osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever);

		const int32_t current_session_id = tracking_session_id;

		osSemaphoreRelease(tracking_partner_semaphore);

		if(current_session_id == session_id)
		{
			osThreadFlagsSet(tracking_control_thread_id, PARTNER_STOP_REQUESTED_FLAG_MASK);
		}
		else
		{
			osThreadFlagsSet(tracking_control_thread_id, UNIT_FAILURE_OCCURED_FLAG_MASK);
		}
	}
	return;
}

static void on_partner_summary_received_ISR(tracking_summary_t const * const tracking_summary)
{
	info1("SUMMARY RECEIVED ISR: session_id = %d | speed = %.3lf", tracking_summary->session_id, tracking_summary->speed);

	osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever);

	if(tracking_session_id == tracking_summary->session_id)
	{
		partner_speed = tracking_summary->speed;

		osThreadFlagsSet(tracking_control_thread_id, PARTNER_SUMMARY_RECEIVED_FLAG_MASK);
	}
	else
	{
		osThreadFlagsSet(tracking_control_thread_id, UNIT_FAILURE_OCCURED_FLAG_MASK);
	}

	osSemaphoreRelease(tracking_partner_semaphore);

	return;
}

static void on_partner_failure_occured_ISR(tracking_partner_error_t error_code)
{
	switch(error_code)
	{
		case COMMUNICATION_ERROR :
		{
			warn1("PARTNER COMMUNICATION FAILURE ISR");

			osThreadFlagsSet(tracking_control_thread_id, UNIT_RADIO_FAILED_FLAG_MASK);

			break;
		}
		case TRACKING_ERROR :
		{
			warn1("PARTNER TRACKING FAILURE ISR");

			osThreadFlagsSet(tracking_control_thread_id, PARTNER_FAILURE_OCCURED_FLAG_MASK);

			break;
		}
	}
	return;
}

static void tracking_control_thread()
{
	const uint32_t waiting_flags = UNIT_MOTION_DETECTED_FLAG_MASK | UNIT_SESSION_COMPUTED_FLAG_MASK | UNIT_FAILURE_OCCURED_FLAG_MASK | UNIT_RADIO_FAILED_FLAG_MASK |
					PARTNER_START_REQUESTED_FLAG_MASK | PARTNER_STOP_REQUESTED_FLAG_MASK | PARTNER_SUMMARY_RECEIVED_FLAG_MASK | PARTNER_FAILURE_OCCURED_FLAG_MASK;

    	while(true)
    	{
		const uint32_t flags = osThreadFlagsWait(waiting_flags, osFlagsWaitAny, osWaitForever);

		bool is_tracking_failed = false;
		bool is_failure_notification_needed = false;

		const uint32_t current_time = osKernelGetTickCount();

		osMutexAcquire(tracking_unit_mutex, osWaitForever);
		osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever);
		{
			/*********************************************************************************/
			// motion is detected by infrared sensor
			/*********************************************************************************/
			if((flags & UNIT_MOTION_DETECTED_FLAG_MASK) > 0U)
			{
				bool is_session_active;
				tracking_session_is_active(&tracking_session, &is_session_active);

				// start tracking
				if(is_session_active == false)
				{
					tracking_session_id = (int32_t)current_time;
					partner_speed = 0.0;

					osMessageQueueReset(motion_data_queue);

					tracking_session_start(&tracking_session, OBJECT_MOVING_AWAY, current_time);
					info1("TRACKING SESSION IS STARTED: session_id = %d", tracking_session_id);

					fake_sensors_turn_on_radar();
					info1("RADAR IS TURNED ON");

					if(tracking_partner_start_session(&tracking_partner, tracking_session_id) == SUCCESS)
					{
						info1("START TRACKING REQUEST IS SENT TO PARTNER");
					}
					else
					{
						is_tracking_failed = true;
						info1("PARTNER COMMUNICATION FAILURE");
					}
				}
				else
				{
					tracking_session_mode_t tracking_session_mode;
					tracking_session_get_mode(&tracking_session, &tracking_session_mode);

					// stop tracking
					if(tracking_session_mode == OBJECT_MOVING_TOWARD)
					{
						bool is_speed_completed;
						tracking_session_is_speed_completed(&tracking_session, &is_speed_completed);

						tracking_session_stop(&tracking_session);
						info1("TRACKING SESSION IS STOPPED");

						fake_sensors_turn_off_radar();
						info1("RADAR IS TURNED OFF");

						if(is_speed_completed == false)
						{
							double measured_speed;

							if(tracking_session_get_speed(&tracking_session, &measured_speed) == SUCCESS)
							{
								warn1("REPORT session_id = %d | measured_speed = %.3lf | partner_speed = %.3lf",
									tracking_session_id, measured_speed, partner_speed);

								tracking_summary_t tracking_summary;
								tracking_summary.speed = measured_speed;
								tracking_summary.session_id = tracking_session_id;

								if(tracking_partner_send_summary(&tracking_partner, &tracking_summary) == SUCCESS)
								{
									info1("TRACKING SUMMARY IS SENT TO PARTNER");
								}
								else
								{
									is_tracking_failed = true;
									info1("PARTNER COMMUNICATION FAILURE");
								}
							}
							else
							{
								is_tracking_failed = true;
								is_failure_notification_needed = true;
							}
						}

						if(tracking_partner_stop_session(&tracking_partner, tracking_session_id) == SUCCESS)
						{
							info1("STOP TRACKING REQUEST IS SENT TO PARTNER");
						}
						else
						{
							is_tracking_failed = true;
							info1("PARTNER COMMUNICATION FAILURE");
						}
						tracking_session_id = 0;
						partner_speed = 0.0;
					}
					// tracking error occured
					else
					{
						is_tracking_failed = true;
						is_failure_notification_needed = true;
					}
				}
			}

			/*********************************************************************************/
			// tracking is finished -> send summary to partner
			/*********************************************************************************/
			if((flags & UNIT_SESSION_COMPUTED_FLAG_MASK) > 0U)
			{
				fake_sensors_turn_off_radar();
				info1("RADAR IS TURNED OFF");

				double measured_speed;

				if(tracking_session_get_speed(&tracking_session, &measured_speed) == SUCCESS)
				{
					tracking_summary_t tracking_summary;
					tracking_summary.speed = measured_speed;
					tracking_summary.session_id = tracking_session_id;

					if(tracking_partner_send_summary(&tracking_partner, &tracking_summary) == SUCCESS)
					{
						info1("TRACKING SUMMARY IS SENT TO PARTNER");
					}
					else
					{
						is_tracking_failed = true;
						info1("PARTNER COMMUNICATION FAILURE");
					}
				}
				else
				{
					is_tracking_failed = true;
					is_failure_notification_needed = true;
				}
			}

			/*********************************************************************************/
			// tracking failure occured -> stop tracking session and inform partner
			/*********************************************************************************/
			if((flags & UNIT_FAILURE_OCCURED_FLAG_MASK) > 0U)
			{
				is_tracking_failed = true;
				is_failure_notification_needed = true;
			}

			/*********************************************************************************/
			// radio communication error occured -> just stop tracking session
			/*********************************************************************************/
			if((flags & UNIT_RADIO_FAILED_FLAG_MASK) > 0U)
			{
				is_tracking_failed = true;
			}

			/*********************************************************************************/
			// start tracking request received from partner
			/*********************************************************************************/
			if((flags & PARTNER_START_REQUESTED_FLAG_MASK) > 0U)
			{
				bool is_session_active;
				tracking_session_is_active(&tracking_session, &is_session_active);

				// start tracking
				if(is_session_active == false)
				{
					tracking_session_id = partner_session_id;
					partner_speed = 0.0;

					tracking_session_start(&tracking_session, OBJECT_MOVING_TOWARD, current_time);
					info1("TRACKING SESSION IS STARTED: session_id = %d", tracking_session_id);

					osMessageQueueReset(motion_data_queue);////////////////////////////////////
					fake_sensors_turn_on_radar();
					info1("RADAR IS TURNED ON");
				}
				// tracking error occured
				else
				{
					is_tracking_failed = true;
					is_failure_notification_needed = true;
				}
			}

			/*********************************************************************************/
			// stop tracking request received from partner
			/*********************************************************************************/
			if((flags & PARTNER_STOP_REQUESTED_FLAG_MASK) > 0U)
			{
				bool is_session_active;
				tracking_session_is_active(&tracking_session, &is_session_active);

				tracking_session_mode_t tracking_session_mode;
				tracking_session_get_mode(&tracking_session, &tracking_session_mode);

				bool is_speed_completed;
				tracking_session_is_speed_completed(&tracking_session, &is_speed_completed);

				// stop tracking
				if((is_session_active == true) && (tracking_session_mode == OBJECT_MOVING_AWAY) && (is_speed_completed == true))
				{
					double measured_speed;
					tracking_session_get_speed(&tracking_session, &measured_speed);
					warn1("REPORT session_id = %d | measured_speed = %.3lf | partner_speed = %.3lf",
						tracking_session_id, measured_speed, partner_speed);

					tracking_session_id = 0;
					partner_speed = 0.0;

					tracking_session_stop(&tracking_session);
					info1("TRACKING SESSION IS STOPPED");

					fake_sensors_turn_off_radar();
					info1("RADAR IS TURNED OFF");
				}
				// tracking error occured
				else
				{
					is_tracking_failed = true;
					is_failure_notification_needed = true;
				}
			}

			/*********************************************************************************/
			// stop tracking request received
			/*********************************************************************************/
			if((flags & PARTNER_SUMMARY_RECEIVED_FLAG_MASK) > 0U)
			{
				info1("PARTNER SUMMARY RECEIVED");
			}

			/*********************************************************************************/
			// partner sent a failure message -> just stop tracking session
			/*********************************************************************************/
			if((flags & PARTNER_FAILURE_OCCURED_FLAG_MASK) > 0U)
			{
				is_tracking_failed = true;
			}

			/*********************************************************************************/
			// tracking error occured
			/*********************************************************************************/
			if(is_tracking_failed == true)
			{
				info1("TRACKING FAILURE OCCURED");

				tracking_session_id = 0;
				partner_speed = 0.0;

				tracking_session_stop(&tracking_session);
				info1("TRACKING SESSION IS STOPPED");

				fake_sensors_turn_off_radar();
				info1("RADAR IS TURNED OFF");

				if(is_failure_notification_needed == true)
				{
					if(tracking_partner_send_failure(&tracking_partner, tracking_session_id) == SUCCESS)
					{
						info1("TRACKING FAILURE IS SENT TO PARTNER");
					}
					else
					{
						info1("PARTNER COMMUNICATION FAILURE");
					}
				}
			}
		}
		osSemaphoreRelease(tracking_partner_semaphore);
		osMutexRelease(tracking_unit_mutex);

		// need to wait a little bit after failure
		if(is_tracking_failed == true)
		{
			osDelay(MAX_TIME_IN_MOVING_AWAY_MODE_MIL_SEC);

			osThreadFlagsClear(flags);	
		}
    	}
	return;
}


static void on_motion_data_received_ISR(motion_data_t const * const motion_data)
{
	info1("RADAR DATA ISR: %.3lf / %.3lf", motion_data->speed, motion_data->energy);

	osMessageQueuePut(motion_data_queue, (void*)motion_data, 0U, 0U);

	return;
}

static void speed_detection_thread()
{
    	while(true)
    	{
		motion_data_t motion_data;

		if(osMessageQueueGet(motion_data_queue, &motion_data, NULL, osWaitForever) == osOK) 
		{
			if(osMutexAcquire(tracking_unit_mutex, osWaitForever) == osOK)
			{
				const uint32_t current_time = osKernelGetTickCount();

				tracking_session_add_motion_data(&tracking_session, &motion_data, current_time);

				bool is_speed_completed;
				tracking_session_is_speed_completed(&tracking_session, &is_speed_completed);

				osMutexRelease(tracking_unit_mutex);

				if(is_speed_completed == true)
				{
					osThreadFlagsSet(tracking_control_thread_id, UNIT_SESSION_COMPUTED_FLAG_MASK);
				}
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
	// init tracking
	{
		// define radio addresses
		const am_addr_t partner_address = DESTINATION_GATEWAY_ADDRESS;
		const am_id_t partner_port = AM_ID;

		am_addr_t radio_address = DEFAULT_AM_ADDR;

	    	if(sigInit() == SIG_GOOD)
		{
			radio_address = sigGetNodeId();
		}
		warn1("RADIO ADDR:%" PRIX16 " PARTNER ADDR:%" PRIX16, radio_address, partner_address);

		// init embedded radio-antenna
		comms_layer_t *radio = radio_setup(radio_address);

		if(radio == NULL)
		{
        		err1("radio init error");

        		while(true); // panic
    		}

		// init tracking session
		tracking_session_init(&tracking_session);

		// init tracking partner
		tracking_partner_init(&tracking_partner, on_partner_tracking_requested_ISR, on_partner_summary_received_ISR, on_partner_failure_occured_ISR);
		int result = tracking_partner_setup_radio_network(&tracking_partner, radio, partner_address, partner_port);

		if(result != SUCCESS)
		{
        		err1("tracking partner init error");

        		while(true); // panic
    		}

		// init tracking sensors
		result = fake_sensors_init(on_sensor_movement_detetcted_ISR, on_motion_data_received_ISR);

		if(result != SUCCESS)
		{
        		err1("sensors init error");

        		while(true); // panic
    		}
	}

	// main loop
    	while(true)
    	{
		osThreadFlagsWait(PUSH_BUTTON_MASK, osFlagsWaitAny, osWaitForever);

		info1("MAIN THREAD");

		// just debug
		{
			double average_speed;

			if(tracking_session_get_speed(&tracking_session, &average_speed) == SUCCESS)
			{
				for(uint8_t i = 0U; i < tracking_session.speed_calculator.speed_size; ++i)
				{
					info1("SPEED ARRAY[%d] = %.3lf", i, tracking_session.speed_calculator.speed[i]);
				}
				info1("AVERAGE SPEED = %.3lf", average_speed);
			}
		}

		osDelay(3000);
    	}
	return;
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

	// Create tracking state thread
	{
		tracking_unit_mutex = osMutexNew(NULL);
		tracking_partner_semaphore = osSemaphoreNew(1U, 1U, NULL);

		const osThreadAttr_t tracking_control_thread_attr = { .name = "tracking_control" };
		tracking_control_thread_id = osThreadNew(tracking_control_thread, NULL, &tracking_control_thread_attr);
	}

	// Create speed calculation thread
	{
		motion_data_queue = osMessageQueueNew(32U, sizeof(motion_data_t), NULL);

		const osThreadAttr_t speed_calc_thread_attr = { .name = "speed_detection" };
		speed_detection_thread_id = osThreadNew(speed_detection_thread, NULL, &speed_calc_thread_attr);
	}

	// Create main thread
	{
		const osThreadAttr_t main_thread_attr = { .name = "main" };
		main_thread_id = osThreadNew(main_thread, NULL, &main_thread_attr);
	}

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



static void radio_start_done(comms_layer_t * comms, comms_status_t status, void *user)
{
	debug("radio started %d", status);

	return;
}

comms_layer_t* radio_setup(am_addr_t radio_address)
{
	comms_layer_t *radio = radio_init(DEFAULT_RADIO_CHANNEL, PAN_ID, radio_address);

	if(radio == NULL)
	{
		return NULL;
	}

	if(comms_start(radio, radio_start_done, NULL) != COMMS_SUCCESS)
	{
		return NULL;
	}

	// Wait for radio to start, could use osTreadFlagWait and set from callback
	while(comms_status(radio) != COMMS_STARTED)
	{
		osDelay(1);
	}
    	debug1("radio rdy");

	return radio;
}

int logger_fwrite_boot(const char *ptr, int len)
{
    	fwrite(ptr, len, 1, stdout);
    	fflush(stdout);

    	return len;
}

