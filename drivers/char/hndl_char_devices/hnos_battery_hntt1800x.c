/*
 *  drivers/char/hndl_char_devices/hnos_battery_generic.c
 *
 *  Battery Control for HNTT1800X and etc.
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

static struct proc_item battery_hntt1800x[] = 
{
	[0] = {
		.name = "battery_supply",		
	  	.pin = AT91_PIN_PB16,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, 
	},

	[1] = {
		.name = "battery_charge", 
		.pin = AT91_PIN_PB18,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, 
	},

	[2] = {
		.name = "battery_adc", 
		.read_func = bat_voltage_get_hex,
	},

	[3] = {
		.name = "vcc5v_adc", 
		.read_func = vcc5v_voltage_get,
	},
	[4] = {
		.name = "rtc_adc", 
		.read_func = rtc_voltage_get,
	},
	
};

static int __init bat_module_init(void)
{
	HNOS_DEBUG_INFO("Register a proc interface for battery management for hntt1800x.\n");
    return hnos_proc_items_create(battery_hntt1800x, ARRAY_SIZE(battery_hntt1800x));
}

static void bat_module_exit(void)
{
	HNOS_DEBUG_INFO("Remove a proc interface for battery management for hntt1800u.\n");
    hnos_proc_items_remove(battery_hntt1800x, ARRAY_SIZE(battery_hntt1800x));
	return;
}

module_init(bat_module_init);
module_exit(bat_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

