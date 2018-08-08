/*
 * drivers/char/hndl_char_devices/hnos_led_hntt1800x.c
 * 
 * LED for hntt1800x.
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include "hnos_generic.h"
#include "hnos_ioctl.h"                /* ioctl */
#include "hnos_led.h"                  /* led  */

#define RUN_LED		WORK_LED
#define WARN_LED    METER_LED

struct at91_led_object leds_hntt1800x[] = {
    [0] = {
        .id  = RUN_LED,
        .pin = AT91_PIN_PB23,
        .logic_level = LOW,
    },
    [1] = {
    	.id  = WARN_LED,
        .pin = AT91_PIN_PB22,
        .logic_level = LOW,
    },
};

static __init int leds_hntt1800x_init(void) 
{
    int i = 0;
    for(i = 0; i < ARRAY_SIZE(leds_hntt1800x); i++) 
	{
    	at91_led_add(&leds_hntt1800x[i]);
    }

    HNOS_DEBUG_INFO("Led module for hntt1800x registered. \n");
    return 0;
}

static __exit void leds_hntt1800x_exit(void)
{
    int i = 0;
    for(i = 0; i < ARRAY_SIZE(leds_hntt1800x); i++) 
	{
        at91_led_remove(&leds_hntt1800x[i]);	
    }

    HNOS_DEBUG_INFO("Led module for hntt1800x unregistered. \n");
    return;
}

module_init(leds_hntt1800x_init);
module_exit(leds_hntt1800x_exit);

MODULE_LICENSE("Dual BSD/GPL");
