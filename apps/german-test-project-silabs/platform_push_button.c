
#include "platform_push_button.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "platform_io.h"
#include "gpiointerrupt.h"


static button_pushed_f push_button_interruption_callback;

static void push_button_gpio_callback(uint8_t pin);


void PLATFORM_PushButtonInit(button_pushed_f push_button_callback)
{
	CMU_ClockEnable(cmuClock_GPIO, true);
	GPIO_PinModeSet(PLATFORM_BUTTON_PORT, PLATFORM_BUTTON_PIN, gpioModeInputPullFilter, 1);

	push_button_interruption_callback = push_button_callback;

	GPIOINT_Init();
	GPIOINT_CallbackRegister(PLATFORM_BUTTON_PIN, push_button_gpio_callback);

	GPIO_ExtIntConfig(PLATFORM_BUTTON_PORT, PLATFORM_BUTTON_PIN, PLATFORM_BUTTON_PIN, false, true, true);

	// need change interruption priority
	NVIC_SetPriority(GPIO_EVEN_IRQn, 1U);
	NVIC_SetPriority(GPIO_ODD_IRQn, 1U);

	return;
}


void push_button_gpio_callback(uint8_t pin)
{
	push_button_interruption_callback();

	return;
}

