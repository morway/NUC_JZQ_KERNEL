/*
 * drivers/char/hndl_char_devices/hnos_commu_core.c
 *
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include "hnos_generic.h"
#include "hnos_commu.h"
#include "hnos_ioctl.h"
#include "hnos_proc.h"

#define DEVICE_NAME	"commu_module"

struct at91_commu_cdev  *commu_device;
static int commu_major =   0;
static int commu_minor =   0;
module_param(commu_major, int, S_IRUGO);
module_param(commu_minor, int, S_IRUGO);

static int commu_module_open(struct inode *inode, struct file *filp);
static int commu_module_release(struct inode *inode, struct file *filp);
static int commu_module_ioctl (struct inode *inode, struct file *filp,
				unsigned int cmd, unsigned long arg);

struct file_operations commu_fops =
{
	.owner =    THIS_MODULE,
	.open =     commu_module_open,
	.release =  commu_module_release,
	.ioctl =    commu_module_ioctl,
};

static int commu_module_open(struct inode *inode, struct file *filp)
{
	struct at91_commu_cdev *dev; 

	dev = container_of(inode->i_cdev, struct at91_commu_cdev, cdev);
	filp->private_data = dev; /* for other methods */

	if(test_and_set_bit(0, &dev->is_open) != 0) 
	{
		return -EBUSY;       
	}

	return 0; 
}

static int commu_module_release(struct inode *inode, struct file *filp)
{
	struct at91_commu_cdev *dev = filp->private_data; 

	if(test_and_clear_bit(0, &dev->is_open) == 0) 
	{/* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}

/* Caller must hold the lock.*/
static int commu_type_generic(void)
{
	return 0xff;
}


/* Caller must hold the lock.*/
static int commu_start_generic(struct at91_commu_object *commu, int level)
{
	unsigned pin = commu->start_pin;
	
	if(!pin) 
	{
		printk("%s: commu->start_pin not initialiazed.\n", __FUNCTION__);
		return -ENODEV;
	}

	dprintk("%s\n", __FUNCTION__);

	if(level == OUTPUT_LEVEL_HIGH)
	{
		//printk("[commu_start_generic]%s: pin=%d,OUTPUT_LEVEL_HIGH.\n", __FUNCTION__,pin - PIN_BASE);
		at91_set_gpio_value(pin, 0);	
	} 
	else if(level == OUTPUT_LEVEL_LOW)
	{
		//printk("[commu_start_generic]%s: pin=%d,OUTPUT_LEVEL_LOW.\n", __FUNCTION__,pin - PIN_BASE);
		at91_set_gpio_value(pin, 1);
	}

	return 0;
}

/* Caller must hold the lock.*/
static int commu_reset_generic(struct at91_commu_object *commu, int level)
{
	/* 
	 * Note: 
	 * For NetMeter Beijing which use MC39i, RESET pin is needed.
	 * For NetMeter Tianjin which use ZTE CDMA/GPRS, RESET pin is not necessary.
	 * */
	int value = 0;
	unsigned pin = commu->reset_pin;

	if(pin) 
	{
		if(level == OUTPUT_LEVEL_HIGH)
		{
			value = 1;
		} 
		else if(level == OUTPUT_LEVEL_LOW) 
		{
			value = 0;
		}

		//printk("[commu_reset_generic]%s: pin=%d,value=%d.\n", __FUNCTION__, pin-PIN_BASE, value);
		at91_set_gpio_value(pin, value);
	}

	return 0;
}

/* Caller must hold the lock.*/
static int commu_power_generic(struct at91_commu_object *commu, enum EPower power)
{
	unsigned pin = commu->power_pin;
	if(!pin) 
	{
		printk("%s: commu->power_pin not initialiazed.\n", __FUNCTION__);
		return -ENODEV;
	}

	dprintk("%s\n", __FUNCTION__);

	if(power == ePowerON) 
	{
		//printk("[commu_power_generic]%s: power == ePowerON,pin=%d.\n", __FUNCTION__, pin-PIN_BASE);
		at91_set_gpio_value(pin, 1);	
	} 
	else if(power == ePowerOFF) 
	{
		//printk("[commu_power_generic]%s: power == ePowerOFF,pin=%d.\n", __FUNCTION__, pin-PIN_BASE);
		at91_set_gpio_value(pin, 0);	
	}

	return 0;
}

static int commu_type_get(struct at91_commu_object *commu)
{
	int type = 0;

	down(&commu_device->lock);
	if(commu && commu->commu_type) 
	{
		type = commu->commu_type();
	} 
	else 
	{
		type = commu_type_generic();
	}
	up(&commu_device->lock);

    dprintk("%s: comm type %d.\n", __FUNCTION__, type);

	return type;
}

static int commu_module_start(struct at91_commu_object *commu, int level)
{
	int ret = 0;

	down(&commu_device->lock);
	if(commu && commu->start) 
	{
		ret = commu->start(commu, level);
	} 
	else 
	{
		ret = commu_start_generic(commu, level);
	}
	up(&commu_device->lock);

	dprintk("%s\n", __FUNCTION__);
	return ret;
}

static int commu_module_reset(struct at91_commu_object *commu, int level)
{
	int ret = 0;

	down(&commu_device->lock);
	if(commu && commu->reset) 
	{
		ret = commu->reset(commu, level);
	} 
	else 
	{
		ret = commu_reset_generic(commu, level);
	}
	up(&commu_device->lock);
	dprintk("%s\n", __FUNCTION__);
	return ret;
}

static int commu_module_power(struct at91_commu_object *commu, enum EPower power)
{
	int ret = 0;

	down(&commu_device->lock);
	if(commu->power) 
	{
		ret = commu->power(commu, power);
	} 
	else 
	{
		ret = commu_power_generic(commu, power);
	}
	up(&commu_device->lock);

	dprintk("%s\n", __FUNCTION__);
	return ret;
}

/*The ioctl() implementation */
static int commu_module_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	unsigned char  value = 0;
	int level = OUTPUT_LEVEL_LOW;

	struct at91_commu_cdev *dev = filp->private_data; /* device information */
	struct at91_commu_object *commu = dev->commu; 
	if(!dev || !commu) 
	{
		printk("%s: no such device.\n", __FUNCTION__);
		return -ENODEV;
	}

	/* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
	if(_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC) 
	{
		return -ENOTTY;
	}
	if(_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR ) 
	{
		return -ENOTTY;
	}

	// dprintk("%s: cmd %02x ", __FUNCTION__, cmd);
	switch(cmd) 
	{
		case IOC_COMMU_MODULES_TYPE:	/* communication modules type.*/
			dprintk("IOC_COMMU_MODULES_TYPE.\n");

			value = commu_type_get(commu);
			if(put_user(value, (char __user *) arg)) 
			{
				return -EFAULT;
			}

			break;

		case IOC_COMMU_MODULES_RESET:	/* communication modules reset.*/
			dprintk("IOC_COMMU_MODULES_RESET.\n");

			level = arg;
			if((level != OUTPUT_LEVEL_LOW) && (level != OUTPUT_LEVEL_HIGH))
			{
				return -EFAULT;
			}

			commu_module_reset(commu, level);
			break;

		case IOC_COMMU_MODULES_START:	/* communication modules start.*/
			dprintk("IOC_COMMU_MODULES_START.\n");

			level = arg;
			if((level != OUTPUT_LEVEL_LOW) && (level != OUTPUT_LEVEL_HIGH))
			{
				return -EFAULT;
			}

			commu_module_start(commu, level);

			break;

		case IOC_COMMU_MODULES_POWERON: /* communication modules power on.*/
			dprintk("IOC_COMMU_MODULES_POWERON.\n");
			commu_module_power(commu, ePowerON);
			break;

		case IOC_COMMU_MODULES_POWEROFF: /* communication modules power off.*/
			dprintk("IOC_COMMU_MODULES_POWEROFF.\n");
			commu_module_power(commu, ePowerOFF);
			break;

		default:  
			return -ENOTTY;
	}

	return 0;
}

int commu_module_register(struct at91_commu_object *commu)
{
	if(!commu_device) 
	{
		printk(KERN_ERR "%s: No commu devices found.\n", __FUNCTION__);
		return -ENODEV;
	}

	if(commu_device->commu) 
	{
		printk(KERN_ERR "%s: Another commu object registered.\n", __FUNCTION__);
		return -EBUSY;
	}

	down(&commu_device->lock);
	commu_device->commu = commu;
	up(&commu_device->lock);

	HNOS_DEBUG_INFO("A commu object registered.\n");
	return 0;
}

int commu_module_unregister(struct at91_commu_object *commu)
{
	if(!commu_device) 
	{
		printk(KERN_ERR "%s: No commu devices found.\n", __FUNCTION__);
		return -ENODEV;
	}

	down(&commu_device->lock);
	commu_device->commu = NULL;
	up(&commu_device->lock);

	HNOS_DEBUG_INFO("Unregitered the commu object.\n");
	return 0;
}

static void  commu_cdev_setup(struct at91_commu_cdev *dev, dev_t devno)
{
	int err;

	cdev_init(&dev->cdev, &commu_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&dev->cdev, devno, 1);

	if(err) 
	{ /* Fail gracefully if need be */
		printk(KERN_NOTICE "Error %d adding hndl_kb_%d", err, MAJOR(devno));
	}
	return;
}

static int commu_proc_read(struct proc_item *item, char *page)
{
	unsigned int len = 0;
    unsigned int tmp = 0;
	struct at91_commu_object *commu = commu_device->commu; 

    if(!commu || !item || !page || !item->name) 
	{
        printk(KERN_ERR "%s: invalid params.\n", __FUNCTION__);
    }

    if(strcmp(item->name, "commu_type") == 0) 
	{
        tmp = commu_type_get(commu);
        len = sprintf(page, "commu_type :\t%d\n", tmp);
        goto out;

    } 
	else if(strcmp(item->name, "commu_reset") == 0) 
	{
        if (commu->reset_pin) 
		{
            tmp = at91_get_gpio_value(commu->reset_pin);
        } 
		else 
		{
            tmp = 0xff;
        }
        len = sprintf(page, "commu_reset:\t%d\n", tmp);
        goto out;

    } 
	else if(strcmp(item->name, "commu_output") == 0) 
	{
        if(commu->start_pin) 
		{
            tmp = at91_get_gpio_value(commu->start_pin);
        } 
		else 
		{
            tmp = 0xff;
        }
        len = sprintf(page, "commu_start:\t%d\n", tmp);
        goto out;

    } 
	else if(strcmp(item->name, "commu_power") == 0) 
	{
        if(commu->power_pin) 
		{
            tmp = at91_get_gpio_value(commu->power_pin);
        } 
		else 
		{
            tmp = 0xff;
        }
        len = sprintf(page, "commu_power:\t%d\n", tmp);
        goto out;
    }

out:
	return len;
}

static int commu_proc_write(struct proc_item *item, const char __user * userbuf,
                            unsigned long count) 
{
	struct at91_commu_object *commu = commu_device->commu; 
    unsigned int value = 0;
    char val[12] = {0};

    if(!commu || !item || !userbuf || !item->name) 
	{
        printk(KERN_ERR "%s: invalid params.\n", __FUNCTION__);
    }

    if(count >= 11)
	{
        return -EINVAL;
    }

    if(copy_from_user(val, userbuf, count))
	{
        return -EFAULT;
    }

    value = (unsigned int)simple_strtoull(val, NULL, 0);

    dprintk(KERN_INFO "%s:val=%s,after strtoull,value=0x%08x\n",
            __FUNCTION__, val, value);


    if(strcmp(item->name, "commu_reset") == 0) 
	{
        commu_module_reset(commu, value);
        goto out;

    } 
	else if(strcmp(item->name, "commu_output") == 0) 
   	{
       commu_module_start(commu, value);
       goto out;

    } 
	else if(strcmp(item->name, "commu_power") == 0) 
	{
       commu_module_power(commu, value);
       goto out;
    }

out:
	return 0;
}



static struct proc_item commu_items[] = 
{
	[0] = {
		.name = "commu_type", 
	},

	[1] = {
		.name = "commu_reset", 
	},

	[2] = {
		.name = "commu_output", 
	},

	[3] = {
		.name = "commu_power", 
	},	
};


/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
void commu_module_cleanup(void)
{
	dev_t devno = MKDEV(commu_major, commu_minor);	
	struct class * class;

	if(commu_device)
	{
        hnos_proc_items_remove(commu_items, ARRAY_SIZE(commu_items));

		/* Get rid of our char dev entries */	
		cdev_del(&commu_device->cdev);	

		class = commu_device->class;
		if(class)
		{
			class_device_destroy(class, devno);
			class_destroy(class);
		}

		kfree(commu_device);
		commu_device = NULL;
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);

	HNOS_DEBUG_INFO("Cleanup device %s, major %d \n", DEVICE_NAME, commu_major);
	return;
}

/*
 * Finally, the module stuff
 */
static int __init  commu_module_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *class = NULL;
    int i = 0;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if(commu_major) 
	{
		dev = MKDEV(commu_major, commu_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} 
	else 
	{
		result = alloc_chrdev_region(&dev, commu_minor, 1, DEVICE_NAME);
		commu_major = MAJOR(dev);
	}
	if(result < 0) 
	{
		printk(KERN_WARNING "hndl_kb: can't get major %d\n", commu_major);
		return result;
	}	

	/* 
	 * allocate the devices -- we do not have them static.
	 */
	commu_device = kmalloc(sizeof(struct at91_commu_cdev), GFP_KERNEL);
	if(!commu_device) 
	{
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(commu_device, 0, sizeof(struct at91_commu_cdev));	

	init_MUTEX(&commu_device->lock);

	/* Register a class_device in the sysfs. */
	class = class_create(THIS_MODULE, DEVICE_NAME);
	if(class == NULL) 
	{
		goto fail;
	}
	class_device_create(class, NULL, dev, NULL, DEVICE_NAME);
	commu_device->class = class;
	commu_cdev_setup(commu_device, dev);	

    for(i=0; i<ARRAY_SIZE(commu_items); i++) 
	{
        commu_items[i].read_func = commu_proc_read;
        commu_items[i].write_func = commu_proc_write;
    }
    hnos_proc_items_create(commu_items, ARRAY_SIZE(commu_items));

	HNOS_DEBUG_INFO("Initialized device %s, major %d \n", DEVICE_NAME, commu_major);
	return 0;

fail:
	commu_module_cleanup();
	return result;
}

EXPORT_SYMBOL(commu_module_register);
EXPORT_SYMBOL(commu_module_unregister);

module_init(commu_module_init);
module_exit(commu_module_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

