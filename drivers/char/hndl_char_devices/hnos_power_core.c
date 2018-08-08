/*
 *  linux/drivers/char/hndl_char/devices/hnos_power_core.c
 *
 *  Driver for the power  on AT91SAM9260EK.
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
#include "hnos_power_core.h"
#include "hnos_proc.h" 
#include "hnos_bat_adc.h"
#include "hnos_ioctl.h"                /* ioctl */

static struct at91_power_cdev *power_device = NULL;
static int power_major =   0;
static int power_minor =   0;
int power_open(struct inode *inode, struct file *filp);
int power_release(struct inode *inode, struct file *filp);
static int power_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg);

struct file_operations at91_power_fops =
{
	.owner =    THIS_MODULE,
	.ioctl =    power_ioctl,
	.open =     power_open,
	.release =  power_release,
};

static int power_state_generic(void)
{
    return SCAN_POWER_ON;
}

static int v6513_state_generic(void)
{
    return SCAN_POWER_HIGH;
}

static int vcc5v_state_generic(void)
{
    return SCAN_POWER_HIGH;
}

static int vcc5v_adc_generic(void)
{
    return ADC_RESLT_INVALID;

}

int power_open(struct inode *inode, struct file *filp)
{
	struct at91_power_cdev *dev; 
	dev = container_of(inode->i_cdev, struct at91_power_cdev, cdev);
	filp->private_data = dev; /* for other methods */
	if (test_and_set_bit(0,&dev->is_open)!=0) {
		return -EBUSY;       
	}
	return 0; /*success.*/
}
int power_release(struct inode *inode, struct file *filp)
{
	struct at91_power_cdev *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}

static int power_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0; 

	if (!power_device) {
		printk("%s: no power_device.\n", __FUNCTION__);
		return -ENODEV;
	}
	
	if(!power_device->power_cbs){
		printk("%s: no power_device->power_cbs.\n", __FUNCTION__);
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
        case IOC_POWER_STATE:	/* LED on.*/
            ret = power_device->power_cbs->power_state();
            break;
        case IOC_V6513_STATE:	/* LED on.*/
            ret = power_device->power_cbs->v6513_state();
        case IOC_VCC5V_STATE:	/* LED on.*/
            ret = power_device->power_cbs->vcc5v_state();
        case IOC_VCC5V_ADC:	/* LED on.*/
            ret = power_device->power_cbs->vcc5v_adc();
        default:  /* redundant, as cmd was checked against MAXNR */
            ret = -ENOTTY;
            break;
    }
    
    return ret;
}

int power_module_unregister(struct at91_power_callbacks *power)
{
	if ((!power_device) || (!power_device->power_cbs)) {
		printk(KERN_ERR "%s: No power devices found.\n", __FUNCTION__);
		return -ENODEV;
	}
    power_device->power_cbs->power_state= NULL;
    power_device->power_cbs->v6513_state= NULL;
    power_device->power_cbs->vcc5v_state= NULL;
    power_device->power_cbs->vcc5v_adc = NULL;
    power_device->power_cbs=NULL;
    
	HNOS_DEBUG_INFO("%s\n",__FUNCTION__);
	return 0;
}


int power_module_register(struct at91_power_callbacks *power)
{
   
	if (!power_device) {
		printk(KERN_ERR "%s: No power devices found.\n", __FUNCTION__);
		return -1;
	}

    if(power_device->power_cbs != NULL){
		printk(KERN_ERR "%s:  power_cbs devices had defined.\n", __FUNCTION__);
		return -1;
	}
        
	if(power->power_state== NULL);
	    power->power_state= power_state_generic;
	if(power->v6513_state== NULL);
	    power->v6513_state= v6513_state_generic;
	if(power->vcc5v_state== NULL);
	    power->vcc5v_state= vcc5v_state_generic;
	if(power->vcc5v_adc== NULL);
	    power->vcc5v_adc= vcc5v_adc_generic;
	    
	power_device->power_cbs = power;

	HNOS_DEBUG_INFO("%s\n",__FUNCTION__);
	return 0;
	
}

static void power_module_exit(void)
{
	dev_t devno = MKDEV(power_major, power_minor);    
	struct class *myclass;
    
	if (power_device != NULL){
		/* Get rid of our char dev entries */   
		cdev_del(&power_device->cdev);    

		myclass = power_device->myclass;
		if (myclass){
			class_device_destroy(myclass, devno);
			class_destroy(myclass);
		}

		kfree(power_device);
		power_device = NULL;
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);
	return;    

}

static int __init power_module_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *myclass;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (power_major){ 

		dev = MKDEV(power_major, power_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} 
	else{ 

		result = alloc_chrdev_region(&dev, power_minor, 1, DEVICE_NAME);
		power_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "PTCT: can't get major %d\n", power_major);
		return result;
	}
	/************/        

	power_device = kmalloc(sizeof(struct at91_power_cdev), GFP_KERNEL);
	if (!power_device) {
		printk(KERN_WARNING "PTCT: can't alloc power_device\n");
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(power_device, 0, sizeof(struct at91_power_cdev)); 


    power_device->power_cbs=NULL;
    
	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if (myclass == NULL) {
		printk(KERN_WARNING "PTCT: can't creat class\n");
		result = -ENODEV;
		goto fail;
	}
	class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	power_device->myclass = myclass;
	cdev_init(&power_device->cdev, &at91_power_fops);
	power_device->cdev.owner = THIS_MODULE;
	result = cdev_add(&power_device->cdev, dev, 1);
	if (result) {
		printk(KERN_NOTICE "Error %d adding power device, major_%d.\n", result, MAJOR(dev));
		goto fail;
	}   

	HNOS_DEBUG_INFO("Initialized device %s, major %d. \n", DEVICE_NAME, power_major);
	return 0;
fail:
	power_module_exit();
	return result;

}


EXPORT_SYMBOL(power_module_register);
EXPORT_SYMBOL(power_module_unregister);

module_init(power_module_init);
module_exit(power_module_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
