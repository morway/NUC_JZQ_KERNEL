/*
 *  drivers/char/hndl_char_devices/hnos_rmi_hntt1000_shanghai.c
 *
 *  Author ZhangRM, peter_zrm@163.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "hnos_generic.h"
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include "hnos_input.h"

struct hndl_rmi_device *rmi_hntt1000_shanghai;
static struct proc_item items_hntt1000_shanghai[] = 
{   
	{       .name = "KEY_SET", 
	        .pin = AT91_PIN_PB1,
	        .settings = GPIO_PULLUP, /* input, pullup */
        	.read_func = hnos_proc_gpio_get,
	}, 
  	{       .name = "writeeprom_state", 
       		.pin = AT91_PIN_PA12,
       		.settings = GPIO_PULLUP, 
       		.read_func = hnos_proc_gpio_get_reverse,
        },
};
static struct gpio_rmi_data gpio_hntt1000_shanghai =
{
	.items = items_hntt1000_shanghai,
	.size = ARRAY_SIZE(items_hntt1000_shanghai),
};

static void  gpio_channel_remove(void)
{
	rmi_gpio_unregister(rmi_hntt1000_shanghai, &gpio_hntt1000_shanghai);
	return;
}

static int gpio_channel_init(void)
{
	u8 offset = 0;
	u8 size = ARRAY_SIZE(items_hntt1000_shanghai);
	return rmi_gpio_register(rmi_hntt1000_shanghai, &gpio_hntt1000_shanghai, offset, size);
}
static int __devinit rmi_hntt1000_shanghai_init(void)
{
	rmi_hntt1000_shanghai = rmi_device_alloc();
	if (!rmi_hntt1000_shanghai) {
		return -1;
	}
	gpio_channel_init();

	return rmi_device_register(rmi_hntt1000_shanghai);
}
static void __devexit rmi_hntt1000_shanghai_remove(void)
{
	gpio_channel_remove();
	rmi_device_unregister(rmi_hntt1000_shanghai);
	rmi_device_free(rmi_hntt1000_shanghai);
	return;
}

module_init(rmi_hntt1000_shanghai_init);
module_exit(rmi_hntt1000_shanghai_remove);
MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
