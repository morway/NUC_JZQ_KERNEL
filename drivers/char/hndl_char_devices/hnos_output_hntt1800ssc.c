/*
 *  drivers/char/hndl_char_devices/hnos_rmc_hntt1800ssc.c
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
#include "hnos_hntt1800ssc.h"
#include "hnos_iomem.h"                /* iomem object  */

#define HNTT1800SC_BEEP_CS    0x30000080

struct iomem_object *beep_iomem = NULL;
static struct hndl_rmc_device *rmc_hntt1800ssc;
struct smcbus_rmc_data smcbus_hntt1800ssc;
static struct proc_item items_hntt1800ssc[] = 
{
	{
		.name = "output-ctrl0", 
		.pin = AT91_PIN_PA13,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output */
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "output-ctrl1", 
		.pin = AT91_PIN_PA14,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH , /* output, init high. */
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "alarm-output", 
		.pin = AT91_PIN_PA15,
		.settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH , /* output, init high. */
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "remote-ctrl-power", 
		.pin = AT91_PIN_PA16,
		.settings = GPIO_OUTPUT_MASK ,		/* output, init low. */
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "lcd-backlight", 
		.pin = AT91_PIN_PB19,
		.settings = GPIO_OUTPUT_MASK ,		/* output, init low. */
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
};

static struct gpio_rmc_data gpio_hntt1800ssc =
{
	.items = items_hntt1800ssc,
	.size = ARRAY_SIZE(items_hntt1800ssc),
};

static int __devinit gpio_channels_init(struct hndl_rmc_device *output)
{
	u8 offset = 0;
	u8 size = ARRAY_SIZE(items_hntt1800ssc);
	return rmc_gpio_register(output, &gpio_hntt1800ssc, offset, size);
}

/*led0~led5 -> outputreg[16]~outputreg[21]*/
/*beep -> outputreg[22]*/
static int smcbus_write(struct smcbus_rmc_data *bus, u32 bitmap, int is_set)
{
	bitmap = bitmap & 0xff;
	if(bitmap == 0x01){
		bus->smcbus_stat = is_set;
		return	beep_iomem->write_bit(beep_iomem, bitmap<<6, is_set);
	}
	if(bitmap == 0x00){
		bus->smcbus_stat = 0;
		return	beep_iomem->write_bit(beep_iomem, bitmap<<6, 0);
	}
	//printk("%s:This bitmap is not for beep!\n",  __FUNCTION__);
	return -EINVAL; 
}

static int smcbus_proc_read(struct smcbus_rmc_data *bus, char *buf)
{
	int len = 0;
	//int reslt = smcbus_read(bus);
	int reslt = bus->smcbus_stat;

	dprintk("%s: %x.\n", __FUNCTION__, reslt);
	len = sprintf(buf + len, "%4x\n", reslt);
	return len;
}

static int smcbus_proc_write(struct smcbus_rmc_data *bus, const char __user *userbuf, unsigned long count)
{
	u32 value = 0;
	char val[14] = {0};

	if (count >= 14){
		return -EINVAL;
	}

	if (copy_from_user(val, userbuf, count)){
		return -EFAULT;
	}

	value = (unsigned int)simple_strtoull(val, NULL, 16);
	bus->smcbus_stat = value;

	dprintk(KERN_INFO "\n%s:val=%s,after strtoull,value=0x%08x\n",
			__FUNCTION__, val, value);

	beep_iomem->write_byte(beep_iomem, value&0xff);

	return 0;

}

static void  smcbus_channels_unregister(struct hndl_rmc_device *output)
{
	rmc_smcbus_unregister(output, &smcbus_hntt1800ssc);

	iomem_object_put(beep_iomem);
	beep_iomem = NULL;

	return;
}

static int __devinit smcbus_channels_init(struct hndl_rmc_device *output)
{
	int result = 0;

	beep_iomem = iomem_object_get(HNTT1800SC_BEEP_CS, 0x01);
	if (!beep_iomem) {
		printk(KERN_ERR "Can NOT remap address %x\n", HNTT1800SC_BEEP_CS);
		result = -1;
		goto out;
	}

	//beep_iomem->write_byte(beep_iomem, 0xff);

	smcbus_hntt1800ssc.read = NULL;
	smcbus_hntt1800ssc.smcbus_stat = 0x01;
	smcbus_hntt1800ssc.write = smcbus_write;
	smcbus_hntt1800ssc.proc_read = smcbus_proc_read;
	smcbus_hntt1800ssc.proc_write = smcbus_proc_write;
	
	result = rmc_smcbus_register(output, &smcbus_hntt1800ssc, 
			OUTPUT_SMCBUS_OFFSET, OUTPUT_SMCBUS_SIZE);
	if (result < 0) {
		goto smcbus_failed;
	}
	
	return result;
smcbus_failed:	
	iomem_object_put(beep_iomem);
	beep_iomem = NULL;
out:
	return result;
}


static void  rmc_hntt1800ssc_remove(void)
{
	rmc_gpio_unregister(rmc_hntt1800ssc, &gpio_hntt1800ssc);
    
	smcbus_channels_unregister(rmc_hntt1800ssc);
	rmc_smcbus_refresh_stop(rmc_hntt1800ssc); 
    
	rmc_device_unregister(rmc_hntt1800ssc);
	rmc_device_free(rmc_hntt1800ssc);
	return;
}

static int __devinit rmc_hntt1800ssc_init(void)
{
	int ret = 0;

	rmc_hntt1800ssc = rmc_device_alloc();
	if (!rmc_hntt1800ssc) {
		return -1;
	}

	gpio_channels_init(rmc_hntt1800ssc);

	smcbus_channels_init(rmc_hntt1800ssc);
    
	ret = rmc_device_register(rmc_hntt1800ssc);
	if (0 == ret){
		ret = rmc_smcbus_refresh_start(rmc_hntt1800ssc);
	}

	return ret; 
}

module_init(rmc_hntt1800ssc_init);
module_exit(rmc_hntt1800ssc_remove);

MODULE_AUTHOR("zw");
MODULE_LICENSE("Dual BSD/GPL");
