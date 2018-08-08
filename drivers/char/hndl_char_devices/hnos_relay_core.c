/*
 *  linux/drivers/char/hndl_char/devices/hnos_relay_core.c
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
#include "hnos_relay_core.h"
#include "hnos_proc.h" 
#include "hnos_ioctl.h" 

static struct at91_relay_cdev *relay_device = NULL;
static int relay_major =   0;
static int relay_minor =   0;
int relay_open(struct inode *inode, struct file *filp);
int relay_release(struct inode *inode, struct file *filp);
static int relay_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg);

struct file_operations at91_relay_fops =
{
	.owner =    THIS_MODULE,
	.ioctl =    relay_ioctl,
	.open =     relay_open,
	.release =  relay_release,
};

static int relay_led_generic(int cmd, long id)
{
    return 0;
}

static int relay_opt_generic (int cmd,long action)
{
    return 0;
}

int relay_open(struct inode *inode, struct file *filp)
{
	struct at91_relay_cdev *dev; 
	dev = container_of(inode->i_cdev, struct at91_relay_cdev, cdev);
	filp->private_data = dev; /* for other methods */
	if (test_and_set_bit(0,&dev->is_open)!=0) {
		return -EBUSY;       
	}
	return 0; /*success.*/
}
int relay_release(struct inode *inode, struct file *filp)
{
	struct at91_relay_cdev *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}

static int relay_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0; 

	if (!relay_device) {
		printk("%s: no relay_device.\n", __FUNCTION__);
		return -ENODEV;
	}
	
	if(!relay_device->relay_cbs){
		printk("%s: no relay_device->relay_cbs.\n", __FUNCTION__);
		return -ENODEV;
    } 

     /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC) {
                return -ENOTTY;
    }
    if (_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR ) {
                return -ENOTTY;
    }

    if (_IOC_DIR(cmd) & _IOC_READ) {
                ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    } 
    else if (_IOC_DIR(cmd) & _IOC_WRITE) {
                ret =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
        }

    if (ret) {
        return -EFAULT;
    }

    dprintk("%s: cmd %d , led %2d \n", __FUNCTION__, cmd, (int)arg);

    switch(cmd) {
        case IOC_LED_ON:	/* LED on.*/
        case IOC_LEDS_ON:	/* LED on.*/
        case IOC_LED_OFF:	/* LED on.*/
        case IOC_LEDS_OFF:	/* LED on.*/
            ret = relay_device->relay_cbs->relay_led(cmd,arg);
            break;
		case IOC_RELAY_START:
		case IOC_RELAY_POWER:
		case IOC_RELAY_OPEN:
		case IOC_RELAY_CLOSE:
            ret =relay_device->relay_cbs->relay_opt(cmd,arg);
            break;
        default:  /* redundant, as cmd was checked against MAXNR */
            ret = -ENOTTY;
            break;
    }
    
    return ret;
}
int relay_module_unregister(struct at91_relay_callbacks *relay)
{
	
	if( (!relay_device) ||(!relay_device->relay_cbs)) {
		printk(KERN_ERR "%s: No relay devices found.\n", __FUNCTION__);
		return -ENODEV;
	}
	
    relay_device->relay_cbs->relay_led = NULL;
    relay_device->relay_cbs->relay_opt= NULL;
    
	HNOS_DEBUG_INFO("%s\n",__FUNCTION__);
	return 0;
}


int relay_module_register(struct at91_relay_callbacks *relay)
{
    
	if (!relay_device) {
		printk(KERN_ERR "%s: No relay devices found.\n", __FUNCTION__);
		return -1;
	}
	if(relay_device->relay_cbs != NULL)
	    return -1;

	if(relay->relay_led == NULL);
	    relay->relay_led = relay_led_generic;
	if(relay->relay_opt== NULL);
	    relay->relay_opt = relay_opt_generic;
	    
	relay_device->relay_cbs = relay;

	HNOS_DEBUG_INFO("%s\n",__FUNCTION__);
	return 0;
	
}

static void relay_module_exit(void)
{
	dev_t devno = MKDEV(relay_major, relay_minor);    
	struct class *myclass;

	if (relay_device != NULL){
		/* Get rid of our char dev entries */   
		cdev_del(&relay_device->cdev);    

		myclass = relay_device->myclass;
		if (myclass){
			class_device_destroy(myclass, devno);
			class_destroy(myclass);
		}
        relay_device->relay_cbs=NULL;
		kfree(relay_device);
		relay_device = NULL;
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);
	return;    

}

static int __init relay_module_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *myclass;

	if (relay_major){ 
		dev = MKDEV(relay_major, relay_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} 
	else{ 
		result = alloc_chrdev_region(&dev, relay_minor, 1, DEVICE_NAME);
		relay_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "Relay: can't get major %d\n", relay_major);
		return result;
	}

	relay_device = kmalloc(sizeof(struct at91_relay_cdev), GFP_KERNEL);
	if (!relay_device) {
		printk(KERN_WARNING "Relay: can't alloc relay_device\n");
		result = -ENOMEM;
		goto fail;  
	}
	memset(relay_device, 0, sizeof(struct at91_relay_cdev)); 


    relay_device->relay_cbs=NULL;
    
	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if (myclass == NULL) {
		printk(KERN_WARNING "Relay: can't creat class\n");
		result = -ENODEV;
		goto fail;
	}
	class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	relay_device->myclass = myclass;
	cdev_init(&relay_device->cdev, &at91_relay_fops);
	relay_device->cdev.owner = THIS_MODULE;
	result = cdev_add(&relay_device->cdev, dev, 1);
	if (result) {
		printk(KERN_NOTICE "Error %d adding relay device, major_%d.\n", result, MAJOR(dev));
		goto fail;
	}   

	HNOS_DEBUG_INFO("Initialized device %s, major %d. \n", DEVICE_NAME, relay_major);
	return 0;
fail:
	relay_module_exit();
	return result;

}


EXPORT_SYMBOL(relay_module_register);
EXPORT_SYMBOL(relay_module_unregister);

module_init(relay_module_init);
module_exit(relay_module_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
