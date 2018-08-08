/*
 *  linux/drivers/serial/hnos_relay_hntt1000x.h
 *
 * Author ZhangRM, peter_zrm@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef		HNOS_RELAY_H
#define		HNOS_RELAY_H

#undef  HNDL_RELAY_DEBUG		
#define DEVICE_NAME		            "relay"
#define RELAY_START_IO		        AT91_PIN_PA0

#define RELAY_BASE_A		        RELAY_CS2		
#define RELAY_BASE_B		        RELAY_CS1

typedef int (*relay_led_t)(int cmd,long id);
typedef int (*relay_opt_t)(int cmd,long action);

struct at91_relay_callbacks
{
	relay_led_t relay_led;
	relay_opt_t relay_opt;
};

struct at91_relay_cdev 
{
	spinlock_t lock;                 /* mutual exclusion lock*/
	unsigned long is_open;
	struct class *myclass;
	struct cdev cdev;	         /* Char device structure*/
	struct at91_relay_callbacks *relay_cbs;
};
int relay_module_unregister(struct at91_relay_callbacks *relay);
int relay_module_register(struct at91_relay_callbacks *relay);

#endif
