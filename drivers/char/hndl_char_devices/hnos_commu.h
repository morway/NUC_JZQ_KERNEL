/*
 * /drivers/char/wfe_char_devices/hnos_commu.h   
 *
 * Sub routines for communiction modules, such as GPRS, CDMA etc.
 *
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

#ifndef __HNDL_COMMU_MODULES_H
#define __HNDL_COMMU_MODULES_H
#include  <linux/hnos_product.h>

#define HNDL_COMM_DEBUG		1

enum  EPower
{
	ePowerON = 1,
	ePowerOFF = 0
};

struct at91_commu_object
{
	unsigned start_pin;
	unsigned reset_pin;
	unsigned power_pin;
	int (*start)(struct at91_commu_object *commu, int level);
	int (*reset)(struct at91_commu_object *commu, int level);
	int (*power)(struct at91_commu_object *commu, enum EPower power);
	int (*commu_type)(void);
};

struct at91_commu_cdev 
{
	unsigned long is_open;	  
	struct class *class;
	struct cdev cdev;	       
	struct semaphore lock;
	struct at91_commu_object *commu;
};

int commu_module_register(struct at91_commu_object *commu);
int commu_module_unregister(struct at91_commu_object *commu);

#endif
