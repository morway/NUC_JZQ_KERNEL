/*
 * drivers/char/hndl_char_devices/hnos_battery_core.c
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
#include "hnos_battery.h"
#include "hnos_ioctl.h"
#include "hnos_flags1600u.h"

struct at91_battery_cdev *battery_device;
static int battery_major = 0;
static int battery_minor = 0;
module_param(battery_major, int, S_IRUGO);
module_param(battery_minor, int, S_IRUGO);

static int battery_module_open(struct inode *inode, struct file *filp)
{
    struct at91_battery_cdev *dev; 

    dev = container_of(inode->i_cdev, struct at91_battery_cdev, battery_cdev);
    filp->private_data = dev; /* for other methods */

    if(test_and_set_bit(0, &dev->is_open) != OK)
    {
        return -EBUSY;       
    }

    return OK;
}

static int battery_module_release(struct inode *inode, struct file *filp)
{
    struct at91_battery_cdev *dev = filp->private_data; 

    if(OK == test_and_clear_bit(0, &dev->is_open))
    {
        return -EINVAL;
    }

    return OK;
}

static ssize_t battery_module_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    u32     battery_value;
    int     ret;
    char    data[4];
    struct at91_battery_cdev *dev = filp->private_data; 
    struct at91_battery_object *battery = dev->battery; 

    down(&battery_device->battery_lock);
    ret = battery->get_battery_value(&battery_value);
    up(&battery_device->battery_lock);

    if(ret < 0)
    {
        HNOS_DEBUG_INFO("Read battery value error!\n");
        return ret;
    }

    data[0] = battery_value & 0xFF;
    data[1] = (battery_value >> 8) & 0xFF;
    data[2] = (battery_value >> 16) & 0xFF;
    data[3] = (battery_value >> 24) & 0xFF;

    ret = copy_to_user(buf, &data[0], 4);
    if(ret != OK)
    {
        HNOS_DEBUG_INFO("Func copy_to_user error! %d\n", ret);
        return -EFAULT;
    }

    return OK;
}

/*
** The ioctl() implementation
*/
static int battery_module_ioctl(struct inode *inode, struct file *filp, u32 cmd, unsigned long arg)
{
    int level = LOW;
    int ret = OK;
    struct at91_battery_cdev *dev = filp->private_data; /* device information */
    struct at91_battery_object *battery = dev->battery; 

    if((NULL == dev) || (NULL == battery))
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

    down(&battery_device->battery_lock);

    switch(cmd)
    {
        case IOC_BATTERY_CHARGE_ENABLE:
            if(LOW == battery->charge_state)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            ret = at91_set_gpio_value(battery->charge_pin, level);
            break;

        case IOC_BATTERY_CHARGE_DISABLE:
            if(LOW == battery->charge_state)
            {
                level = HIGH;
            }
            else
            {
                level = LOW;
            }
            ret = at91_set_gpio_value(battery->charge_pin, level);
            break;

        case IOC_BATTERY_SUPPLY_ENABLE:
            if(LOW == battery->supply_state)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            ret = at91_set_gpio_value(battery->supply_pin, level);
            break;
            
        case IOC_BATTERY_SUPPLY_DISABLE:
            if(LOW == battery->supply_state)
            {
                level = HIGH;
            }
            else
            {
                level = LOW;
            }
            ret = at91_set_gpio_value(battery->supply_pin, level);
            break;
            
        case IOC_BATTERY_STATE:
            *(int *)arg = at91_get_gpio_value(battery->power_type_pin);
            break;

        default:
            break;
    }

    up(&battery_device->battery_lock);

    return ret;
}

struct file_operations battery_fops =
{
    .owner  =   THIS_MODULE,
    .open   =   battery_module_open,
    .read   =   battery_module_read,
    .release =  battery_module_release,
    .ioctl  =   battery_module_ioctl,
};

int battery_module_register(struct at91_battery_object *battery)
{
    if(NULL == battery_device)
    {
        HNOS_DEBUG_INFO("No BATTERY device found!\n");
        return -ENODEV;
    }

    down(&battery_device->battery_lock);
    battery_device->battery = battery;
    up(&battery_device->battery_lock);

    HNOS_DEBUG_INFO("BATTERY object registered.\n");
    
    return OK;
}

int battery_module_unregister(struct at91_battery_object *battery)
{
    if(NULL == battery_device)
    {
        HNOS_DEBUG_INFO("No BATTERY device found!\n");
        return -ENODEV;
    }

    down(&battery_device->battery_lock);
    battery_device->battery = NULL;
    up(&battery_device->battery_lock);

    HNOS_DEBUG_INFO("Unregitered the BATTERY object.\n");
    
    return OK;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
static void __exit battery_module_exit(void)
{
    dev_t devno = MKDEV(battery_major, battery_minor);    

    if(battery_device != NULL)
    {
        /* Get rid of our char dev entries */    
        cdev_del(&battery_device->battery_cdev);    

        if(battery_device->battery_class != NULL)
        {
            class_device_destroy(battery_device->battery_class, devno);
            class_destroy(battery_device->battery_class);
        }

        kfree(battery_device);
        battery_device = NULL;
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, 1);
    
    return;
}

/*
 * Finally, the module stuff
 */
static int __init battery_module_init(void)
{
    int result = 0;
    dev_t dev = 0;
    struct class *pClass = NULL;

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if(battery_major != 0)
    {
        dev = MKDEV(battery_major, battery_minor);
        result = register_chrdev_region(dev, 1, BATTERY_DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev, battery_minor, 1, BATTERY_DEVICE_NAME);
        battery_major = MAJOR(dev);
    }
    if(result < 0)
    {
        HNOS_DEBUG_INFO("BATTERY can not get major and minor!\n");
        return ERROR;
    }    

    /* 
     * allocate the devices -- we do not have them static.
     */
    battery_device = kmalloc(sizeof(struct at91_battery_cdev), GFP_KERNEL);
    if(NULL == battery_device)
    {
        HNOS_DEBUG_INFO("BATTERY have no memory for battery_device!\n");
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }
    memset(battery_device, 0, sizeof(struct at91_battery_cdev));    

    init_MUTEX(&battery_device->battery_lock);

    /* Register a class_device in the sysfs. */
    pClass = class_create(THIS_MODULE, BATTERY_DEVICE_NAME);
    if(NULL == pClass)
    {
        HNOS_DEBUG_INFO("BATTERY can not create class!\n");
        kfree(battery_device);
        battery_device = NULL;
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }
    class_device_create(pClass, NULL, dev, NULL, BATTERY_DEVICE_NAME);
    battery_device->battery_class = pClass;

    /* Cdev setup */
    cdev_init(&battery_device->battery_cdev, &battery_fops);
    battery_device->battery_cdev.owner = THIS_MODULE;
    result = cdev_add (&battery_device->battery_cdev, dev, 1);
    if(result != OK)
    {
        HNOS_DEBUG_INFO("BATTERY can not add cdev!\n");
        class_device_destroy(battery_device->battery_class, dev);
        class_destroy(battery_device->battery_class);
        kfree(battery_device);
        battery_device = NULL;
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }

    HNOS_DEBUG_INFO("Initialized device %s, major %d \n", BATTERY_DEVICE_NAME, battery_major);
    
    return OK;
}

EXPORT_SYMBOL(battery_module_register);
EXPORT_SYMBOL(battery_module_unregister);

module_init(battery_module_init);
module_exit(battery_module_exit);

MODULE_AUTHOR("KT");
MODULE_LICENSE("Dual BSD/GPL");

