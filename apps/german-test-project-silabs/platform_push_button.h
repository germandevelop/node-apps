
#ifndef _PLATFORM_PUSH_BUTTON_H_
#define _PLATFORM_PUSH_BUTTON_H_

/**
 * PushButton callback.
 */
typedef void (*button_pushed_f) ();

/**
 * Configure platform PUSH BUTTON GPIO.
 */
void PLATFORM_PushButtonInit(button_pushed_f push_button_callback);

#endif

