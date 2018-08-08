/*
 *  drivers/char/hndl_char_devices/hnos_version.c
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
#include "hnos_version.h" 

extern char bstrap_ver[100];
extern char uboot_ver[100];

static const unsigned char version_string [] =
	 HNOS_PRODUCT " " HNOS_VERSION ", build " HNOS_RELEASE_DATE;
	 
/* Write only once! */
static unsigned char cramfs_version[150] = "cramfs v1.0.0 , build 20101023";
unsigned int write_protectd = 0;

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

#undef dprintk
#define dprintk		printk

static int proc_read_func(struct proc_item *item, char *buf)
{
	int len = 0;
	len += sprintf(buf + len, "bootstrap: %s\n", bstrap_ver);
	len += sprintf(buf + len, "u-boot   : %s\n", uboot_ver);
	len += sprintf(buf + len, "module   : %s\n", version_string);
	len += sprintf(buf + len, "rootfs   : %s\n", cramfs_version);
	return len;
}

int proc_write_func(struct proc_item *item, const char __user * userbuf, unsigned long count) 
{
	if (count >= 150){
		return -EINVAL;
	}

	if (0 == write_protectd) {
		write_protectd = 1;
		if (copy_from_user(cramfs_version, userbuf, count)) {
			return -EFAULT;
		}
	} else {
		return -EFAULT;
	}

	return 0;
}

static struct proc_item items[] = 
{
	{
		.name = "version", 
		.read_func = proc_read_func,
		.write_func = proc_write_func,
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

	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		status = -ENODEV;
	} else {
		status = proc_devices_add();
		if (status) {
			hnos_proc_rmdir();
		}
	}

	HNOS_DEBUG_INFO("Proc Filesystem Interface for HNOS Version Stuff init.\n");
	return (!status) ? 0 : -ENODEV;
}

static void proc_module_exit(void)
{
	proc_devices_remove();
	hnos_proc_rmdir();

	HNOS_DEBUG_INFO("Proc Filesystem Interface for HNOS Version Stuff exit.\n");
	return;
}

module_init(proc_module_init);
module_exit(proc_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

