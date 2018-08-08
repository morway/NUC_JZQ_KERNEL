/*
 * drivers/char/hndl_char_devices/hnos_led_hntt1000_1800e.c
 * 
 * LED for hntt1000_1800e. shanghai
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

static int hntt1000_sh_led_light(struct at91_led_object *led, int action);
struct at91_led_object leds_hntt1000_shanghai[] = {
	[0] = {
		.id  = ERR_LED,
		.pin = AT91_PIN_PA6,
		.at91_led_light = hntt1000_sh_led_light,
	},
	[1] = {
		.id  = NET_LED_1,
		.pin = AT91_PIN_PA7,
		.at91_led_light = hntt1000_sh_led_light,
	},
	[2] = {
		.id  = NET_LED_2,
		.pin = AT91_PIN_PA8,
		.at91_led_light = hntt1000_sh_led_light,
	},	
	[3] = {
		.id  = A_LED,
		.pin = AT91_PIN_PA2,
		.at91_led_light = hntt1000_sh_led_light,
	},
	[4] = {
		.id  = B_LED,
		.pin = AT91_PIN_PA0,
		.at91_led_light = hntt1000_sh_led_light,
	},
	[5] = {
		.id  = C_LED,
		.pin = AT91_PIN_PA1,
		.at91_led_light = hntt1000_sh_led_light,
	},
	[6] = {
		.id  = UART2_TX_LED,
		.pin = AT91_PIN_PC3,
                .at91_led_light = hntt1000_sh_led_light,
	},
	[7] = {
		.id  = UART2_RX_LED,
		.pin = AT91_PIN_PB19,
                .at91_led_light = hntt1000_sh_led_light,
	},
	[8] = {
		.id  = UART3_TX_LED,
		.pin = AT91_PIN_PC2,
		.at91_led_light = hntt1000_sh_led_light,
	},
	[9] = {
		.id  = UART3_RX_LED,
		.pin = AT91_PIN_PC1,
		.at91_led_light = hntt1000_sh_led_light,
	},	
};
static int hntt1000_sh_led_light(struct at91_led_object *led, int action)
{
	int value = 1;
	unsigned pin = 0;

	if (led == NULL) {
		printk("Empty led object, something error.\n");
		return -1;
	}
	pin = led->pin;

	if (action == LED_ON) {
		value = 0;
	} else if (action == LED_OFF){
		value = 1;
	}
        dprintk("led object =%d\n",value);
	at91_set_gpio_value(pin, value);
	return 0;
}



static __init int leds_hntt1000_shanghai_init(void) 
{
	int i = 0;
	for (i=0; i<ARRAY_SIZE(leds_hntt1000_shanghai); i++) {
	        at91_set_GPIO_periph(leds_hntt1000_shanghai[i].pin,0);
		at91_led_add(&leds_hntt1000_shanghai[i]);
                at91_set_gpio_value(leds_hntt1000_shanghai[i].pin, 1);
                dprintk("close led =%d\n",i);
	}
	return 0;
}

static __exit void leds_hntt1000_shanghai_exit(void)
{
	int i = 0;
	for (i=0; i<ARRAY_SIZE(leds_hntt1000_shanghai); i++) {
		at91_led_remove(&leds_hntt1000_shanghai[i]);	
	}
	return;
}

module_init(leds_hntt1000_shanghai_init);
module_exit(leds_hntt1000_shanghai_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
