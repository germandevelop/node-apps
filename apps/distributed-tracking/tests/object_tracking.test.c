#include "object_tracking.h"

#include "unity.h"

#include "motion_data.h"
#include "result.h"
#include "motion_data_examples.h"


#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))


static object_tracking_t object_tracking;


void setUp()
{
	tracking_params_t tracking_params;
	tracking_params.pass_detection_threshold = 0.280;

	object_tracking_init(&object_tracking, &tracking_params);
}

void tearDown()
{
}


void test_DetectTowardSpeed_Success()
{
	// Arrange: create and set up a system under test
	const double *input_motion_data = moving_toward_good_data;
	const tracking_mode_t tracking_mode = OBJECT_MOVING_TOWARD;

	const uint32_t current_time = 0U;
	const uint32_t max_time_length = 500U;

	object_tracking_start(&object_tracking, tracking_mode, current_time, max_time_length);

	// Act: poke the system under test
	for(size_t i = 0U; input_motion_data[i] != LAST_ELEMENT_VALUE; i += 2U)
	{
		motion_data_t motion_data = { .speed = input_motion_data[i], .energy = input_motion_data[i + 1U] };

		object_tracking_add_motion_data(&object_tracking, &motion_data, (uint32_t)i);
	}

	bool is_speed_completed;
	object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

	double speed;
	const int ret_val = object_tracking_get_speed(&object_tracking, &speed);

	// Assert: make unit test pass or fail
	TEST_ASSERT_TRUE(is_speed_completed);
	TEST_ASSERT_EQUAL_INT(SUCCESS, ret_val);
	TEST_ASSERT_DOUBLE_WITHIN(0.5, 48.0, speed);
}

void test_DetectTowardSpeed_NoSpeed()
{
	// Arrange: create and set up a system under test
	const double *input_motion_data = moving_toward_no_speed_data;
	const tracking_mode_t tracking_mode = OBJECT_MOVING_TOWARD;

	const uint32_t current_time = 0U;
	const uint32_t max_time_length = 500U;

	object_tracking_start(&object_tracking, tracking_mode, current_time, max_time_length);

	// Act: poke the system under test
	for(size_t i = 0U; input_motion_data[i] != LAST_ELEMENT_VALUE; i += 2U)
	{
		motion_data_t motion_data = { .speed = input_motion_data[i], .energy = input_motion_data[i + 1U] };

		object_tracking_add_motion_data(&object_tracking, &motion_data, (uint32_t)i);
	}

	bool is_speed_completed;
	object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

	double speed;
	const int ret_val = object_tracking_get_speed(&object_tracking, &speed);

	// Assert: make unit test pass or fail
	TEST_ASSERT_FALSE(is_speed_completed);
	TEST_ASSERT_EQUAL_INT(FAILURE, ret_val);
}

void test_DetectTowardSpeed_TimeOut()
{
	// Arrange: create and set up a system under test
	const double *input_motion_data = moving_toward_good_data;
	const tracking_mode_t tracking_mode = OBJECT_MOVING_TOWARD;

	const uint32_t current_time = 0U;
	const uint32_t max_time_length = 60U;

	object_tracking_start(&object_tracking, tracking_mode, current_time, max_time_length);

	// Act: poke the system under test
	for(size_t i = 0U; input_motion_data[i] != LAST_ELEMENT_VALUE; i += 2U)
	{
		motion_data_t motion_data = { .speed = input_motion_data[i], .energy = input_motion_data[i + 1U] };

		object_tracking_add_motion_data(&object_tracking, &motion_data, (uint32_t)i);
	}

	bool is_speed_completed;
	object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

	double speed;
	const int ret_val = object_tracking_get_speed(&object_tracking, &speed);

	// Assert: make unit test pass or fail
	TEST_ASSERT_TRUE(is_speed_completed);
	TEST_ASSERT_EQUAL_INT(FAILURE, ret_val);
}



void test_DetectAwaySpeed_Success()
{
	// Arrange: create and set up a system under test
	const double *input_motion_data = moving_away_good_data;
	const tracking_mode_t tracking_mode = OBJECT_MOVING_AWAY;

	const uint32_t current_time = 0U;
	const uint32_t max_time_length = 500U;

	object_tracking_start(&object_tracking, tracking_mode, current_time, max_time_length);

	// Act: poke the system under test
	for(size_t i = 0U; input_motion_data[i] != LAST_ELEMENT_VALUE; i += 2U)
	{
		motion_data_t motion_data = { .speed = input_motion_data[i], .energy = input_motion_data[i + 1U] };

		object_tracking_add_motion_data(&object_tracking, &motion_data, (uint32_t)i);
	}

	bool is_speed_completed;
	object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

	double speed;
	const int ret_val = object_tracking_get_speed(&object_tracking, &speed);

	// Assert: make unit test pass or fail
	TEST_ASSERT_TRUE(is_speed_completed);
	TEST_ASSERT_EQUAL_INT(SUCCESS, ret_val);
	TEST_ASSERT_DOUBLE_WITHIN(0.5, 45.0, speed);
}

void test_DetectAwaySpeed_NoSpeed()
{
	// Arrange: create and set up a system under test
	const double *input_motion_data = moving_away_no_speed_data;
	const tracking_mode_t tracking_mode = OBJECT_MOVING_AWAY;

	const uint32_t current_time = 0U;
	const uint32_t max_time_length = 500U;

	object_tracking_start(&object_tracking, tracking_mode, current_time, max_time_length);

	// Act: poke the system under test
	for(size_t i = 0U; input_motion_data[i] != LAST_ELEMENT_VALUE; i += 2U)
	{
		motion_data_t motion_data = { .speed = input_motion_data[i], .energy = input_motion_data[i + 1U] };

		object_tracking_add_motion_data(&object_tracking, &motion_data, (uint32_t)i);
	}

	bool is_speed_completed;
	object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

	double speed;
	const int ret_val = object_tracking_get_speed(&object_tracking, &speed);

	// Assert: make unit test pass or fail
	TEST_ASSERT_FALSE(is_speed_completed);
	TEST_ASSERT_EQUAL_INT(FAILURE, ret_val);
}

void test_DetectAwaySpeed_TimeOut()
{
	// Arrange: create and set up a system under test
	const double *input_motion_data = moving_away_good_data;
	const tracking_mode_t tracking_mode = OBJECT_MOVING_AWAY;

	const uint32_t current_time = 0U;
	const uint32_t max_time_length = 60U;

	object_tracking_start(&object_tracking, tracking_mode, current_time, max_time_length);

	// Act: poke the system under test
	for(size_t i = 0U; input_motion_data[i] != LAST_ELEMENT_VALUE; i += 2U)
	{
		motion_data_t motion_data = { .speed = input_motion_data[i], .energy = input_motion_data[i + 1U] };

		object_tracking_add_motion_data(&object_tracking, &motion_data, (uint32_t)i);
	}

	bool is_speed_completed;
	object_tracking_is_speed_completed(&object_tracking, &is_speed_completed);

	double speed;
	const int ret_val = object_tracking_get_speed(&object_tracking, &speed);

	// Assert: make unit test pass or fail
	TEST_ASSERT_TRUE(is_speed_completed);
	TEST_ASSERT_EQUAL_INT(FAILURE, ret_val);
}


