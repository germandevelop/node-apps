#ifndef TRACKING_PARTNER_H_
#define TRACKING_PARTNER_H_

#include <stdbool.h>

#include "radio_socket.h"

typedef struct tracking_report tracking_report_t;

typedef void(*tracking_state_set_f)(bool is_turn_on);
typedef void(*tracking_report_process_f)(tracking_report_t const * const tracking_report);

typedef struct tracking_partner
{
	radio_socket_t radio_socket;

	tracking_state_set_f set_state_callback;
	tracking_report_process_f process_report_callback;

} tracking_partner_t;

int tracking_partner_init(tracking_partner_t * const tracking_partner,
				tracking_state_set_f set_state_callback,
				tracking_report_process_f process_report_callback);

int tracking_partner_turn_on_radar(tracking_partner_t * const tracking_partner);
int tracking_partner_turn_off_radar(tracking_partner_t * const tracking_partner);
int tracking_partner_send_tracking_report(tracking_partner_t * const tracking_partner,
						tracking_report_t const * const tracking_report);

#endif
