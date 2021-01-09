#include "tracking_sensors.h"

#include "motion_data.h"
#include "result.h"


#ifdef USE_SENSORS_SIMULATOR

#include "fake_sensors.h"

int tracking_sensors_init(movement_detect_t detect_movement_callback,
			motion_data_receive_t receive_motion_data_callback)
{
	return fake_sensors_init(detect_movement_callback, receive_motion_data_callback);
}

void tracking_sensors_turn_on_radar()
{
	fake_sensors_turn_on_radar();

	return;
}

void tracking_sensors_turn_off_radar()
{
	fake_sensors_turn_off_radar();

	return;
}

#else

#include <stddef.h>

#include "microwave_sensor.h"

static void pir_sensor_init(movement_detect_t detect_movement_callback);

int tracking_sensors_init(movement_detect_t detect_movement_callback,
			motion_data_receive_t receive_motion_data_callback)
{
	pir_sensor_init(detect_movement_callback);

	microwave_sensor_init(receive_motion_data_callback);

	return SUCCESS;
}

void tracking_sensors_turn_on_radar()
{
	microwave_sensor_turn_on();

	return;
}

void tracking_sensors_turn_off_radar()
{
	microwave_sensor_turn_off();

	return;
}


#include "em_cmu.h"
#include "em_gpio.h"
#include "gpiointerrupt.h"

#include "cmsis_os2.h"

#include "platform_io.h"

#define PIR_EXT_INT_NO	0

static movement_detect_t pir_sensor_isr_user_callback;

static void pir_sensor_ISR(uint8_t pin);

void pir_sensor_init(movement_detect_t detect_movement_callback)
{
	pir_sensor_isr_user_callback = detect_movement_callback;

	CMU_ClockEnable(cmuClock_GPIO, true);

	// 3V sensor power must be enabled
	// Enable 3V3_SW, by pulling PB12 pin to ground. This enables 3.3V voltage to the sensors
	GPIO_PinModeSet(POWER_CONTROL_3V3_ENABLE_PORT, POWER_CONTROL_3V3_ENABLE_PIN, gpioModePushPull, 0);
		
	GPIO_PinModeSet(PIR_INT_PORT, PIR_INT_PIN, gpioModeInputPullFilter, 1);

	GPIOINT_Init();
	GPIOINT_CallbackRegister(PIR_EXT_INT_NO, pir_sensor_ISR);
		
	GPIO_ExtIntConfig(PIR_INT_PORT, PIR_INT_PIN, PIR_EXT_INT_NO, true, false, true);
	
	// need change interruption priority to use RTOS functions
	NVIC_SetPriority(GPIO_EVEN_IRQn, PIR_INTERRUPT_PRIORITY);
	NVIC_SetPriority(GPIO_ODD_IRQn, PIR_INTERRUPT_PRIORITY);
	
	return;
}

#define PIR_SENSOR_SMOOTH_TIME_S 2U // ~ 2 sec

void pir_sensor_ISR(uint8_t pin)
{
	static uint32_t last_interruption_time = 0U;

	if (pir_sensor_isr_user_callback != NULL)
	{
		const uint32_t current_time = osKernelGetTickCount();

		uint32_t interruption_interval = current_time - last_interruption_time;

		if (interruption_interval > (PIR_SENSOR_SMOOTH_TIME_S * 1000U))
		{
			pir_sensor_isr_user_callback();

			last_interruption_time = current_time;
		}
	}
	return;
}

#endif

