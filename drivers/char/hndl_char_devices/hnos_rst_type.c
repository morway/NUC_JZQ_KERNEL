
/*
 *  drivers/char/hndl_char_devices/hnos_rst_type.c
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

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

static unsigned char *rst_reason[] = 
{
	"Both VDDCORE and VDDBU rising",
	"VDDCORE rising",
	"Watchdog fault occurred",
	"Processor reset required by the software",
	"NRST pin detected low",
};

static int rst_read_func(struct proc_item *item, char *buf)
{
	unsigned long data = 0;
	int len = 0;

	data = at91_sys_read(AT91_RSTC_SR);
	data = ( data >> 8 ) & 0x7;
	if (data < ARRAY_SIZE(rst_reason)) {
		len = sprintf(buf, "Last reset was caused by \"%s\".\n", rst_reason[data]);
	} else {
		len = sprintf(buf, "Last reset was caused by unknown reason.\n");
	}

	return len;
}

static struct proc_item items[] = 
{
	{
		.name = "reset_reason", 
		.read_func = rst_read_func,
	},
	{NULL}
};

static int __init  rst_devices_add(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = items; item->name; ++item) {
		ret += hnos_proc_entry_create(item);
	}
	return ret;
}

static int rst_devices_remove(void)
{
	struct proc_item *item;

	for (item = items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}


/* proc module init */
static int __init rst_module_init(void)
{
	int status;


	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		status = -ENODEV;
	} else {
		status = rst_devices_add();
		if (status) {
			hnos_proc_rmdir();
		}
	}

	HNOS_DEBUG_INFO("Proc Filesystem Interface for Reset Reason init.\n");
	return (!status) ? 0 : -ENODEV;
}

static void __exit rst_module_exit(void)
{
	rst_devices_remove();
	hnos_proc_rmdir();

	HNOS_DEBUG_INFO("Proc Filesystem Interface for Reset Reason exit.\n");
	return;
}

module_init(rst_module_init);
module_exit(rst_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

