#ifndef TRACKING_PARTNER_H_
#define TRACKING_PARTNER_H_

#include "radio_socket.h"

typedef struct tracking_summary tracking_summary_t;

typedef enum tracking_partner_error
{
	COMMUNICATION_ERROR = 0,
	TRACKING_ERROR

} tracking_partner_error_t;

typedef void(*tracking_session_state_set_f)(int32_t tracking_session_id, bool is_needed_to_start);
typedef void(*tracking_summary_process_f)(tracking_summary_t const * const tracking_summary);
typedef void(*tracking_failure_catch_f)(tracking_partner_error_t error_code);

typedef struct tracking_partner
{
	radio_socket_t radio_socket;

	tracking_session_state_set_f set_session_state_callback;
	tracking_summary_process_f process_summary_callback;
	tracking_failure_catch_f catch_failure_callback;

} tracking_partner_t;

void tracking_partner_init(tracking_partner_t * const tracking_partner,
				tracking_session_state_set_f set_session_state_callback,
				tracking_summary_process_f process_summary_callback,
				tracking_failure_catch_f catch_failure_callback);

int tracking_partner_setup_radio_network(tracking_partner_t * const tracking_partner,
					comms_layer_t *radio,
					am_addr_t partner_address,
					am_id_t port);


int tracking_partner_start_session(tracking_partner_t * const tracking_partner,
					int32_t tracking_session_id);

int tracking_partner_stop_session(tracking_partner_t * const tracking_partner,
					int32_t tracking_session_id);

int tracking_partner_send_failure(tracking_partner_t * const tracking_partner,
					int32_t tracking_session_id);

int tracking_partner_send_summary(tracking_partner_t * const tracking_partner,
					tracking_summary_t const * const tracking_summary);

#endif
