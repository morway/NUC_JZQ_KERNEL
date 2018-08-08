/*
 *  drivers/char/hndl_char_devices/hnos_carrier_hntt1800x.c
 *
 *  Author zrm, peter_zrm@163.com
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

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

static struct proc_item carrier_wave_items[] = 
{
	[0] = {
		.name = "carrier_wave_A", 
		.pin = AT91_PIN_PB0,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
		.write_func = hnos_proc_gpio_set,
	},
	[1] = {
		.name = "carrier_wave_B", 
		.pin = AT91_PIN_PB3,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
		.write_func = hnos_proc_gpio_set,
	},	
	[2] = {
		.name = "carrier_wave_C", 
		.pin = AT91_PIN_PB2,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
		.write_func = hnos_proc_gpio_set,
	},	
	{NULL}
};

static int __init  carrier_devices_add(void)
{
	struct proc_item *devices = carrier_wave_items;
	struct proc_item *item;
	int ret = 0;

	for (item = devices; item->name; ++item) {
			hnos_gpio_cfg(item->pin, item->settings);
			ret += hnos_proc_entry_create(item);
	}
	return ret;
}

static int carrier_devices_remove(void)
{
	struct proc_item *devices = carrier_wave_items;
	struct proc_item *item;

	for (item = devices; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}


/* proc module init */
static int __init carrier_module_init(void)
{
	int status;

	HNOS_DEBUG_INFO("Proc Filesystem Interface of Carrier Wave Signals for HNTT1800X init.\n");

	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		status = -ENODEV;
	} else {
		status = carrier_devices_add();
		if (status) {
			hnos_proc_rmdir();
		}
	}

	return (!status) ? 0 : -ENODEV;
}

static void carrier_module_exit(void)
{
	carrier_devices_remove();
	hnos_proc_rmdir();

	HNOS_DEBUG_INFO("Proc Filesystem Interface of Carrier Wave Signals for HNTT1800X exit.\n");
	return;
}

module_init(carrier_module_init);
module_exit(carrier_module_exit);

MODULE_AUTHOR("zrm");
MODULE_LICENSE("Dual BSD/GPL");

