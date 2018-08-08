/*
 *  drivers/char/hndl_char_devices/hnos_rmc_mb9260.c
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
#include "hnos_output.h"

#ifdef CONFIG_MB9260_VERSION_2
#define gpio_power_ctrl_generic         hnos_proc_gpio_set_reverse
#define snd_power_ctrl			hnos_proc_snd_power_ctrl

#else
#define gpio_power_ctrl_generic		hnos_proc_gpio_set
#define snd_power_ctrl                  hnos_proc_gpio_set
#endif


static struct hndl_rmc_device *rmc_mb9260;
static struct proc_item items_mb9260[] = 
{
	{
		.name = "irda-power", 
		.pin = AT91_PIN_PB21,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, 
		.write_func = gpio_power_ctrl_generic,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "wireless-power", 
		.pin = AT91_PIN_PB29,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, 
		.write_func = gpio_power_ctrl_generic,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "bat-prog", 
		.pin = AT91_PIN_PA27,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH , 
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "soft-power", 
		.pin = AT91_PIN_PA25,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH ,
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "lcd-reset", 
		.pin = AT91_PIN_PB31,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH ,
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "lcd-backlight", 
		.pin = AT91_PIN_PB30,
#ifdef CONFIG_MB9260_VERSION_2 
		.settings = GPIO_OUTPUT_MASK,
#else
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH,
#endif
		.write_func = gpio_power_ctrl_generic,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "snd-power", 
#ifndef CONFIG_MB9260_VERSION_2 
		.pin = AT91_PIN_PC2,
#endif
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = snd_power_ctrl,
		.read_func = hnos_proc_gpio_get,
	},
};

static struct gpio_rmc_data gpio_mb9260 =
{
	.items = items_mb9260,
	.size = ARRAY_SIZE(items_mb9260),
};

static int __devinit gpio_channels_init(struct hndl_rmc_device *rmc)
{
	u8 offset = 0;
	u8 size = ARRAY_SIZE(items_mb9260);
	return rmc_gpio_register(rmc, &gpio_mb9260, offset, size);
}

static void  rmc_mb9260_remove(void)
{
	rmc_gpio_unregister(rmc_mb9260, &gpio_mb9260);
	rmc_device_unregister(rmc_mb9260);
	rmc_device_free(rmc_mb9260);
	return;
}

static int __devinit rmc_mb9260_init(void)
{
	rmc_mb9260 = rmc_device_alloc();
	if (!rmc_mb9260) {
		return -1;
	}

	gpio_channels_init(rmc_mb9260);
	return rmc_device_register(rmc_mb9260);
}

module_init(rmc_mb9260_init);
module_exit(rmc_mb9260_remove);

MODULE_LICENSE("Dual BSD/GPL");


