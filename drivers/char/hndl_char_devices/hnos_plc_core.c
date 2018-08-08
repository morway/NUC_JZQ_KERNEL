/*
 * drivers/char/hndl_char_devices/hnos_plc_core.c
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
#include "hnos_plc1600u.h"
#include "hnos_ioctl.h"
#include "hnos_flags1600u.h"

struct at91_plc_cdev *plc_device;
static int plc_major =   0;
static int plc_minor =   0;
module_param(plc_major, int, S_IRUGO);
module_param(plc_minor, int, S_IRUGO);

static int plc_module_open(struct inode *inode, struct file *filp)
{
    struct at91_plc_cdev *dev; 

    dev = container_of(inode->i_cdev, struct at91_plc_cdev, plc_cdev);
    filp->private_data = dev; /* for other methods */

    if(test_and_set_bit(0, &dev->is_open) != OK)
    {
        return -EBUSY;
    }

    return OK;
}

static int plc_module_release(struct inode *inode, struct file *filp)
{
    struct at91_plc_cdev *dev = filp->private_data; 

    if(OK == test_and_clear_bit(0, &dev->is_open))
    {
        return -EINVAL;
    }

    return OK;
}

/*
** The ioctl() implementation
*/
static int plc_module_ioctl(struct inode *inode, struct file *filp, u32 cmd, unsigned long arg)
{
    int level = LOW;
    struct at91_plc_cdev *dev = filp->private_data; /* device information */
    struct at91_plc_object *plc = dev->plc; 

    if((NULL == dev) || (NULL == plc))
    {
        HNOS_DEBUG_INFO("%s: no such device.\n", __FUNCTION__);
        return -ENODEV;
    }

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if(_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC)
    {
        return -ENOTTY;
    }
    if(_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR)
    {
        return -ENOTTY;
    }

    down(&plc_device->plc_lock);

    switch(cmd)
    {
        case IOC_PLC_POWERON:
            if(LOW == plc->power_on_state)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(plc->power_pin, level);
            break;

        case IOC_PLC_POWEROFF:
            if(LOW == plc->power_on_state)
            {
                level = HIGH;
            }
            else
            {
                level = LOW;
            }
            at91_set_gpio_value(plc->power_pin, level);
            break;

        case IOC_PLC_RESET:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(plc->reset_pin, level);
            break;
            
        case IOC_PLC_A:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(plc->plc_a_pin, level);
            break;
            
        case IOC_PLC_B:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(plc->plc_b_pin, level);
            break;

        case IOC_PLC_C:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(plc->plc_c_pin, level);
            break;

        default:
            break;
    }

    up(&plc_device->plc_lock);

    return OK;
}

struct file_operations plc_fops =
{
    .owner =    THIS_MODULE,
    .open =     plc_module_open,
    .release =  plc_module_release,
    .ioctl =    plc_module_ioctl,
};

int plc_module_register(struct at91_plc_object *plc)
{
    if(NULL == plc_device)
    {
        HNOS_DEBUG_INFO("No PLC device found!\n");
        return -ENODEV;
    }

    down(&plc_device->plc_lock);
    plc_device->plc = plc;
    up(&plc_device->plc_lock);

    HNOS_DEBUG_INFO("PLC object registered.\n");
    
    return OK;
}

int plc_module_unregister(struct at91_plc_object *plc)
{
    if(NULL == plc_device)
    {
        HNOS_DEBUG_INFO("No PLC device found!\n");
        return -ENODEV;
    }

    down(&plc_device->plc_lock);
    plc_device->plc = NULL;
    up(&plc_device->plc_lock);

    HNOS_DEBUG_INFO("Unregitered the PLC object.\n");
    
    return OK;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
static void __exit plc_module_exit(void)
{
    dev_t devno = MKDEV(plc_major, plc_minor);    

    if(plc_device != NULL)
    {
        /* Get rid of our char dev entries */    
        cdev_del(&plc_device->plc_cdev);    

        if(plc_device->plc_class != NULL)
        {
            class_device_destroy(plc_device->plc_class, devno);
            class_destroy(plc_device->plc_class);
        }

        kfree(plc_device);
        plc_device = NULL;
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, 1);
    
    return;
}

/*
 * Finally, the module stuff
 */
static int __init plc_module_init(void)
{
    int result = 0;
    dev_t dev = 0;
    struct class *pClass = NULL;

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if(plc_major != 0)
    {
        dev = MKDEV(plc_major, plc_minor);
        result = register_chrdev_region(dev, 1, PLC_DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev, plc_minor, 1, PLC_DEVICE_NAME);
        plc_major = MAJOR(dev);
    }
    if(result < 0)
    {
        HNOS_DEBUG_INFO("PLC can not get major and minor!\n");
        return ERROR;
    }    

    /* 
     * allocate the devices -- we do not have them static.
     */
    plc_device = kmalloc(sizeof(struct at91_plc_cdev), GFP_KERNEL);
    if(NULL == plc_device)
    {
        HNOS_DEBUG_INFO("PLC have no memory for plc_device!\n");
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }
    memset(plc_device, 0, sizeof(struct at91_plc_cdev));    

    init_MUTEX(&plc_device->plc_lock);

    /* Register a class_device in the sysfs. */
    pClass = class_create(THIS_MODULE, PLC_DEVICE_NAME);
    if(NULL == pClass)
    {
        HNOS_DEBUG_INFO("PLC can not create class!\n");
        kfree(plc_device);
        plc_device = NULL;
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }
    class_device_create(pClass, NULL, dev, NULL, PLC_DEVICE_NAME);
    plc_device->plc_class = pClass;

    /* Cdev setup */
    cdev_init(&plc_device->plc_cdev, &plc_fops);
    plc_device->plc_cdev.owner = THIS_MODULE;
    result = cdev_add (&plc_device->plc_cdev, dev, 1);
    if(result != OK)
    {
        HNOS_DEBUG_INFO("PLC can not add cdev!\n");
        class_device_destroy(plc_device->plc_class, dev);
        class_destroy(plc_device->plc_class);
        kfree(plc_device);
        plc_device = NULL;
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }

    HNOS_DEBUG_INFO("Initialized device %s, major %d \n", PLC_DEVICE_NAME, plc_major);
    
    return OK;
}

EXPORT_SYMBOL(plc_module_register);
EXPORT_SYMBOL(plc_module_unregister);

module_init(plc_module_init);
module_exit(plc_module_exit);

MODULE_AUTHOR("KT");
MODULE_LICENSE("Dual BSD/GPL");

