/*
 *  drivers/char/hndl_char_devices/hnos_battery.c
 *
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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>

#include <linux/proc_fs.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>

#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <linux/ctype.h>

#include <asm/uaccess.h>
#include <mach/adc.h>

#include <mach/board.h>
#include <mach/gpio.h>

#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include <linux/hnos_product.h>
#include <linux/hnos_debug.h>

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

#if defined (CONFIG_HNDL_PRODUCT_NETMETER)

#elif defined (CONFIG_HNDL_PRODUCT_METERGATHER) 

#elif defined (CONFIG_HNDL_PRODUCT_HNTT1800X) || defined (CONFIG_HNDL_PRODUCT_HNDL900FJ)
static struct proc_item battery_manage_items[] = 
{
	[0] = {
		.name = "battery_supply", 
		.pin = AT91_PIN_PB16,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
		.read_func = hnos_proc_gpio_get,
		.write_func = hnos_proc_gpio_set,
	},
	[1] = {
		.name = "battery_charge", 
		.pin = AT91_PIN_PB18,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
		.read_func = hnos_proc_gpio_get,
		.write_func = hnos_proc_gpio_set,
	},
	[2] = {
		.name = "battery_adc", 
		.read_func = bat_voltage_get_hex,
	},
	[3] = {
		.name = "poweroff_state", 
		//.pin = AT91_PIN_PC1, //用于Battery采样
	  //.pin = AT91_PIN_PC3, //记录VCC_5v使用情况,edit by sethf,2010-08-31
		.pin = AT91_PIN_PC2, //用于5v检测 ,edit by tuxr ,2013-06-24
		.read_func = hnos_proc_gpio_get_reverse,
	},	

	{NULL}
};
#else
static struct proc_item battery_manage_items[] = 
{
	{NULL}
};
#endif


#if defined (CONFIG_HNDL_PRODUCT_NETMETER)
#else
static void __init proc_gpio_remap(struct proc_item *item)  { }
#endif

static int __init  proc_devices_add(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = battery_manage_items; item->name; ++item) {
		if ( !netmeter_id_match(item->id_mask) ) {

			proc_gpio_remap(item);
			hnos_gpio_cfg(item->pin, item->settings);
			ret += hnos_proc_entry_create(item);
			HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management:%s\n",item->name);
		}
	}

	return ret;
}

static int proc_devices_remove(void)
{
	struct proc_item *item;

	for (item = battery_manage_items; item->name; ++item) {
		if ( !netmeter_id_match(item->id_mask) ) {
			remove_proc_entry(item->name, hndl_proc_dir);
		}
	}

	return 0;
}


/* proc module init */
static int __init proc_module_init(void)
{
	int status;

	HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management.\n");
	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		status = -ENODEV;
		HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management1.\n");
	} else {
		status = proc_devices_add();
		HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management2.\n");
		if (status) {
			hnos_proc_rmdir();
		}
	}
	HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management3.\n");

	return (!status) ? 0 : -ENODEV;
}

static void proc_module_exit(void)
{
	proc_devices_remove();
	hnos_proc_rmdir();

	return;
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

