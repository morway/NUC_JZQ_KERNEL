/*
 * arch/arm/mach-at91/hndl_product_hndl1000x.c
 * Author ZhangRM, peter_zrm@163.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include "hnos_generic.h"
#include "hnos_proc.h" 
#include "hnos_prog.h" 
#include "hnos_ioctl.h" 
#include "hnos_gpio.h" 


static int product_proc_read(struct proc_item *item, char *page);

static  struct proc_dir_entry	*hndl_proc_dir = NULL;
static struct proc_item items[] = 
{
	{
		.name = "product_info",
		.pin = 0,
		.read_func = product_proc_read,
	},
	{NULL},
};

static struct product_info hndl1000x_info[] =
{
        {
        .product_id = PRODUCT_HNDL1000X_CQ,
        .product= "HNDL1000X_CQ", 
        .measure = "att7022",
        .voice = "none",
        .lcd = "uc1698fb",
        },
        {
        .product_id = PRODUCT_HNDL1000X_LN,
        .product= "HNDL1000X_LN", 
        .measure = "measure6513",
        .voice = "ap89170",
        .lcd = "uc1698fb",
        },
};

static int productinfo_operations(char *page,struct product_info *productinfo)
{
        int len =0;

        len += sprintf(page+len, "Product_id\t: %d\n", productinfo->product_id);
        len += sprintf(page+len, "Product\t\t: %s\n", productinfo->product);
        len += sprintf(page+len, "Measure\t\t: %s\n", productinfo->measure);
        len += sprintf(page+len, "Voice\t\t: %s\n", productinfo->voice);
        len += sprintf(page+len, "Lcd\t\t: %s\n", productinfo->lcd);

        return len;
}

static int product_info_get(char *page)
{
	int i;
	int len=0;
	
	for (i=0; i<ARRAY_SIZE(hndl1000x_info); i++){
                if (ID_MATCHED == hndl1000x_id_match(hndl1000x_info[i].product_id)){
                        struct product_info *productinfo = &hndl1000x_info[i];
                        len = productinfo_operations(page,productinfo);
                        break;
                }        
	} 
	
	return len;
}


static int product_proc_read(struct proc_item *item, char *page)
{
	char my_buffer[512];
	char *buf;
	int len;
	
	buf = my_buffer;

	len = product_info_get(buf);
	
	strncpy(page,my_buffer,len);
	return len;
}


static int __init  product_proc_devices_add(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = items; item->name; ++item) {
		ret += hnos_proc_entry_create(item);
	}

	return ret;
}
static int product_proc_devices_remove(void)
{
	struct proc_item *item;

	for (item = items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}

static void product_module_exit(void)
{
	if (hndl_proc_dir) {
		product_proc_devices_remove();
		hnos_proc_rmdir();
	}

	return;    

}

/* proc module init */
static int __init product_module_init(void)
{
	int result;

	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		result = -ENODEV;
		goto fail;
	} else {
		result = product_proc_devices_add();
		if (result) {
			result = -ENODEV;
			goto fail;
		}
	}

	HNOS_DEBUG_INFO("product_module_init. ok \n");

	return 0;

fail:
	product_module_exit();
	HNOS_DEBUG_INFO("product_module_init. fail\n");
	return result;

}

module_init(product_module_init);
module_exit(product_module_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
