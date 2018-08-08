/*
 *  linux/drivers/serial/hnos_power_hntt1000x.h
 *
 * Author ZhangRM, peter_zrm@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef		HNOS_POWER_H
#define		HNOS_POWER_H

#undef  HNDL_POWER_DEBUG		
#define DEVICE_NAME		            "power"
#define SCAN_POWER_OFF        1
#define SCAN_POWER_ON         0
#define SCAN_POWER_LOW        0
#define SCAN_POWER_HIGH       1

typedef int (*power_state_t)(void);
typedef int (*v6513_state_t)(void);
typedef int (*vcc5v_state_t)(void);
typedef int (*vcc5v_adc_t)(void);


struct at91_power_callbacks
{
	power_state_t power_state;
	v6513_state_t v6513_state;
	vcc5v_state_t vcc5v_state;
	vcc5v_adc_t   vcc5v_adc;
};

struct at91_power_cdev 
{
	spinlock_t lock;                 /* mutual exclusion lock*/
	unsigned long is_open;
	struct class *myclass;
	struct cdev cdev;	         /* Char device structure*/
	struct at91_power_callbacks *power_cbs;
};

int power_module_register(struct at91_power_callbacks *power);
int power_module_unregister(struct at91_power_callbacks *power);

#endif
