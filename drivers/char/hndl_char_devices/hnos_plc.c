/*
 * drivers/char/hndl_char_devices/hnos_plc.c
 * 
 * PLC module control.
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowplcgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include "hnos_generic.h"
#include "hnos_plc.h"                /* plc  */
#include "hnos_ioctl.h"

#define DEVICE_NAME   "plc_ctrl"

struct at91_plc_cdev 
{       
	struct cdev cdev;			
	unsigned long is_open;
	struct class *myclass;
};

static int plc_major =   0;
static int plc_minor =   0;
module_param(plc_major, int, S_IRUGO);
module_param(plc_minor, int, S_IRUGO);

struct at91_plc_cdev  *plc_device;

static int at91_plc_open(struct inode *inode, struct file *filp)
{
	struct at91_plc_cdev *dev; 

	dev = container_of(inode->i_cdev, struct at91_plc_cdev, cdev);
	filp->private_data = dev; /* for other methods */

	if (test_and_set_bit(0, &dev->is_open) != 0) {
		return -EBUSY;       
	}

	return 0; 
}

static int at91_plc_release(struct inode *inode, struct file *filp)
{
	struct at91_plc_cdev *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}

/*The ioctl() implementation */
static int at91_plc_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int err = 0;

	/* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
	if (_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC) {
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR ) {
		return -ENOTTY;
	}

	/*
	 * the type is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. Note that the type is user-oriented, while
	 * verify_area is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	}

	if (err) {
		return -EFAULT;
	}

	dprintk("%s: cmd %02x , pin %2x \n", __FUNCTION__, cmd, (int)arg);

	switch(cmd) {
		case IOC_PLC_POWERON:	
			at91_set_gpio_output(PLC_POWER_PIN, 1);
			break;

		case IOC_PLC_POWEROFF:	
			at91_set_gpio_output(PLC_POWER_PIN, 0);
			break;

		default: 
			return -ENOTTY;
	}


	return 0;
}

struct file_operations plc_fops =
{
	.owner =    THIS_MODULE,
	.open =     at91_plc_open,
	.release =  at91_plc_release,
	.ioctl = at91_plc_ioctl,
};

static void  at91_plc_cdev_setup(struct at91_plc_cdev *dev, dev_t devno)
{
	int err;

	cdev_init(&dev->cdev, &plc_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&dev->cdev, devno, 1);

	if (err) { 
		printk(KERN_NOTICE "Error %d adding PLC device, major_%d", err, MAJOR(devno));
	}
	return;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
void at91_plc_cleanup(void)
{
	dev_t devno = MKDEV(plc_major, plc_minor);	
	struct class * myclass;

	if (plc_device){
		/* Get rid of our char dev entries */	
		cdev_del(&plc_device->cdev);	

		myclass = plc_device->myclass;
		if (myclass){
			class_device_destroy(myclass, devno);
			class_destroy(myclass);
		}

		kfree(plc_device);
		plc_device = NULL;
	}

	/* cleanup_module is never calplc if registering faiplc */
	unregister_chrdev_region(devno, 1);

	HNOS_DEBUG_INFO("Unregister device %s, major %d.\n", DEVICE_NAME, plc_major);
	return;
}

/*
 * Finally, the module stuff
 */
static int __init  at91_plc_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *myclass = NULL;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (plc_major) {
		dev = MKDEV(plc_major, plc_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} else {
		result = alloc_chrdev_region(&dev, plc_minor, 1, DEVICE_NAME);
		plc_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "%s: can't get major %d\n", DEVICE_NAME, plc_major);
		return result;
	}	

	/* 
	 * allocate the devices -- we do not have them static.
	 */
	plc_device = kmalloc(sizeof(struct at91_plc_cdev), GFP_KERNEL);
	if (!plc_device) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(plc_device, 0, sizeof(struct at91_plc_cdev));	

	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if (myclass == NULL) {
		goto fail;
	}

	class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	plc_device->myclass = myclass;

	at91_plc_cdev_setup(plc_device, dev);	

	HNOS_DEBUG_INFO("Initialized device %s, major %d.\n", DEVICE_NAME, plc_major);
	return 0;

fail:
	at91_plc_cleanup();
	return result;
}

module_init(at91_plc_init);
module_exit(at91_plc_cleanup);

MODULE_LICENSE("Dual BSD/GPL");


