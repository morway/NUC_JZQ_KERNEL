/*
 * drivers/char/hndl_char_devices/hnos_led_hntt1000x.c
 * 
 * LED for hntt1000x.
 * Author ZhangRM, peter_zrm@163.com
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
#include "hnos_iomem.h"                /* iomem object  */
#include "hnos_hndl1000x.h"

struct at91_led_busmem 
{
	unsigned int id;
	unsigned int bit;
	long phy_busmem;
	struct iomem_object *iomem;		  
};

static int hntt1000x_led_light(struct at91_led_object *led, int action);

struct at91_led_busmem leds_busmem_hntt1000x[] = {
	[0] = {
		.id  = LED_FAULT,
		.bit = FAULT,
		.phy_busmem = GPRS_LCD_BEEP_CS,
		.iomem = NULL,
	},
};


struct at91_led_object leds_hntt1000x[] = {
	[0] = {
		.id  = LED_FAULT,
		.pin = 0,
		.at91_led_light = hntt1000x_led_light,
	},
};

static int hntt1000x_led_light(struct at91_led_object *led, int action)
{
	int i,data = 1,ret = 0;
	struct at91_led_busmem *busdev = leds_busmem_hntt1000x;

	if (led == NULL) {
		printk("Empty led object, something error.\n");
		return -1;
	}

	if (action == LED_ON) {
		data = 1;
	} else if (action == LED_OFF){
		data = 0;
	}

	for (i=0; i<ARRAY_SIZE(busdev); i++) 
	{   if(busdev[i].id == led->id ){
		ret = busdev[i].iomem->write_bit(busdev[i].iomem,busdev[i].bit,data);
		break;
		}    
	}
	return ret;
}


static __init int leds_hntt1000x_init(void) 
{   int result = 0;
	int i = 0;
	struct at91_led_busmem *busdev = leds_busmem_hntt1000x;
	struct at91_led_object *ledsdev = leds_hntt1000x;


	for (i=0; i<ARRAY_SIZE(busdev); i++){
		if (i > 0){ 
			if (busdev[i].phy_busmem == busdev[i-1].phy_busmem){
				busdev[i].iomem = busdev[i-1].iomem;
				continue;
			}
		}  

		busdev[i].iomem = iomem_object_get(busdev[i].phy_busmem, 0);
		if (!busdev[i].iomem) {
			printk(KERN_ERR "Can NOT remap address %ld\n", busdev[i].phy_busmem);
			result = -1;
			goto iomem_fail;
		}

	}    

	for (i=0; i<ARRAY_SIZE(ledsdev); i++) {
		at91_led_add(&ledsdev[i]);
	}

	return result;

iomem_fail:
	for (i=0; i<ARRAY_SIZE(busdev); i++){ 
		if (i > 0){ 
			if (busdev[i].phy_busmem == busdev[i-1].phy_busmem){
				continue;
			}
		}   

		if (busdev[i].iomem != NULL) {
			iomem_object_put(busdev[i].iomem);
		}

	} 
	return result;
}

static __exit void leds_hntt1000x_exit(void)
{
	int i = 0;
	struct at91_led_busmem *busdev = leds_busmem_hntt1000x;
	struct at91_led_object *ledsdev = leds_hntt1000x;


	for (i=0; i<ARRAY_SIZE(ledsdev); i++) {
		at91_led_remove(&ledsdev[i]);    
	}

	for (i=0; i<ARRAY_SIZE(busdev); i++){ 
		if (i > 0){ 
			if (busdev[i].phy_busmem == busdev[i-1].phy_busmem){
				continue;
			}
		}

		if (busdev[i].iomem) {
			iomem_object_put(busdev[i].iomem);
		}

	}

	return;
}

module_init(leds_hntt1000x_init);
module_exit(leds_hntt1000x_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
