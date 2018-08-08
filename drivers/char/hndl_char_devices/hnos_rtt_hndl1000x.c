/*
 *  linux/drivers/char/hndl_char/hnos_rtt_hndl1000x.c
 *
 *  Driver for the relay  on AT91SAM9260EK.
 *
 *  Author ZhangRM <peter_zrm@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "hnos_generic.h"

#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include <linux/hnos_debug.h>
#include <asm/arch/at91_rtt.h>


#define DEVICE_NAME		"rtt_time"
static int rtt_major =   0;
static int rtt_minor =   0;

struct hndl_rtt_cdev 
{
	spinlock_t lock;                 /* mutual exclusion lock*/
	unsigned long is_open;
	struct class *myclass;
	struct cdev cdev;	         /* Char device structure*/
};
static struct hndl_rtt_cdev *rtt_dev = NULL;

int rtt_open(struct inode *inode, struct file *filp);
int rtt_release(struct inode *inode, struct file *filp);
ssize_t rtt_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
ssize_t rtt_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos);
static int rtt_proc_read(struct proc_item *item, char *page);
static int rtt_proc_write(struct proc_item *item, const char __user * userbuf,
		unsigned long count);
		
static  struct proc_dir_entry	*hndl_proc_dir = NULL;
static struct proc_item items[] = 
{
	{
		.name = "rtt_time",
		.pin = 0,
		.read_func = rtt_proc_read,
		.write_func = rtt_proc_write,
	},
	{NULL},
};

struct file_operations hndl_rtt_fops =
{
    .owner =    THIS_MODULE,
    .read =     rtt_read,
    .write =    rtt_write,
    .open =     rtt_open,
    .release =  rtt_release,
};
static unsigned int rtt_value_get(void)
{
        unsigned int rtt_value =0;
        unsigned int rtt_value1 =0;
        int i;
	for(i =0;i<15;i++){
	        rtt_value = at91_sys_read(AT91_RTT_VR);
                rtt_value1 = at91_sys_read(AT91_RTT_VR);
                if(rtt_value == rtt_value1)
                        break;
        }
        if(i == 15)
             rtt_value = 0;
             
        return rtt_value;     
}

int rtt_open(struct inode *inode, struct file *filp)
{
    struct hndl_rtt_cdev *dev; 
	dev = container_of(inode->i_cdev, struct hndl_rtt_cdev, cdev);
    filp->private_data = dev; /* for other methods */
    if (test_and_set_bit(0,&dev->is_open)!=0) {
        return -EBUSY;       
    }
    return 0; /*success.*/
}
/*no test*/
int rtt_release(struct inode *inode, struct file *filp)
{
    struct hndl_rtt_cdev *dev = filp->private_data; 

    if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
        return -EINVAL; /* already released: error */
    }

    return 0;
}
/*no test*/
ssize_t rtt_read(struct file *filp, char __user *buf, size_t count,
        loff_t *f_pos)
{
    unsigned int rtt_value,len;
    unsigned long flags;
    struct hndl_rtt_cdev *dev = filp->private_data;
    
    spin_lock_irqsave(&dev->lock,flags);
    rtt_value = rtt_value_get();
    spin_unlock_irqrestore(&dev->lock,flags);
    
    len= sizeof(unsigned int);
    
    if (copy_to_user(buf,&rtt_value,len)){
        return (-EFAULT);
    }
    return len;   
}

ssize_t rtt_write(struct file *filp, const char __user *buf, size_t count,
        loff_t *f_pos)
{
    return count;
}




static int rtt_proc_read(struct proc_item *item, char *page)
{
        int len = 0;        
        unsigned long flags;
        
        spin_lock_irqsave(&rtt_dev->lock,flags);
	len= sprintf(page,"%d\n",rtt_value_get());
        spin_unlock_irqrestore(&rtt_dev->lock,flags);
        
	return len;
}

static int rtt_proc_write(struct proc_item *item, const char __user * userbuf,
		unsigned long count) 
{

	return 0;
}

static int __init  rtt_proc_devices_add(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = items; item->name; ++item) {
		ret += hnos_proc_entry_create(item);
	}

	return ret;
}
static int rtt_proc_devices_remove(void)
{
	struct proc_item *item;

	for (item = items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}

static void rtt_module_exit(void)
{
        dev_t devno = MKDEV(rtt_major, rtt_minor);    
        struct class *myclass;
                
        if (hndl_proc_dir) {
                rtt_proc_devices_remove();
                hnos_proc_rmdir();
        }
               
        if (rtt_dev != NULL){
                /* Get rid of our char dev entries */   
                cdev_del(&rtt_dev->cdev);    
        
                myclass = rtt_dev->myclass;
                if (myclass){
                        class_device_destroy(myclass, devno);
                        class_destroy(myclass);
                }
            
                kfree(rtt_dev);
                rtt_dev = NULL;
        }
            
                /* cleanup_module is never called if registering failed */
        unregister_chrdev_region(devno, 1);
        return;    

}

/* proc module init */
static int __init rtt_module_init(void)
{
	int result;
        dev_t dev = 0;
        struct class *myclass;
        
	at91_sys_write(AT91_RTT_MR, AT91_RTT_RTTRST | 0x8000);
	
        if (rtt_major){ 
                dev = MKDEV(rtt_major, rtt_minor);
                result = register_chrdev_region(dev, 1, DEVICE_NAME);
        } 
       else{ 
                result = alloc_chrdev_region(&dev, rtt_minor, 1, DEVICE_NAME);
                rtt_major = MAJOR(dev);
        }
        if (result < 0) {
                printk(KERN_WARNING "PTCT: can't get major %d\n", rtt_major);
                return result;
        }
        /************/        
               
        rtt_dev = kmalloc(sizeof(struct hndl_rtt_cdev), GFP_KERNEL);
        if (!rtt_dev) {
                pr_debug("RTT: can't alloc rtt_dev\n");
                result = -ENOMEM;
                goto fail;  /* Make this more graceful */
        }
        memset(rtt_dev, 0, sizeof(struct hndl_rtt_cdev)); 
           
        spin_lock_init(&rtt_dev->lock);
        
        /* Register a class_device in the sysfs. */
        myclass = class_create(THIS_MODULE, DEVICE_NAME);
        if (myclass == NULL) {
            pr_debug("RTT: can't creat class\n");
            result = -ENODEV;
            goto fail;
        }
        class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
        rtt_dev->myclass = myclass;
        cdev_init(&rtt_dev->cdev, &hndl_rtt_fops);
        rtt_dev->cdev.owner = THIS_MODULE;
        result = cdev_add(&rtt_dev->cdev, dev, 1);
        if (result) {
            printk(KERN_NOTICE "Error %d adding rtt device, major_%d.\n", result, MAJOR(dev));
            goto fail;
        }   

        
        hndl_proc_dir = hnos_proc_mkdir();
        if (!hndl_proc_dir) {
                result = -ENODEV;
                goto fail;
        } else {
                result = rtt_proc_devices_add();
                if (result) {
                        result = -ENODEV;
                        goto fail;
                }
        }

	HNOS_DEBUG_INFO("rtt_proc_module_init. ok \n");
	
	return 0;

fail:
        rtt_module_exit();
	HNOS_DEBUG_INFO("rtt_proc_module_init. fail\n");
	return result;
        
}

module_init(rtt_module_init);
module_exit(rtt_module_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

