/*
 * drivers/char/hndl_char_devices/hnos_commu_generic.c
 *
 * Communication modules for HNTT1800X/HNDL-ND2000/HNDL900B/HNDL900FJ/HNTT1800SJL and etc.
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

#define COMMU_ID_BASE	UL(0x30000000)
void __iomem *base = NULL;

struct at91_commu_object  commu_generic = 
{
	.start_pin = AT91_PIN_PA10,
	.power_pin = AT91_PIN_PA6,
	.reset_pin = AT91_PIN_PA11,
};

static int commu_type_get(void)
{
	if(base) 
	{
		return (readb(base) & 0x1f);
	}

	return -1;
}

int __init commu_gpio_init(void)
{
	struct at91_commu_object  *dev = &commu_generic;

	base = ioremap(COMMU_ID_BASE, 1);
	if(!base) 
	{
		printk(KERN_ERR " Can NOT remap commu id base %8x.\n", (unsigned int)COMMU_ID_BASE);
		return -1;
	}

	/*
	dev->start_pin = AT91_PIN_PA10;	
	dev->power_pin = AT91_PIN_PA6;
	dev->reset_pin = AT91_PIN_PA11;
	*/

	at91_set_gpio_output(dev->power_pin, 0);	// SIM卡加热控制信号，"0" 关断  	
	at91_set_gpio_output(dev->start_pin, 1);	// 模块开机控制信号，低电平1 S  开机
	at91_set_gpio_output(dev->reset_pin, 1);	// 模块复位控制信号，"0" 复位

	return 0;
}

static void __exit commu_generic_exit(void)
{
	commu_module_unregister(&commu_generic);
	if(base) 
	{
		iounmap(base);
		base = NULL;
	}

    HNOS_DEBUG_INFO("Unregisterd the generic Commu module for hntt1800x and etc.\n");
	return;
}

static int __init  commu_generic_init(void)
{
	int ret = 0;

	ret = commu_gpio_init();
    if(ret) 
	{
    	return ret;
    }

    HNOS_DEBUG_INFO("Registered a generic Commu module for hntt1800x and etc.\n");

	commu_generic.commu_type = commu_type_get;
	return commu_module_register(&commu_generic);
}

module_init(commu_generic_init);
module_exit(commu_generic_exit);

MODULE_LICENSE("Dual BSD/GPL");

