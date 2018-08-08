/*
 *  drivers/char/hndl_char_devices/hnos_proc_intf.c
 *
 *  proc file system interface. 
 *
 *	   zrm,eter_zrm@163.com
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

static  struct proc_dir_entry	  *hnos_proc_root;
static  atomic_t                  ref_cnt;

struct proc_dir_entry *hnos_proc_mkdir(void)
{
    if ((atomic_inc_return(&ref_cnt) == 1)
            && !hnos_proc_root) {
        hnos_proc_root = proc_mkdir(HNOS_PROC_ROOT, NULL);
    }
    if (!hnos_proc_root) {
        printk(KERN_ERR "%s: can't create /proc/%s.\n", __FUNCTION__, HNOS_PROC_ROOT);
        return NULL;
    }
    return hnos_proc_root;
}
EXPORT_SYMBOL(hnos_proc_mkdir);


void hnos_proc_rmdir(void)
{
    if (atomic_dec_and_test(&ref_cnt) && hnos_proc_root) {
        remove_proc_entry(HNOS_PROC_ROOT, NULL);
        hnos_proc_root = NULL;
    }

    return;
}
EXPORT_SYMBOL(hnos_proc_rmdir);
#if 0

int hnos_proc_dispatch_read(char *page, char **start, 
        off_t off, int count, int *eof, void *data)
{
    struct proc_item *item = (struct proc_item *)data;
    int len;

    if (!item ||!item->read_func) {
        return -EINVAL;
    }

    len = item->read_func(item, page);
    if (len < 0)
        return len;

    if (len <= off + count)
        *eof = 1;
    *start = page + off;
    len -= off;
    if (len > count)
        len = count;
    if (len < 0)
        len = 0;

    return len;
}

int hnos_proc_dispatch_write(struct file *file, const char __user * userbuf,
        unsigned long count, void *data)
{
    struct proc_item *item = (struct proc_item *)data;
    int ret;

    if (!item || !item->write_func) {
        return -EINVAL;
    }

    ret = item->write_func(item,  userbuf, count);	
    if (ret == 0) {
        ret = count;
    }

    return ret;
}

int hnos_proc_entry_create(struct proc_item *item)
{
    struct proc_dir_entry *proc;
    proc = create_proc_read_entry(item->name,
            S_IFREG | S_IRUGO | S_IWUSR,
            hnos_proc_root,
            hnos_proc_dispatch_read,
            item);
    if (proc) {
        proc->owner = THIS_MODULE;
    } else {
        return -1;
    }

    if (proc && item->read_func) {
        proc->read_proc = (read_proc_t *) hnos_proc_dispatch_read;
    }

    if (proc && item->write_func) {
        proc->write_proc = (write_proc_t *) hnos_proc_dispatch_write;
    }

    return 0;
}
EXPORT_SYMBOL(hnos_proc_entry_create);

int hnos_proc_items_create(struct proc_item *items, size_t size)
{
    int    ret = 0, i = 0;

    if (!items || size == 0) {
        printk(KERN_ERR "%s: invalide params.\n", __FUNCTION__);
        return -1;
    }

    hnos_proc_mkdir();

    for (i=0; i<size; i++) {
        if (!items[i].name) {
            continue;
        }

        if (items[i].pin) {
            if (items[i].settings) {
                hnos_gpio_cfg(items[i].pin, items[i].settings);
            }

            /* If read or write function is not set, use default. */
            if (!items[i].read_func) {
                items[i].read_func = hnos_proc_gpio_get;
            }

            if (!items[i].write_func && (items[i].settings & GPIO_OUTPUT_MASK)) {
                items[i].write_func = hnos_proc_gpio_set;
            }
        }

        ret += hnos_proc_entry_create(&items[i]);
    }

    return ret;
}
EXPORT_SYMBOL(hnos_proc_items_create);
#endif

int hnos_proc_items_remove(struct proc_item *items, size_t size)
{
    int i = 0;

    if (!items || size == 0) {
        printk(KERN_ERR "%s: invalide params.\n", __FUNCTION__);
        return -1;
    }

    for (i=0; i<size; i++) {
        if (!items[i].name) {
            continue;
        }

        remove_proc_entry(items[i].name, hnos_proc_root);
    }

    hnos_proc_rmdir();
    return 0;
}

EXPORT_SYMBOL(hnos_proc_items_remove);

MODULE_LICENSE("Dual BSD/GPL");

