#include "tracking_unit.h"

#include "cmsis_os2.h"

#include "loglevels.h"
#define __MODUUL__ "tracking_unit"
#define __LOG_LEVEL__ (LOG_LEVEL_tracking_unit & BASE_LOG_LEVEL)
#include "log.h"

#include "tracking_partner.h"
#include "tracking_sensors.h"
#include "tracking_session.h"
#include "object_tracking.h"
#include "tracking_summary.h"
#include "motion_data.h"
#include "result.h"


#define UNIT_MOTION_DETECTED_AT_START 	0B0000000000000001
#define UNIT_MOTION_DETECTED_AT_END 	0B0000000000000010
#define UNIT_TRACKING_FINISHED 		0B0000000000000100
#define UNIT_FAILURE_OCCURED 		0B0000000000001000
#define UNIT_COMMUNICATION_FAILED 	0B0000000000010000
#define PARTNER_START_REQUESTED 	0B0000000000100000
#define PARTNER_STOP_REQUESTED		0B0000000001000000
#define PARTNER_SUMMARY_RECEIVED 	0B0000000010000000
#define PARTNER_FAILURE_OCCURED 	0B0000000100000000


static osThreadId_t tracking_control_thread_id;
static osThreadId_t speed_detection_thread_id;
static osMutexId_t tracking_unit_mutex;
static osSemaphoreId_t tracking_partner_semaphore;
static osMessageQueueId_t motion_data_queue;


static tracking_partner_t tracking_partner;
static tracking_session_t tracking_session;
static object_tracking_t object_tracking;

static int32_t requested_session_id;

static double tracking_partner_distance; // m
static uint32_t max_session_time_in_moving_away_mode; // ms
static uint32_t max_session_time_in_moving_toward_mode; // ms


static void on_sensor_movement_detetcted_ISR(bool is_movement);
static void on_partner_tracking_requested_ISR(int32_t partner_session_id, bool is_session_requested);
static void on_partner_summary_received_ISR(tracking_summary_t const * const partner_summary);
static void on_partner_failure_occured_ISR(tracking_partner_error_t error_code);
static void tracking_control_thread();

static void on_motion_data_received_ISR(motion_data_t const * const motion_data);
static void speed_detection_thread();

static uint32_t calculate_max_session_length(double speed);


int tracking_unit_init(tracking_config_t const * const tracking_config)
{
	// init object tracking
	tracking_partner_distance = tracking_config->distance_between_units;

	max_session_time_in_moving_away_mode = (uint32_t)(((tracking_partner_distance / 2.0) / (MIN_VEHICLE_SPEED_KMH / 3.60)) * 1000.0);
	max_session_time_in_moving_toward_mode = (uint32_t)((tracking_partner_distance / (MIN_VEHICLE_SPEED_KMH / 3.60)) * 1000.0);

	tracking_params_t tracking_params = { .pass_detection_threshold = tracking_config->pass_detection_threshold };

	object_tracking_init(&object_tracking, &tracking_params);

	// init tracking session
	tracking_session_init(&tracking_session);

	// init tracking partner
	tracking_partner_init(&tracking_partner,
				on_partner_tracking_requested_ISR,
				on_partner_summary_received_ISR,
				on_partner_failure_occured_ISR);

	int result = tracking_partner_setup_radio_network(&tracking_partner,
								tracking_config->radio_module,
								tracking_config->partner_radio_address,
								tracking_config->radio_port);

	if(result != SUCCESS)
	{
        	err1("tracking partner init error");

        	return FAILURE;
    	}

	// init tracking sensors
	result = tracking_sensors_init(on_sensor_movement_detetcted_ISR, on_motion_data_received_ISR);

	if(result != SUCCESS)
	{
        	err1("tracking sensors init error");

        	return FAILURE;
    	}

	// create tracking control thread
	tracking_unit_mutex = osMutexNew(NULL);
	tracking_partner_semaphore = osSemaphoreNew(1U, 1U, NULL);

	const osThreadAttr_t tracking_control_thread_attr = { .name = "tracking_control" };
	tracking_control_thread_id = osThreadNew(tracking_control_thread, NULL, &tracking_control_thread_attr);

	// create speed detection thread
	motion_data_queue = osMessageQueueNew(32U, sizeof(motion_data_t), NULL);

	const osThreadAttr_t speed_calc_thread_attr = { .name = "speed_detection" };
	speed_detection_thread_id = osThreadNew(speed_detection_thread, NULL, &speed_calc_thread_attr);

	return SUCCESS;
}


void on_sensor_movement_detetcted_ISR(bool is_movement)
{
	static bool is_movement_previous_frame = false;

	if((is_movement_previous_frame == false) && (is_movement == true))
	{
		warn1("MOTION SENSOR BEGIN ISR");

		is_movement_previous_frame = true;

		osThreadFlagsSet(tracking_control_thread_id, UNIT_MOTION_DETECTED_AT_START);
	}
	else if((is_movement_previous_frame == true) && (is_movement == false))
	{
		warn1("MOTION SENSOR END ISR");

		is_movement_previous_frame = false;

		osThreadFlagsSet(tracking_control_thread_id, UNIT_MOTION_DETECTED_AT_END);
	}
	return;
}

void on_partner_tracking_requested_ISR(int32_t partner_session_id, bool is_session_requested)
{
	if(is_session_requested == true)
	{
		warn1("START TRACKING REQUEST ISR");
		warn1("START TRACKING SESSION ID ISR: %d", partner_session_id);

		osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever);

		int32_t current_session_id;
		tracking_session_get_id(&tracking_session, &current_session_id);

		if(current_session_id != partner_session_id)
		{
			requested_session_id = partner_session_id;

			osThreadFlagsSet(tracking_control_thread_id, PARTNER_START_REQUESTED);
		}
		else
		{
			osThreadFlagsSet(tracking_control_thread_id, UNIT_FAILURE_OCCURED);
		}

		osSemaphoreRelease(tracking_partner_semaphore);
	}
	else
	{
		warn1("STOP TRACKING REQUEST ISR ID");
		warn1("STOP TRACKING SESSION ID ISR: %d", partner_session_id);

		osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever);

		int32_t current_session_id;
		tracking_session_get_id(&tracking_session, &current_session_id);

		osSemaphoreRelease(tracking_partner_semaphore);

		if(current_session_id == partner_session_id)
		{
			osThreadFlagsSet(tracking_control_thread_id, PARTNER_STOP_REQUESTED);
		}
		else
		{
			osThreadFlagsSet(tracking_control_thread_id, UNIT_FAILURE_OCCURED);
		}
	}
	return;
}

void on_partner_summary_received_ISR(tracking_summary_t const * const partner_summary)
{
	warn1("SUMMARY RECEIVED ISR");
	warn1("SUMMARY SESSION ID ISR: %d", partner_summary->session_id);
	warn1("SUMMARY SPEED ISR: %.3lf", partner_summary->speed);

	osSemaphoreAcquire(tracking_partner_semaphore, osWaitForever);

	int32_t current_session_id;
	tracking_session_get_id(&tracking_session, &current_session_id);

	if(current_session_id == partner_summary->session_id)
	{
		tracking_session_set_partner_speed(&tracking_session, partner_summary->speed);

		osThreadFlagsSet(tracking_control_thread_id, PARTNER_SUMMARY_RECEIVED);
	}
	else
	{
		osThreadFlagsSet(tracking_control_thread_id, UNIT_FAILURE_OCCURED);
	}

	osSemaphoreRelease(tracking_partner_semaphore);

	return;
}

void on_partner_failure_occured_ISR(tracking_partner_error_t error_code)
{
	switch(error_code)
	{
		case COMMUNICATION_ERROR :
		{
			err1("PARTNER COMMUNICATION FAILURE ISR");

			osThreadFlagsSet(tracking_control_thread_id, UNIT_COMMUNICATION_FAILED);

			break;
		}
		case TRACKING_ERROR :
		{
			err1("PARTNER TRACKING FAILURE ISR");

			osThreadFlagsSet(tracking_control_thread_id, PARTNER_FAILURE_OCCURED);

			break;
		}
	}
	return;
}

void tracking_control_thread()
{
	const uint32_t waiting_flags = UNIT_MOTION_DETECTED_AT_START | UNIT_MOTION_DETECTED_AT_END | UNIT_TRACKING_FINISHED | UNIT_FAILURE_OCCURED | UNIT_COMMUNICATION_FAILED |
					PARTNER_START_REQUESTED | PARTNER_STOP_REQUESTED | PARTNER_SUMMARY_RECEIVED | PARTNER_FAILURE_OCCURED;

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
			// motion begin is detected with infrared sensor
			/*********************************************************************************/
			if((flags & UNIT_MOTION_DETECTED_AT_START) > 0U)
			{
				bool is_tracking_session_active;
				tracking_session_is_active(&tracking_session, &is_tracking_session_active);

				tracking_mode_t object_tracking_mode;
				object_tracking_get_mode(&object_tracking, &object_tracking_mode);

				// object moves away -> start tracking
				if(is_tracking_session_active == false)
				{
					osMessageQueueReset(motion_data_queue);
					warn1("\n\n\n");

					const int32_t new_session_id = (int32_t)current_time;

					tracking_session_reset(&tracking_session);
					tracking_session_set_id(&tracking_session, new_session_id);
					warn1("TRACKING SESSION IS STARTED");
					warn1("TRACKING SESSION ID: %d", new_session_id);

					object_tracking_start(&object_tracking, OBJECT_MOVING_AWAY, current_time, max_session_time_in_moving_away_mode);
					warn1("OBJECT TRACKING MAX LENGTH: %d s", (uint32_t)(max_session_time_in_moving_away_mode / 1000.0));

					tracking_sensors_turn_on_radar();
					warn1("RADAR IS TURNED ON");

					if(tracking_partner_start_session(&tracking_partner, new_session_id) == SUCCESS)
					{
						warn1("START TRACKING REQUEST IS SENT TO PARTNER");
					}
					else
					{
						is_tracking_failed = true;
						warn1("PARTNER COMMUNICATION FAILURE");
					}
				}
				// object moves away -> tracking error occured
				else if((is_tracking_session_active == true) && (object_tracking_mode == OBJECT_MOVING_AWAY))
				{
					is_tracking_failed = true;
					is_failure_notification_needed = true;
				}
				// object moves toward -> stop tracking
				else if((is_tracking_session_active == true) && (object_tracking_mode == OBJECT_MOVING_TOWARD))
				{
					warn1("REPORT SESSION ID: %d", tracking_session.id);
					warn1("REPORT MEASURED SPEED: %.3lf", tracking_session.unit_speed);
					warn1("REPORT PARTNER SPEED: %.3lf", tracking_session.partner_speed);

					bool is_session_valid;
					tracking_session_is_valid(&tracking_session, &is_session_valid);

					if(is_session_valid == true)
					{
						int32_t current_session_id;
						tracking_session_get_id(&tracking_session, &current_session_id);

						tracking_sensors_turn_off_radar();
						warn1("RADAR IS TURNED OFF");

						tracking_session_reset(&tracking_session);
						warn1("TRACKING SESSION IS STOPPED");

						if(tracking_partner_stop_session(&tracking_partner, current_session_id) == SUCCESS)
						{
							warn1("STOP TRACKING REQUEST IS SENT TO PARTNER");
						}
						else
						{
							is_tracking_failed = true;
							warn1("PARTNER COMMUNICATION FAILURE");
						}
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
			// motion end is detected with infrared sensor
			/*********************************************************************************/
			if((flags & UNIT_MOTION_DETECTED_AT_END) > 0U)
			{

			}

			/*********************************************************************************/
			// tracking is finished -> send summary to partner
			/*********************************************************************************/
			if((flags & UNIT_TRACKING_FINISHED) > 0U)
			{
				warn1("OBJECT TRACKING IS FINISHED");

				tracking_mode_t object_tracking_mode;
				object_tracking_get_mode(&object_tracking, &object_tracking_mode);

				bool is_partner_speed_detected;
				tracking_session_is_partner_speed_detected(&tracking_session, &is_partner_speed_detected);

				if((object_tracking_mode == OBJECT_MOVING_AWAY) && (is_partner_speed_detected == true))
				{
					is_tracking_failed = true;
					is_failure_notification_needed = true;
				}
				else
				{
					double unit_measured_speed;

					if(object_tracking_get_speed(&object_tracking, &unit_measured_speed) == SUCCESS)
					{
						tracking_session_set_unit_speed(&tracking_session, unit_measured_speed);

						int32_t current_session_id;
						tracking_session_get_id(&tracking_session, &current_session_id);

						tracking_summary_t tracking_summary = { .session_id = current_session_id, .speed = unit_measured_speed};

						if(tracking_partner_send_summary(&tracking_partner, &tracking_summary) == SUCCESS)
						{
							warn1("TRACKING SUMMARY IS SENT TO PARTNER");
						}
						else
						{
							is_tracking_failed = true;
							warn1("PARTNER COMMUNICATION FAILURE");
						}
					}
					else if(is_partner_speed_detected == true)
					{
						warn1("REPORT SESSION ID: %d", tracking_session.id);
						warn1("REPORT MEASURED SPEED: %.3lf", tracking_session.unit_speed);
						warn1("REPORT PARTNER SPEED: %.3lf", tracking_session.partner_speed);

						bool is_session_valid;
						tracking_session_is_valid(&tracking_session, &is_session_valid);

						if(is_session_valid == true)
						{
							int32_t current_session_id;
							tracking_session_get_id(&tracking_session, &current_session_id);

							tracking_sensors_turn_off_radar();
							warn1("RADAR IS TURNED OFF");

							tracking_session_reset(&tracking_session);
							warn1("TRACKING SESSION IS STOPPED");

							if(tracking_partner_stop_session(&tracking_partner, current_session_id) == SUCCESS)
							{
								warn1("STOP TRACKING REQUEST IS SENT TO PARTNER");
							}
							else
							{
								is_tracking_failed = true;
								warn1("PARTNER COMMUNICATION FAILURE");
							}
						}
						else
						{
							is_tracking_failed = true;
							is_failure_notification_needed = true;
						}
					}
					else
					{
						is_tracking_failed = true;
						is_failure_notification_needed = true;
					}
				}
			}

			/*********************************************************************************/
			// tracking failure occured -> stop tracking session and inform partner
			/*********************************************************************************/
			if((flags & UNIT_FAILURE_OCCURED) > 0U)
			{
				is_tracking_failed = true;
				is_failure_notification_needed = true;
			}

			/*********************************************************************************/
			// radio communication error occured -> just stop tracking session
			/*********************************************************************************/
			if((flags & UNIT_COMMUNICATION_FAILED) > 0U)
			{
				is_tracking_failed = true;
			}

			/*********************************************************************************/
			// start tracking request received from partner
			/*********************************************************************************/
			if((flags & PARTNER_START_REQUESTED) > 0U)
			{
				bool is_tracking_session_active;
				tracking_session_is_active(&tracking_session, &is_tracking_session_active);

				// start tracking
				if(is_tracking_session_active == false)
				{
					osMessageQueueReset(motion_data_queue);
					warn1("\n\n\n");

					const int32_t new_session_id = requested_session_id;

					tracking_session_reset(&tracking_session);
					tracking_session_set_id(&tracking_session, new_session_id);
					warn1("TRACKING SESSION IS STARTED");
					warn1("TRACKING SESSION ID: %d", new_session_id);

					object_tracking_start(&object_tracking, OBJECT_MOVING_TOWARD, current_time, max_session_time_in_moving_toward_mode);
					warn1("OBJECT TRACKING MAX LENGTH: %d s", (uint32_t)(max_session_time_in_moving_toward_mode / 1000.0));

					tracking_sensors_turn_on_radar();
					warn1("RADAR IS TURNED ON");
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
			if((flags & PARTNER_STOP_REQUESTED) > 0U)
			{
				bool is_tracking_session_active;
				tracking_session_is_active(&tracking_session, &is_tracking_session_active);

				tracking_mode_t object_tracking_mode;
				object_tracking_get_mode(&object_tracking, &object_tracking_mode);

				// stop tracking
				if((is_tracking_session_active == true) && (object_tracking_mode == OBJECT_MOVING_AWAY))
				{
					warn1("REPORT SESSION ID: %d", tracking_session.id);
					warn1("REPORT MEASURED SPEED: %.3lf", tracking_session.unit_speed);
					warn1("REPORT PARTNER SPEED: %.3lf", tracking_session.partner_speed);

					bool is_session_valid;
					tracking_session_is_valid(&tracking_session, &is_session_valid);

					if(is_session_valid == true)
					{
						tracking_sensors_turn_off_radar();
						warn1("RADAR IS TURNED OFF");

						tracking_session_reset(&tracking_session);
						warn1("TRACKING SESSION IS STOPPED");
					}
					// tracking error occured
					else
					{
						is_tracking_failed = true;
						is_failure_notification_needed = true;
					}
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
			if((flags & PARTNER_SUMMARY_RECEIVED) > 0U)
			{
				warn1("PARTNER SUMMARY RECEIVED");
				warn1("TRACKING SESSION ID: %d", tracking_session.id);

				double partner_speed;
				tracking_session_get_partner_speed(&tracking_session, &partner_speed);

				const uint32_t new_session_length = calculate_max_session_length(partner_speed);

				object_tracking_set_max_length(&object_tracking, new_session_length);
				warn1("OBJECT TRACKING NEW MAX LENGTH: %d s", new_session_length / 1000U);
			}

			/*********************************************************************************/
			// partner sent a failure message -> just stop tracking session
			/*********************************************************************************/
			if((flags & PARTNER_FAILURE_OCCURED) > 0U)
			{
				is_tracking_failed = true;
			}

			/*********************************************************************************/
			// tracking error occured
			/*********************************************************************************/
			if(is_tracking_failed == true)
			{
				err1("TRACKING FAILURE OCCURED");

				tracking_sensors_turn_off_radar();
				err1("RADAR IS TURNED OFF");

				if(is_failure_notification_needed == true)
				{
					int32_t current_session_id;
					tracking_session_get_id(&tracking_session, &current_session_id);

					if(tracking_partner_send_failure(&tracking_partner, current_session_id) == SUCCESS)
					{
						err1("TRACKING FAILURE IS SENT TO PARTNER");
					}
					else
					{
						err1("PARTNER COMMUNICATION FAILURE");
					}
				}
				tracking_session_reset(&tracking_session);
				err1("TRACKING SESSION IS STOPPED");
			}
		}

		osSemaphoreRelease(tracking_partner_semaphore);
		osMutexRelease(tracking_unit_mutex);

		// need to wait a little bit after failure
		if(is_tracking_failed == true)
		{
			osDelay(max_session_time_in_moving_away_mode);

			osThreadFlagsClear(waiting_flags);	
		}
    	}
	return;
}


void on_motion_data_received_ISR(motion_data_t const * const motion_data)
{
	info1("RADAR DATA ISR: %.3lf / %.3lf", motion_data->speed, motion_data->energy);

	osMessageQueuePut(motion_data_queue, (void*)motion_data, 0U, 0U);

	return;
}

void speed_detection_thread()
{
    	while(true)
    	{
		motion_data_t motion_data;

		if(osMessageQueueGet(motion_data_queue, &motion_data, NULL, osWaitForever) == osOK) 
		{
			osMutexAcquire(tracking_unit_mutex, osWaitForever);

			bool is_speed_completed;
			object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

			if(is_speed_completed == false)
			{
				const uint32_t current_time = osKernelGetTickCount();

				object_tracking_add_motion_data(&object_tracking, &motion_data, current_time);

				object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

				if(is_speed_completed == true)
				{
					osThreadFlagsSet(tracking_control_thread_id, UNIT_TRACKING_FINISHED);
				}
			}
			osMutexRelease(tracking_unit_mutex);
		}
    	}
	return;
}


uint32_t calculate_max_session_length(double speed)
{
	const double speed_m_per_s = speed / 3.6;

	const double session_length_s = tracking_partner_distance / speed_m_per_s;

	return (uint32_t)(session_length_s * 1000.0) + 1U;
}

