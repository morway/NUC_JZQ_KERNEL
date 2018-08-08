/*
 * drivers/char/hndl_char_devices/hnos_dc_intf.c 
 *
 * Interface for DC Analog Measurement.
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
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_dc_intf.h" 

#define DEVICE_NAME	"dc_analog"

static struct dc_device *dc;
static int dc_major =   0;
static int dc_minor =   0;

int dc_adc_register(struct adc_ops *adc)
{
    if (!adc || !adc->read) {
        printk(KERN_ERR "%s error: invalid params.\n", __FUNCTION__);
        return -EINVAL;
    }

    if (dc->adc) {
        printk(KERN_ERR "%s error: adc already existed.\n", __FUNCTION__);
        return -EINVAL;
    }

    down(&dc->lock);
    dc->adc = adc;
    up(&dc->lock);

    HNOS_DEBUG_INFO("Register a ADC instance for DC Analog interface.\n");
    return 0;
}

int dc_adc_unregister(struct adc_ops *adc)
{
    if (!adc || !adc->read || !dc->adc) {
        printk(KERN_ERR "%s error: invalid params.\n", __FUNCTION__);
        return -EINVAL;
    }

    if (dc->adc != adc) {
        printk(KERN_ERR "%s error: another adc_ops found!\n", __FUNCTION__);
        return -EINVAL;
    }

    down(&dc->lock);
    dc->adc = NULL;
    up(&dc->lock);

    HNOS_DEBUG_INFO("Unregister the ADC instance for DC Analog interface.\n");
    return 0;
}

static int dc_read_adc(struct dc_device *dev, u8 ch, u32 *reslt)
{
    int ret = 0;

    if (!dev || !reslt || !dev->adc || !dev->adc->read) {
        printk(KERN_ERR "%s error: invalid params.\n", __FUNCTION__);
        return -EINVAL;
    }

    down(&dc->lock);
    ret = dev->adc->read(ch, reslt);
    up(&dc->lock);

    dprintk("%s: read ch %d return code %d, relst %d.\n", 
                    __FUNCTION__, ch, ret, *reslt);
    return ret;
}

int dc_open(struct inode *inode, struct file *filp)
{
    static struct dc_device *dev;

    dev = container_of(inode->i_cdev, struct dc_device, cdev);
    filp->private_data = dev; 

    if (test_and_set_bit(0, &dev->is_open) != 0) {
        return -EBUSY;       
    }

    return 0; 
}

int dc_release(struct inode *inode, struct file *filp)
{
    struct dc_device *dev = filp->private_data; 

    if (test_and_clear_bit(0, &dev->is_open) == 0) {
        return -EINVAL; 
    }

    return 0;
}

ssize_t dc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct dc_device *dev = filp->private_data; 
    short reslt = 0;
    u8    channel = 0;
    int   ret   = 0;

    /* It's ugly, but we really do this in Linux-2.4.18 on S3C2410 :-( */
    if (get_user(channel, buf)) {
        return -EFAULT;
    }

    ret = dc_read_adc(dev, channel,(u32 *) &reslt);
    if (ret < 0) {
        return -EIO;
    }

    if (copy_to_user(buf, &reslt, sizeof(reslt))) {
        return -EFAULT;
    }

    return sizeof(reslt);
}

static int dc_ioctl (struct inode *inode, struct file *filp, 
                     unsigned int cmd, unsigned long arg)
{
    struct dc_device *dev = filp->private_data; 
    u32 reslt = 0;
    int   ret = 0;

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC) {
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR ) {
        return -ENOTTY;
    }

    dprintk("%s: cmd %02x ", __FUNCTION__, cmd);
    switch(cmd) {
        case IOC_ANALOG_CHAN_MAX:	/* The Maximum channels of DC interface.*/
            return NR_DC_CHAN;

        case IOC_DC_ANALOG_GET:	   /* Get the ADC of the spcified channel.*/
            if (arg >= NR_DC_CHAN) {
                return -EFAULT;
            }

            ret = dc_read_adc(dev, arg, &reslt);
            if (ret < 0) {
                return ret;
            } 

            return reslt;

        default:  
            return -ENOTTY;
    }

    return 0;

}

int dc_proc_read(struct proc_item *item, char *page)
{
    int      cnt  = 0;
    u32 reslt_ch0 = 0;
    u32 reslt_ch1 = 0;

    dc_read_adc(dc, 0, &reslt_ch0);
    dprintk("%s: ch %d, relst %d.\n", __FUNCTION__, 0, reslt_ch0);

    dc_read_adc(dc, 1, &reslt_ch1);
    dprintk("%s: ch %d, relst %d.\n", __FUNCTION__, 1, reslt_ch1);

    cnt += sprintf(page + cnt, "%d\t\t%d\n", 0, reslt_ch0); 
    cnt += sprintf(page + cnt, "%d\t\t%d\n", 1, reslt_ch1); 

    return cnt;
}

struct file_operations dc_fops =
{
    .owner =    THIS_MODULE,
    .read =     dc_read,
    .open =     dc_open,
    .ioctl =    dc_ioctl,
    .release =  dc_release,
};

static struct proc_item items[] = 
{
    [0] = {
        .name = DEVICE_NAME, 
        .read_func = dc_proc_read,
    },
};

static void  inline dc_cdev_setup(struct dc_device *dev, dev_t devno)
{
    int err;

    cdev_init(&dev->cdev, &dc_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);

    if (err) {
        printk(KERN_NOTICE "Error %d adding dc device, major_%d.\n",
                err, MAJOR(devno));
    }

    return;
}

static void  dc_exit(void)
{
    dev_t devno = MKDEV(dc_major, dc_minor);	
    struct class * class;

    if (dc){
        /* Get rid of our char dev entries */	
        cdev_del(&dc->cdev);	

        class = dc->class;
        if (class){
            class_device_destroy(class, devno);
            class_destroy(class);
        }

        hnos_proc_items_remove(items, ARRAY_SIZE(items));

        kfree(dc);
        dc = NULL;
    }

    unregister_chrdev_region(devno, 1);

    HNOS_DEBUG_INFO("Module DC Interface Exit.\n");
    return;
}

static int __init  dc_init(void)
{
    int result = 0;
    dev_t dev = 0;
    struct class *class;

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if (dc_major) {
        dev = MKDEV(dc_major, dc_minor);
        result = register_chrdev_region(dev, 1, DEVICE_NAME);
    } else {
        result = alloc_chrdev_region(&dev, dc_minor, 1, DEVICE_NAME);
        dc_major = MAJOR(dev);
    }
    if (result < 0) {
        printk(KERN_WARNING "dc: can't get major %d\n", dc_major);
        return result;
    }	

    /* allocate the devices -- we do not have them static. */
    dc = kmalloc(sizeof(struct dc_device), GFP_KERNEL);
    if (!dc) {
        result = -ENOMEM;
        goto fail;  /* Make this more graceful */
    }
    memset(dc, 0, sizeof(struct dc_device));	

    init_MUTEX(&dc->lock);

    /* Register a class_device in the sysfs. */
    class = class_create(THIS_MODULE, DEVICE_NAME);
    if (class == NULL) {
        goto fail;
    }

    class_device_create(class, NULL, dev, NULL, DEVICE_NAME);
    dc->class = class;
    dc_cdev_setup(dc, dev);	

    hnos_proc_items_create(items, ARRAY_SIZE(items));

    HNOS_DEBUG_INFO("Initialized device %s, major %d.\n", DEVICE_NAME, dc_major);
    return 0;

fail:
    dc_exit();
    return result;
}

EXPORT_SYMBOL(dc_adc_register);
EXPORT_SYMBOL(dc_adc_unregister);

module_init(dc_init);
module_exit(dc_exit);

MODULE_LICENSE("Dual BSD/GPL");
