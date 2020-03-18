#ifndef RADIO_MSG_H_
#define RADIO_MSG_H_

#pragma pack(push, 1)
typedef struct radio_msg
{
	uint8_t command_id;

} radio_msg_t;
#pragma pack(pop)

typedef enum radio_command
{
	DO_NOTHING = 0,
	TURN_ON_LEDS,
	TURN_OFF_LEDS

} radio_command_t;

enum RadioCountToLedsEnum
{
    AMID_RADIO_COUNT_TO_LEDS = 6,
};

#endif
