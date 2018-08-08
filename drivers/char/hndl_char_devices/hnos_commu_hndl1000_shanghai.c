/*
 * drivers/char/hndl_char_devices/hnos_commu_generic.c
 *
 * Communication modules for Netmeter/HNTT1800X/HNDL-ND2000/HNDL900B and etc.
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

#include "hnos_generic.h"
#include "hnos_commu.h"
#include "hnos_ioctl.h"

struct at91_commu_object  commu_hndl1000_shanghai = 
{
	.start_pin = AT91_PIN_PA18,
	.power_pin = AT91_PIN_PA11,
	.reset_pin = AT91_PIN_PA10,
};

static int commu_type_hndl1000_shanghai(void)
{
	u8 value = 0,i;
        unsigned int comm_type[5]={
        AT91_PIN_PA16,
        AT91_PIN_PA17,
        AT91_PIN_PA19,
        AT91_PIN_PA15,
        AT91_PIN_PA14
        };
        
        for (i=0; i<5; i++)
        {
		value |= (at91_get_gpio_value(comm_type[i]) << i);
        }
	return value;
}

int __init commu_gpio_init(void)
{
	struct at91_commu_object  *dev = &commu_hndl1000_shanghai;

	dev->commu_type = commu_type_hndl1000_shanghai;
	
	at91_set_gpio_output(dev->power_pin, 1);
	at91_set_gpio_output(dev->start_pin, 1);

	if (dev->reset_pin) {
		at91_set_gpio_output(dev->reset_pin, 0);
	}

	return 0;
}

static void __exit commu_hndl1000_shanghai_exit(void)
{
	commu_module_unregister(&commu_hndl1000_shanghai);

        HNOS_DEBUG_INFO("unregistered the commu module for hntt1800 Shanghai.");
	return;
}

static int __init  commu_hndl1000_shanghai_init(void)
{
        HNOS_DEBUG_INFO("Registered a commu module for hntt1800 Shanghai.");

	commu_gpio_init();
	return commu_module_register(&commu_hndl1000_shanghai);
}

module_init(commu_hndl1000_shanghai_init);
module_exit(commu_hndl1000_shanghai_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

