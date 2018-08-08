/*
 * drivers/char/hndl_char_devices/hnos_pio_core.c
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
#include "hnos_pio.h"
#include "hnos_ioctl.h"
#include "hnos_flags1600u.h"

struct at91_pio_cdev *pio_device;
static int pio_major =   0;
static int pio_minor =   0;
module_param(pio_major, int, S_IRUGO);
module_param(pio_minor, int, S_IRUGO);

static int pio_module_open(struct inode *inode, struct file *filp)
{
    struct at91_pio_cdev *dev; 

    dev = container_of(inode->i_cdev, struct at91_pio_cdev, pio_cdev);
    filp->private_data = dev; /* for other methods */

    if(test_and_set_bit(0, &dev->is_open) != OK)
    {
        return -EBUSY;
    }

    return OK;
}

static int pio_module_release(struct inode *inode, struct file *filp)
{
    struct at91_pio_cdev *dev = filp->private_data; 

    if(OK == test_and_clear_bit(0, &dev->is_open))
    {
        return -EINVAL;
    }

    return OK;
}

/*
** The ioctl() implementation
*/
static int pio_module_ioctl(struct inode *inode, struct file *filp, u32 cmd, unsigned long arg)
{
    int level = LOW;
    struct at91_pio_cdev *dev = filp->private_data; /* device information */
    struct at91_pio_object *pio = dev->pio; 

    if((NULL == dev) || (NULL == pio))
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

    down(&pio_device->pio_lock);

    switch(cmd)
    {
        case IOC_PIO_BIT0:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit0_pin, level);
            break;

        case IOC_PIO_BIT1:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit1_pin, level);
            break;

        case IOC_PIO_BIT2:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit2_pin, level);
            break;
        
        case IOC_PIO_BIT3:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit3_pin, level);
            break;

        case IOC_PIO_BIT4:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit4_pin, level);
            break;
        
        case IOC_PIO_BIT5:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit5_pin, level);
            break;
        
        case IOC_PIO_BIT6:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit6_pin, level);
            break;
        
        case IOC_PIO_BIT7:
            if(LOW == arg)
            {
                level = LOW;
            }
            else
            {
                level = HIGH;
            }
            at91_set_gpio_value(pio->pio_bit7_pin, level);
            break;

        default:
            break;
    }

    up(&pio_device->pio_lock);

    return OK;
}

struct file_operations pio_fops =
{
    .owner =    THIS_MODULE,
    .open =     pio_module_open,
    .release =  pio_module_release,
    .ioctl =    pio_module_ioctl,
};

int pio_module_register(struct at91_pio_object *pio)
{
    if(NULL == pio_device)
    {
        HNOS_DEBUG_INFO("No PIO device found!\n");
        return -ENODEV;
    }

    down(&pio_device->pio_lock);
    pio_device->pio = pio;
    up(&pio_device->pio_lock);

    HNOS_DEBUG_INFO("PIO object registered.\n");
    
    return OK;
}

int pio_module_unregister(struct at91_pio_object *pio)
{
    if(NULL == pio_device)
    {
        HNOS_DEBUG_INFO("No PIO device found!\n");
        return -ENODEV;
    }

    down(&pio_device->pio_lock);
    pio_device->pio = NULL;
    up(&pio_device->pio_lock);

    HNOS_DEBUG_INFO("Unregitered the PIO object.\n");
    
    return OK;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
static void __exit pio_module_exit(void)
{
    dev_t devno = MKDEV(pio_major, pio_minor);    

    if(pio_device != NULL)
    {
        /* Get rid of our char dev entries */    
        cdev_del(&pio_device->pio_cdev);    

        if(pio_device->pio_class != NULL)
        {
            class_device_destroy(pio_device->pio_class, devno);
            class_destroy(pio_device->pio_class);
        }

        kfree(pio_device);
        pio_device = NULL;
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, 1);
    
    return;
}

/*
 * Finally, the module stuff
 */
static int __init pio_module_init(void)
{
    int result = 0;
    dev_t dev = 0;
    struct class *pClass = NULL;

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if(pio_major != 0)
    {
        dev = MKDEV(pio_major, pio_minor);
        result = register_chrdev_region(dev, 1, PIO_DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev, pio_minor, 1, PIO_DEVICE_NAME);
        pio_major = MAJOR(dev);
    }
    if(result < 0)
    {
        HNOS_DEBUG_INFO("PIO can not get major and minor!\n");
        return ERROR;
    }    

    /* 
     * allocate the devices -- we do not have them static.
     */
    pio_device = kmalloc(sizeof(struct at91_pio_cdev), GFP_KERNEL);
    if(NULL == pio_device)
    {
        HNOS_DEBUG_INFO("PIO have no memory for pio_device!\n");
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }
    memset(pio_device, 0, sizeof(struct at91_pio_cdev));    

    init_MUTEX(&pio_device->pio_lock);

    /* Register a class_device in the sysfs. */
    pClass = class_create(THIS_MODULE, PIO_DEVICE_NAME);
    if(NULL == pClass)
    {
        HNOS_DEBUG_INFO("PIO can not create class!\n");
        kfree(pio_device);
        pio_device = NULL;
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }
    class_device_create(pClass, NULL, dev, NULL, PIO_DEVICE_NAME);
    pio_device->pio_class = pClass;

    /* Cdev setup */
    cdev_init(&pio_device->pio_cdev, &pio_fops);
    pio_device->pio_cdev.owner = THIS_MODULE;
    result = cdev_add (&pio_device->pio_cdev, dev, 1);
    if(result != OK)
    {
        HNOS_DEBUG_INFO("PIO can not add cdev!\n");
        class_device_destroy(pio_device->pio_class, dev);
        class_destroy(pio_device->pio_class);
        kfree(pio_device);
        pio_device = NULL;
        unregister_chrdev_region(dev, 1);
        return ERROR;
    }

    HNOS_DEBUG_INFO("Initialized device %s, major %d \n", PIO_DEVICE_NAME, pio_major);
    
    return OK;
}

EXPORT_SYMBOL(pio_module_register);
EXPORT_SYMBOL(pio_module_unregister);

module_init(pio_module_init);
module_exit(pio_module_exit);

MODULE_AUTHOR("KT");
MODULE_LICENSE("Dual BSD/GPL");

