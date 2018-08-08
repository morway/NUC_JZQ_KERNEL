/*
 * drivers/char/hndl_char_devices/hnos_led.h   
 *
 * LED IO pin definition.
 *
 * Modified by  ZhangRM
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

#ifndef __HNDL_AT91_LED_H
#define __HNDL_AT91_LED_H

#define		NR_LEDS		4

struct at91_led_cdev 
{
	unsigned long is_open;	  
	struct class *myclass;
	struct cdev cdev;	       
};

struct at91_led_object 
{
	struct list_head list;
	spinlock_t lock;
	unsigned pin;
	unsigned int id;
	unsigned int is_timer_started;
	struct timer_list timer;
	unsigned char last_stat;
	unsigned long blinking_timeout;
    u8 logic_level;
	int (*at91_led_light)(struct at91_led_object *led, int action);
};


int at91_led_add(struct at91_led_object *led);
int at91_led_remove(struct at91_led_object *led);

#define LOW    0
#define HIGH   1

#endif

