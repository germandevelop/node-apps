#include "tracking_session.h"

#include "result.h"


void tracking_session_init(tracking_session_t * const tracking_session)
{
	tracking_session_reset(tracking_session);

	return;
}

void tracking_session_reset(tracking_session_t * const tracking_session)
{
	tracking_session->id = 0;
	tracking_session->is_active = false;
	
	tracking_session->is_unit_speed_detected = false;
	tracking_session->unit_speed = 0.0;

	tracking_session->is_partner_speed_detected = false;
	tracking_session->partner_speed = 0.0;

	return;
}

void tracking_session_set_id(tracking_session_t * const tracking_session, int32_t id)
{
	tracking_session->id = id;
	tracking_session->is_active = true;

	return;
}

void tracking_session_get_id(tracking_session_t const * const tracking_session, int32_t * const id)
{
	*id = tracking_session->id;

	return;
}

void tracking_session_set_unit_speed(tracking_session_t * const tracking_session, double speed)
{
	tracking_session->is_unit_speed_detected = true;
	tracking_session->unit_speed = speed;

	return;
}

void tracking_session_is_unit_speed_detected(tracking_session_t const * const tracking_session, bool * const is_detected)
{
	*is_detected = tracking_session->is_unit_speed_detected;

	return;
}

void tracking_session_set_partner_speed(tracking_session_t * const tracking_session, double speed)
{
	tracking_session->is_partner_speed_detected = true;
	tracking_session->partner_speed = speed;

	return;
}

void tracking_session_get_partner_speed(tracking_session_t const * const tracking_session, double * const speed)
{
	*speed = tracking_session->partner_speed;

	return;
}

void tracking_session_is_partner_speed_detected(tracking_session_t const * const tracking_session, bool * const is_detected)
{
	*is_detected = tracking_session->is_partner_speed_detected;

	return;
}

void tracking_session_is_active(tracking_session_t const * const tracking_session, bool * const is_active)
{
	*is_active = tracking_session->is_active;
}

void tracking_session_is_valid(tracking_session_t const * const tracking_session, bool * const is_valid)
{
	*is_valid = false;

	const bool is_unit_speed_valid = (tracking_session->is_unit_speed_detected == true) &&
						(tracking_session->unit_speed < MAX_VEHICLE_SPEED_KMH) &&
						(tracking_session->unit_speed > MIN_VEHICLE_SPEED_KMH);

	const bool is_partner_speed_valid = (tracking_session->is_partner_speed_detected == true) &&
						(tracking_session->partner_speed < MAX_VEHICLE_SPEED_KMH) &&
						(tracking_session->partner_speed > MIN_VEHICLE_SPEED_KMH);

	if((is_unit_speed_valid == true) || (is_partner_speed_valid == true))
	{
		*is_valid = true;
	}
	return;
}

