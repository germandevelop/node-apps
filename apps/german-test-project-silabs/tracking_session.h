#ifndef TRACKING_SESSION_H_
#define TRACKING_SESSION_H_

// CONFIGURABLE VALUES
/************************************************************************************************/
#define MAX_VEHICLE_SPEED_KMH 		86.0 // 86 km/h
#define MIN_VEHICLE_SPEED_KMH 		10.0 // 10 km/h
/************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>

typedef struct tracking_session
{
	int32_t id;
	bool is_active;
	
	bool is_unit_speed_detected;
	double unit_speed;

	bool is_partner_speed_detected;
	double partner_speed;

} tracking_session_t;

void tracking_session_init(tracking_session_t * const tracking_session);

void tracking_session_reset(tracking_session_t * const tracking_session);

void tracking_session_set_id(tracking_session_t * const tracking_session, int32_t id);
void tracking_session_get_id(tracking_session_t const * const tracking_session, int32_t * const id);

void tracking_session_set_unit_speed(tracking_session_t * const tracking_session, double speed);
void tracking_session_is_unit_speed_detected(tracking_session_t const * const tracking_session, bool * const is_detected);

void tracking_session_set_partner_speed(tracking_session_t * const tracking_session, double speed);
void tracking_session_get_partner_speed(tracking_session_t const * const tracking_session, double * const speed);
void tracking_session_is_partner_speed_detected(tracking_session_t const * const tracking_session, bool * const is_detected);

void tracking_session_is_active(tracking_session_t const * const tracking_session, bool * const is_active);
void tracking_session_is_valid(tracking_session_t const * const tracking_session, bool * const is_valid);

#endif
