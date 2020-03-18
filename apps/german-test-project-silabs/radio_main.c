/**
 * The GermanTestProject application for Silabs Wireless Gecko platforms.
 *
 * Configure platform, logging to UART, start kernel and start sending
 * / receiving count messages, displaying received count on LEDs.
 *
 * Copyright Thinnect Inc. 2019
 * @license MIT
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "retargetserial.h"

#include "cmsis_os2.h"

#include "platform.h"

#include "SignatureArea.h"
#include "DeviceSignature.h"

#include "loggers_ext.h"
#include "logger_fwrite.h"

#include "platform_push_button.h"
#include "radio_msg.h"

#include "DeviceSignature.h"
#include "mist_comm_am.h"
#include "radio.h"

#include "endianness.h"

#include "loglevels.h"
#define __MODUUL__ "main"
#define __LOG_LEVEL__ (LOG_LEVEL_main & BASE_LOG_LEVEL)
#include "log.h"

// Include the information header binary
#include "incbin.h"
INCBIN(Header, "header.bin");

static comms_msg_t m_msg;
static bool m_sending = false;
static osMutexId_t m_mutex;


#define FLAGS_MSK1 0x00000001U
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

typedef void(*command_t)(radio_msg_t *raw_message);

typedef struct leds_message
{   
	bool is_turn_on;

} leds_message_t;

static osThreadId_t app_thread_id;
static osMessageQueueId_t leds_queue;


static void do_nothing(radio_msg_t *raw_message)
{
	debug1("COMMAND : do_nothing()");

	return;
}

static void turn_on_leds(radio_msg_t *raw_message)
{
	debug1("COMMAND : turn_on_leds()");

	leds_message_t leds_message;
	leds_message.is_turn_on = true;

	osMessageQueuePut(leds_queue, &leds_message, 0U, 0U);

	return;
}

static void turn_off_leds(radio_msg_t *raw_message)
{
	debug1("COMMAND : turn_off_leds()");

	leds_message_t leds_message;
	leds_message.is_turn_on = false;

	osMessageQueuePut(leds_queue, &leds_message, 0U, 0U);

	return;
}

static const command_t commands[] = {

	[DO_NOTHING]	= do_nothing,
	[TURN_ON_LEDS]	= turn_on_leds,
	[TURN_OFF_LEDS]	= turn_off_leds
};

// Receive a message from the network
static void receive_message (comms_layer_t *comms, const comms_msg_t *msg, void *user)
{
    	if (comms_get_payload_length(comms, msg) >= sizeof(radio_msg_t))
    	{
        	radio_msg_t *packet = (radio_msg_t*)comms_get_payload(comms, msg, sizeof(radio_msg_t));

		const uint8_t cmd_id = packet->command_id;

        	debug("rcv COMMAND : %u", (unsigned int)cmd_id);

		if (cmd_id < ARRAY_SIZE(commands))
		{
			commands[cmd_id](packet);
		}
		else
		{
			warn1("rcv UNKNOWN COMMAND");
		}
    	}
    	else
    	{
		warn1("size %d", (unsigned int)comms_get_payload_length(comms, msg));
    	}
}

// Message has been sent
static void radio_send_done (comms_layer_t * comms, comms_msg_t * msg, comms_error_t result, void * user)
{
    	logger(result == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snt %u", result);
    	while(osMutexAcquire(m_mutex, 1000) != osOK);
    	m_sending = false;
    	osMutexRelease(m_mutex);
}

static void radio_start_done (comms_layer_t * comms, comms_status_t status, void * user)
{
    	debug1("started %d", status);
}

// Perform basic radio setup, register to receive GermanTestProject packets
static comms_layer_t* radio_setup (am_addr_t node_addr)
{
    	static comms_receiver_t rcvr;
    	comms_layer_t * radio = radio_init(DEFAULT_RADIO_CHANNEL, 0x22, node_addr);
    	if (NULL == radio)
    	{
		return NULL;
    	}

    	if (COMMS_SUCCESS != comms_start(radio, radio_start_done, NULL))
    	{
        	return NULL;
    	}

    	// Wait for radio to start, could use osTreadFlagWait and set from callback
    	while(COMMS_STARTED != comms_status(radio))
    	{
        	osDelay(1);
    	}

    	comms_register_recv(radio, &rcvr, receive_message, NULL, AMID_RADIO_COUNT_TO_LEDS);
    	debug1("radio rdy");
    	return radio;
}

static void push_button_callback()
{
	const uint32_t flags = osThreadFlagsSet(app_thread_id, FLAGS_MSK1);

	return;
}

int logger_fwrite_boot (const char *ptr, int len)
{
    	fwrite(ptr, len, 1, stdout);
    	fflush(stdout);
    	return len;
}

// LEDS loop 
void leds_loop ()
{
	leds_message_t leds_message;

	// Loop forever
	while (true)
	{
		const osStatus_t status = osMessageQueueGet(leds_queue, &leds_message, NULL, osWaitForever);

		if (status == osOK) 
		{
			if (leds_message.is_turn_on == true)
			{
				const uint8_t all_leds = 0B00000111;

				PLATFORM_LedsSet(all_leds);
			}
			else
			{
				const uint8_t no_leds = 0B00000000;

				PLATFORM_LedsSet(no_leds);
			}
		}
	}
	return;
}

// App loop
void app_loop ()
{
    	am_addr_t node_addr = DEFAULT_AM_ADDR;
    	uint8_t node_eui[8];

    	m_mutex = osMutexNew(NULL);
    	leds_queue = osMessageQueueNew(4, sizeof(leds_message_t), NULL);

    	uint8_t cmd_id = DO_NOTHING;

    	// Initialize node signature - get address and EUI64
    	if (SIG_GOOD == sigInit())
    	{
        	node_addr = sigGetNodeId();
        	sigGetEui64(node_eui);
        	infob1("ADDR:%"PRIX16" EUI64:", node_eui, sizeof(node_eui), node_addr);
    	}
    	else
    	{
        	warn1("ADDR:%"PRIX16, node_addr); // Falling back to default addr
    	}

    	// initialize radio
    	comms_layer_t* radio = radio_setup(node_addr);

    	if (NULL == radio)
    	{
        	err1("radio");
        	for (;;); // panic
    	}

    	// Loop forever
    	while(true)
    	{
		const uint32_t flags = osThreadFlagsWait(FLAGS_MSK1, osFlagsWaitAny, osWaitForever);

            	info1("BUTTON");

		while(osMutexAcquire(m_mutex, 1000) != osOK);

		if(false == m_sending)
		{
			comms_init_message(radio, &m_msg);
			radio_msg_t *packet = comms_get_payload(radio, &m_msg, sizeof(radio_msg_t));

			if (NULL != packet)
			{
				cmd_id = (cmd_id != TURN_ON_LEDS ? TURN_ON_LEDS : TURN_OFF_LEDS);

		        	packet->command_id = cmd_id;

		        	// Send data packet
		       		comms_set_packet_type(radio, &m_msg, AMID_RADIO_COUNT_TO_LEDS);
		        	comms_am_set_destination(radio, &m_msg, AM_BROADCAST_ADDR);
		        	//comms_am_set_source(radio, &m_msg, radio_address); // No need, it will use the one set with radio_init
		        	comms_set_payload_length(radio, &m_msg, sizeof(radio_msg_t));

				//comms_error_t ack_error = comms_set_ack_required(radio, &m_msg, true);
				//debug("ACK : %d %d", ack_error, comms_is_ack_required(radio, &m_msg));

		        	comms_error_t result = comms_send(radio, &m_msg, radio_send_done, NULL);
		        	logger(result == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd %u", result);

		        	if (COMMS_SUCCESS == result)
		        	{
		            		m_sending = true;
		        	}
			}
			else
			{
				err1("pckt");
			}
		}
		osMutexRelease(m_mutex);
    	}
	return;
}


int main ()
{
    	// init platform
	PLATFORM_Init();

    	PLATFORM_LedsInit();
	PLATFORM_PushButtonInit(push_button_callback);
	PLATFORM_RadioInit(); // Radio GPIO/PRS - LNA on some MGM12P

    	// Configure debug output
    	RETARGET_SerialInit();
    	log_init(BASE_LOG_LEVEL, &logger_fwrite_boot, NULL);

	info1("GermanTestProject "VERSION_STR" (%d.%d.%d)", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	// Initialize OS kernel
	osKernelInitialize();

	// Create app thread
	const osThreadAttr_t app_thread_attr = { .name = "app" };
	app_thread_id = osThreadNew(app_loop, NULL, &app_thread_attr);

	// Create leds thread
	const osThreadAttr_t leds_thread_attr = { .name = "leds" };
	osThreadNew(leds_loop, NULL, &leds_thread_attr);

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
