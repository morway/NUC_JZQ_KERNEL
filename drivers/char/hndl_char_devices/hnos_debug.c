/*
 *  drivers/char/hndl_char_devices/hnos_debug.c
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

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

#undef dprintk
#define dprintk		printk

/* 
 * mr addr 
 * mw addr data
 * */
unsigned char local_buf[100] = {0};

static int mem_addr_valid(unsigned long phy_base)
{
	if (phy_base < 0x30000000 || phy_base > 0xfffffd60) {
		return -1;
	} else {
		return 0;
	}
}

static int inline mem_write(unsigned long phy_base, unsigned long data) 
{
	void *iomem = NULL;
	int is_reg = 0;

	if (mem_addr_valid(phy_base) != 0) {
		printk(KERN_ERR "%s: addr %ld invalid.\n", __FUNCTION__, phy_base);
	}

	if (phy_base > 0xf0000000) {
		is_reg = 1;
	}
	iomem = ioremap(phy_base, 4);
	if (!iomem) {
		printk(KERN_ERR "%s: ioremap %ld failed.\n", __FUNCTION__, phy_base);
		return  -1;
	}
	if (is_reg) {
		writel(data, iomem);
	} else {
		writeb(data, iomem);
	}
	iounmap(iomem);
	return 0;
}

static int inline mem_read(unsigned long phy_base, unsigned int *data) 
{
	void *iomem = NULL;
	int is_reg = 0;

	if (mem_addr_valid(phy_base) != 0) {
		printk(KERN_ERR "%s: addr %ld invalid.\n", __FUNCTION__, phy_base);
	}

	if (phy_base > 0xf0000000) {
		is_reg = 1;
	}

	iomem = ioremap(phy_base, 4);
	if (!iomem) {
		printk(KERN_ERR "%s: ioremap %ld failed.\n", __FUNCTION__, phy_base);
		return  -1;
	}

	if (is_reg) { /* AT91SAM9260 register is 32 bit. */
		*data = readl(iomem);
	} else {
		*data = readb(iomem);
	}

	iounmap(iomem);
	return 0;
}

static int inline mem16_write(unsigned long phy_base, unsigned long data) 
{
	void *iomem = NULL;
	iomem = ioremap(phy_base, 4);
	if (!iomem) {
		printk(KERN_ERR "%s: ioremap %ld failed.\n", __FUNCTION__, phy_base);
		return  -1;
	}
	writew(data, iomem);
	iounmap(iomem);
	return 0;
}

static int inline mem16_read(unsigned long phy_base, unsigned int *data) 
{
	void *iomem = NULL;

	iomem = ioremap(phy_base, 4);
	if (!iomem) {
		printk(KERN_ERR "%s: ioremap %ld failed.\n", __FUNCTION__, phy_base);
		return  -1;
	}

	*data = readw(iomem);

	iounmap(iomem);
	return 0;
}

static int inline mem16_debug(unsigned char *buf, unsigned char is_write, int count ) 
{
	unsigned int addr = 0;
	unsigned int data = 0;

	if (!buf || !count) {
		return -1;
	}

	if (is_write) {
		if (sscanf(buf, "wmw %x,%x ", &addr, &data) == 2) {
			dprintk("\n%s: write data %x to 0x%08x.\n", __FUNCTION__, (unsigned int)data, (unsigned int)addr);
			mem16_write(addr, data);
		}
	} else {
		if (sscanf(buf, "wmr %x\n", &addr) == 1) {
			if (mem16_read(addr, &data) == 0) {
				dprintk("\n%s: read data %x from 0x%08x.\n", __FUNCTION__, (unsigned int)data, (unsigned int) addr);
			}
		}

	}

	return 0;
}


/* 
 * iow PA8, data
 * ior PA8
 * */
static int gpio_debug(unsigned char *buf, unsigned char is_write, int count)
{
	int i = 0;
	unsigned char gpio_buf[5] = {0};
	unsigned int off = 0;
	char bank = 'A';
	unsigned int pin = AT91_PIN_PA0;
	unsigned char data_buf[5] = {0};
	unsigned int data = 0;

	char *s = strchr(buf, 'P');
	if (NULL == s) {
		return -1;
	}

	bank = *(s + 1);

	s = s + 2;
	while ( (*s != ',') && (!(isspace(*s))) && *s != '\0') {
		if (i >= sizeof(gpio_buf)) {
			break;
		}
		gpio_buf[i] = *s;
		i++;
		s++;
	}
	off = simple_strtoull(gpio_buf, NULL, 0);
	if (off > 31) {
		off = 31;
	}

	switch (bank) {
		case 'A':
			pin = AT91_PIN_PA0 + off;
			break;
		case 'B':
			pin = AT91_PIN_PB0 + off;
			break;
		case 'C':
			pin = AT91_PIN_PC0 + off;
			break;
		default:
			break;
	}

	if (is_write) {
		s = strchr(buf, ',');
		if (!s) {
			return -1;
		}
		s = s + 1;
		memcpy(data_buf, s, sizeof(data_buf));
		data = simple_strtoull(data_buf, NULL, 0);
		dprintk("\n%s: write %d to gpio P%c%d, pin %d.\n", __FUNCTION__, data, bank, off, pin);
		at91_set_gpio_output(pin, data);
	} else {
		data = at91_get_gpio_value(pin);
		dprintk("\n%s: read %d from gpio P%c%d, pin %d.\n", __FUNCTION__, data, bank, off, pin);
	}

	return 0;
} 

static int inline mem_debug(unsigned char *buf, unsigned char is_write, int count ) 
{
	unsigned int addr = 0;
	unsigned int data = 0;

	if (!buf || !count) {
		return -1;
	}

	if (is_write) {
		if (sscanf(buf, "mw %x,%x ", &addr, &data) == 2) {
			dprintk("\n%s: write data %x to 0x%08x.\n", __FUNCTION__, (unsigned int)data, (unsigned int)addr);
			mem_write(addr, data);
		}
	} else {
		if (sscanf(buf, "mr %x\n", &addr) == 1) {
			if (mem_read(addr, &data) == 0) {
				dprintk("\n%s: read data %x from 0x%08x.\n", __FUNCTION__, (unsigned int)data, (unsigned int) addr);
			}
		}

	}

	return 0;
}


int proc_debug_func(struct proc_item *item, const char *buf, unsigned long count)
{

	if (count >= 100){
		return -EINVAL;
	}

	if (copy_from_user(local_buf, buf, count)){
		return -EFAULT;
	}

	//dprintk(KERN_INFO "%s: cmd \"%s\"", __FUNCTION__, local_buf);

	if (strncmp(local_buf, "mr", 2) == 0) {
		return mem_debug(local_buf, 0, sizeof(local_buf));
	} else if (strncmp(local_buf, "mw", 2) == 0) {
		return mem_debug(local_buf, 1, sizeof(local_buf));
	} else if (strncmp(local_buf, "ior", 3) == 0) {
		return gpio_debug(local_buf, 0, sizeof(local_buf));
	} else if (strncmp(local_buf, "iow", 3) == 0) {
		return gpio_debug(local_buf, 1, sizeof(local_buf));
	} else if (strncmp(local_buf, "wmr", 3) == 0) {
		return mem16_debug(local_buf, 0, sizeof(local_buf));
	} else if (strncmp(local_buf, "wmw", 3) == 0) {
		return mem16_debug(local_buf, 1, sizeof(local_buf));
	}

	return 0;
}

static int proc_read_func(struct proc_item *item, char *buf)
{
	int len = 0;
	len = sprintf(buf, "last cmd: %s\n", local_buf);
	return len;
}

static struct proc_item items[] = 
{
	{
		.name = "hnos_debug", 
		.read_func = proc_read_func,
		.write_func = proc_debug_func,
	},
	{NULL}
};

static int __init  proc_devices_add(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = items; item->name; ++item) {
		ret += hnos_proc_entry_create(item);
	}
	return ret;
}

static int proc_devices_remove(void)
{
	struct proc_item *item;

	for (item = items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}


/* proc module init */
static int __init proc_module_init(void)
{
	int status;

	HNOS_DEBUG_INFO("Proc Filesystem Interface for Debug.\n");

	hndl_proc_dir = hnos_proc_mkdir();
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

