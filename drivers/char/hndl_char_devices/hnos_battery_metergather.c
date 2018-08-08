/*
 *  drivers/char/hndl_char_devices/hnos_battery_metergather.c
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
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include "hnos_bat_adc.h"

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

static struct proc_item battery_nd2000[] = 
{
	[0] = {
		.name = "battery_supply", 
		.pin = AT91_PIN_PA21,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, 
	},
	[1] = {
		.name = "battery_charge", 
		.pin = AT91_PIN_PA20,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, 
	},
};

static int __init bat_module_init(void)
{
	HNOS_DEBUG_INFO("Register proc interface for battery management for hndl-nd2000.\n");

        return hnos_proc_items_create(battery_nd2000, ARRAY_SIZE(battery_nd2000));
}

static void bat_module_exit(void)
{
	HNOS_DEBUG_INFO("Remove proc interface for battery management for hndl-nd2000.\n");

        hnos_proc_items_remove(battery_nd2000, ARRAY_SIZE(battery_nd2000));
	return;
}

module_init(bat_module_init);
module_exit(bat_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

