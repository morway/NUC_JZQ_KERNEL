/*
 *  drivers/char/hndl_char_devices/hnos_meter_event.c
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

#include "hnos_generic.h"
#include "hnos_proc.h"
#include "hnos_gpio.h"

#define HNOS_PROC_ROOT			"hndl"

static  struct proc_dir_entry		*hndl_proc_dir;

static struct proc_item meter_event_items[] = 
{
	[0] = {
		.name = "event_state", 
		.pin = AT91_PIN_PA24,
		.settings = 0, /* input, no pullup */
		.id_mask = NETMETER_ID_MASK_ALL,
		.read_func = hnos_proc_gpio_get,
	},
	[1] = {
		.name = "module_hotplug_state", 
		.pin = AT91_PIN_PA8,
		.settings = GPIO_PULLUP, /* input, pullup */
		.id_mask = NETMETER_ID_MASK_ALL & ~(PRODUCT_HNDL800BC_V100),
		.read_func = hnos_proc_gpio_get,
	},
	{NULL}
};


static int __init  proc_devices_add(void)
{
	struct proc_item *devices = meter_event_items;
	struct proc_item *item;
	int ret = 0;

	for (item = devices; item->name; ++item) {
		if ( ID_MATCHED == netmeter_id_match(item->id_mask) ) {
			hnos_gpio_cfg(item->pin, item->settings);
			ret += hnos_proc_entry_create(item);
		}
	}

	return ret;
}

static int proc_devices_remove(void)
{
	struct proc_item *devices = meter_event_items;
	struct proc_item *item;

	for (item = devices; item->name; ++item) {
		if ( ID_MATCHED ==  netmeter_id_match(item->id_mask) ) {
			remove_proc_entry(item->name, hndl_proc_dir);
		}
	}

	return 0;
}


/* proc module init */
static int __init proc_module_init(void)
{
	int status;

	HNOS_DEBUG_INFO("Proc Filesystem Interface of Meter Event State for NetMeter.\n");

	if (!hndl_proc_dir) {
		hndl_proc_dir = hnos_proc_mkdir();
	}
	if (!hndl_proc_dir) {
		status = -ENODEV;
	} else {
		status = proc_devices_add();
		if (status) {
			hnos_proc_rmdir();
		}
	}

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

