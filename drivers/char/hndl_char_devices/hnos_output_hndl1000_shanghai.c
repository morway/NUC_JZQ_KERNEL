/*
 *  drivers/char/hndl_char_devices/hnos_rmc_hntt1000.c shanghai
 *
 *  Author ZhangRM,peter_zrm@163.com
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
#include "hnos_output.h"

static struct hndl_rmc_device *rmc_hntt1000_shanghai;
static struct proc_item items_hntt1000_shanghai[] = 
{
	{
		.name = "led_uart3_tx", 
		.pin = AT91_PIN_PC2,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "led_uart3_rx", 
		.pin = AT91_PIN_PC1,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "led_uart2_tx", 
		.pin = AT91_PIN_PC3,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "led_uart2_rx", 
		.pin = AT91_PIN_PB19,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},	

        {
		.name = "led_err", 
		.pin = AT91_PIN_PA6,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "led_net1", 
		.pin = AT91_PIN_PA7,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},	
	{
		.name = "led_net2", 
		.pin = AT91_PIN_PA8,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "led_a", 
		.pin = AT91_PIN_PA2,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "led_b", 
		.pin = AT91_PIN_PA0,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	
	{
		.name = "led_c", 
		.pin = AT91_PIN_PA1,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	
	{
		.name = "gprs_power", 
		.pin = AT91_PIN_PA11,
		.settings = GPIO_OUTPUT_MASK , /* output,  */
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "gprs_reset", 
		.pin = AT91_PIN_PA10,
		.settings = GPIO_OUTPUT_MASK , /* output,  */
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
	{
		.name = "gprs_start", 
		.pin = AT91_PIN_PA18,
		.settings = GPIO_OUTPUT_MASK , /* output,  */
		.write_func = hnos_proc_gpio_set_reverse,
		.read_func = hnos_proc_gpio_get_reverse,
	},
        {NULL}
};


static struct gpio_rmc_data gpio_rmc_hntt1000_shanghai =
{
	.items = items_hntt1000_shanghai,
	.size = ARRAY_SIZE(items_hntt1000_shanghai),
};

static void  rmc_hntt1000_shanghai_remove(void)
{

	rmc_gpio_unregister(rmc_hntt1000_shanghai, &gpio_rmc_hntt1000_shanghai);

	rmc_device_unregister(rmc_hntt1000_shanghai);

	rmc_device_free(rmc_hntt1000_shanghai);

	return;

}


static int __devinit rmc_hntt1000_shanghai_init(void)
{
        u8 offset = 0;
        u8 size = ARRAY_SIZE(items_hntt1000_shanghai);
        int ret =0;
        
	rmc_hntt1000_shanghai = rmc_device_alloc();

	if (!rmc_hntt1000_shanghai) {
		return -1;

	}

        rmc_gpio_register(rmc_hntt1000_shanghai, 
                        &gpio_rmc_hntt1000_shanghai, offset, size);

        ret = rmc_device_register(rmc_hntt1000_shanghai);
        for(offset =0; offset< 10; offset++){
	        at91_set_GPIO_periph(items_hntt1000_shanghai[offset].pin,0);
                at91_set_gpio_value(items_hntt1000_shanghai[offset].pin, 1);
        }        
	return ret;


}

module_init(rmc_hntt1000_shanghai_init);
module_exit(rmc_hntt1000_shanghai_remove);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
