#include <stdio.h>
#include <inttypes.h>

#include "retargetserial.h"

#include "cmsis_os2.h"

#include "platform.h"

#include "dmadrv.h"

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

#include "tracking_unit.h"
#include "result.h"



// CONFIGURABLE VALUES
/************************************************************************************************/
// tracking
#define DISTANCE_BETWEEN_TRACKING_UNITS_M 	100.0 // 100m
#define PASS_DETECTION_THRESHOLD 		0.280
#define TRACKING_PARTNER_RADIO_ADDRESS 		DESTINATION_GATEWAY_ADDRESS
#define TRACKING_RADIO_PORT 			0xE1

// radio
#define PAN_ID 0x22
/************************************************************************************************/


static osThreadId_t main_thread_id;


static comms_layer_t* radio_setup(am_addr_t radio_address);
static int logger_fwrite_boot(const char *ptr, int len);
#include "tracking_sensors.h"

static void main_thread()
{
	comms_layer_t *radio = NULL;

	// init radio
	{
		am_addr_t radio_address = DEFAULT_AM_ADDR;

	    	if(sigInit() == SIG_GOOD)
		{
			radio_address = sigGetNodeId();
		}
		warn1("RADIO ADDR:%" PRIX16 " PARTNER ADDR:%" PRIX16, radio_address, TRACKING_PARTNER_RADIO_ADDRESS);

		// init embedded radio-antenna
		radio = radio_setup(radio_address);

		if(radio == NULL)
		{
        		err1("radio init error");

        		while(true); // panic
    		}
	}



	//init tracking unit
	/*{
		tracking_config_t tracking_config;

		tracking_config.distance_between_units = DISTANCE_BETWEEN_TRACKING_UNITS_M;
		tracking_config.pass_detection_threshold = PASS_DETECTION_THRESHOLD;
		tracking_config.radio_module = radio;
		tracking_config.partner_radio_address = TRACKING_PARTNER_RADIO_ADDRESS;
		tracking_config.radio_port = TRACKING_RADIO_PORT;

		const int result = tracking_unit_init(&tracking_config);

		if(result != SUCCESS)
		{
        		err1("tracking unit init error");

        		while(true); // panic
    		}
	}*/

tracking_sensors_init(NULL, NULL);

	// main loop
    	while(true)
    	{
		info1("MAIN THREAD");

		osDelay(10000);
    	}
	return;
}

int main()
{
    	// init platform
	PLATFORM_Init();

    	PLATFORM_LedsInit();
	PLATFORM_RadioInit(); // Radio GPIO/PRS - LNA on some MGM12P

    	// Configure debug output
    	RETARGET_SerialInit();
    	log_init(BASE_LOG_LEVEL, &logger_fwrite_boot, NULL);

	info1("GermanTestProject "VERSION_STR" (%d.%d.%d)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	// Initialize DMA subsystem in general
	DMADRV_Init();

	// Initialize OS kernel
	osKernelInitialize();

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

