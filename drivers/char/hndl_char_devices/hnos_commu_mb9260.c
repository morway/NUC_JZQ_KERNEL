/*
 * drivers/char/hndl_char_devices/hnos_commu_mb9260.c
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

struct at91_commu_object  commu_mb9260 = 
{
	.start_pin = AT91_PIN_PB20,
	.power_pin = AT91_PIN_PB19,
	.reset_pin = AT91_PIN_PA5,
};

static int commu_type_get(void)
{
	return eGPRS_siemens;
}

int __init commu_gpio_init(void)
{
	struct at91_commu_object  *dev = &commu_mb9260;
	
	at91_set_gpio_output(dev->power_pin, 0);
	at91_set_gpio_output(dev->start_pin, 1);
	at91_set_gpio_output(dev->reset_pin, 0);

	return 0;
}

static void __exit commu_mb9260_exit(void)
{
	commu_module_unregister(&commu_mb9260);

        HNOS_DEBUG_INFO("Unregisterd the commu module for mobile9260.");
	return;
}

static int __init  commu_mb9260_init(void)
{
        HNOS_DEBUG_INFO("Registered a commu module for mobile9260.");

	commu_gpio_init();
	commu_mb9260.commu_type = commu_type_get;
	return commu_module_register(&commu_mb9260);
}

module_init(commu_mb9260_init);
module_exit(commu_mb9260_exit);

MODULE_LICENSE("Dual BSD/GPL");

