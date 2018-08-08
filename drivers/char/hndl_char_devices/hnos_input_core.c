/*
 *  drivers/char/hndl_char_devices/hnos_output_core.c
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
#include "hnos_gpio.h" 
#include "hnos_input.h"

#define DEVICE_NAME	"io-input"

struct class *class;
static  struct proc_dir_entry	*hndl_proc_dir = NULL;
static struct hndl_rmi_device *rmis[NR_INPUT_DEVICES];
static DEFINE_SPINLOCK(rmis_lock);
static int rmi_major =   0;
static int rmi_minor =   -1;

static int rmi_smcbus_proc_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    struct smcbus_rmi_data *smcbus = (struct smcbus_rmi_data *)data;

    if (smcbus->proc_read) {
        len = smcbus->proc_read(smcbus, buf);
    }
    if (len < 0)
        return len;

    if (len <= off + count)
        *eof = 1;
    *start = buf + off;
    len -= off;
    if (len > count)
        len = count;
    if (len < 0)
        len = 0;

    return len;
}

static int rmi_smcbus_proc_create(struct hndl_rmi_device *rmi)
{
    struct proc_dir_entry *proc;
    unsigned char name[18] = {0};

    sprintf(name, "smcbus-input%d", MINOR(rmi->cdev.dev));

    if (rmi->smcbus && hndl_proc_dir) {
        proc = create_proc_read_entry(name,
                S_IFREG | S_IRUGO | S_IWUSR,
                hndl_proc_dir,
                rmi_smcbus_proc_read,
                rmi->smcbus);
        if (!proc) {
            return -1;
        }
        if (proc) {
            proc->read_proc = (read_proc_t *) rmi_smcbus_proc_read;
        }

        return 0;
    }

    return -1;
}

static  int rmi_smcbus_proc_del(struct hndl_rmi_device *rmi)
{
    unsigned char name[18] = {0};

    if (rmi->smcbus && hndl_proc_dir) {
        sprintf(name, "smcbus-input%d", MINOR(rmi->cdev.dev));
        remove_proc_entry(name, hndl_proc_dir);
    }

    return 0;
}

static int rmi_gpio_proc_del(struct hndl_rmi_device *rmi)
{
    struct gpio_rmi_data *gpio = rmi->gpio;
    struct proc_item *item = gpio->items;
    int i = 0;

    for (i=0; i<gpio->size; i++) {
        item = &gpio->items[i];
        remove_proc_entry(item->name, hndl_proc_dir);
    }

    return 0;
}

static int  rmi_gpio_proc_create(struct hndl_rmi_device *rmi)
{
    struct gpio_rmi_data *gpio = rmi->gpio;
    struct proc_item *item = gpio->items;
    int ret = 0, i = 0;

    for (i=0; i<gpio->size; i++) {
        item = &gpio->items[i];
        if (item->pin) {
            hnos_gpio_cfg(item->pin, item->settings);
        }
        ret += hnos_proc_entry_create(item);
    }
    return ret;
}

int rmi_get_smcbus_offset(struct hndl_rmi_device *rmi)
{
    return rmi->smcbus_offset;
}

int rmi_gpio_register(struct hndl_rmi_device *rmi, struct gpio_rmi_data *gpio, u8 offset, u8 size)
{
    down(&rmi->lock);
    rmi->gpio = gpio;
    rmi->gpio_offset = offset;
    rmi->gpio_end = size - 1;
    up(&rmi->lock);

    HNOS_DEBUG_INFO("GPIOs for Remote Input Signal registered, offset %d, end %d.\n",
            offset, size - 1);
    return 0; 
}

int rmi_smcbus_register(struct hndl_rmi_device *rmi, struct smcbus_rmi_data *bus, u8 offset, u8 size)
{
    u8 smcbus_off = offset; 
    u8 smcbus_end = offset + size - 1;

    down(&rmi->lock);

    if (0 == offset) {
        if (rmi->gpio) {
            smcbus_off = rmi->gpio_end + 1;
        } else {
            smcbus_off = 0;
        }

        smcbus_end = smcbus_off + size - 1;
    }
    if (smcbus_end >= INPUT_CHAN_MAX) {
        printk(KERN_WARNING "%s: so many smcbus channels:-(\n", __FUNCTION__);
        up(&rmi->lock);
        return -EFAULT;
    }

    rmi->smcbus = bus;
    rmi->smcbus_offset = smcbus_off;
    rmi->smcbus_end = smcbus_end;

    up(&rmi->lock);

    HNOS_DEBUG_INFO("SMCBUS for Remote Input Signal registered, offset %d, end %d.\n", 
            smcbus_off, smcbus_end);
    return  0;
}

int rmi_gpio_unregister(struct hndl_rmi_device *rmi, struct gpio_rmi_data *gpio)
{
    down(&rmi->lock);
    rmi_gpio_proc_del(rmi);

    rmi->gpio = NULL;
    rmi->gpio_offset = 0;
    rmi->gpio_end = 0;
    up(&rmi->lock);

    return 0;
}

int rmi_smcbus_unregister(struct hndl_rmi_device *rmi, struct smcbus_rmi_data *bus)
{
    down(&rmi->lock);
    rmi_smcbus_proc_del(rmi);

    rmi->smcbus = NULL;
    rmi->smcbus_offset = 0;
    rmi->smcbus_end = 0;
    up(&rmi->lock);

    return 0;
}

static inline int rmi_read_gpio_chan(struct gpio_rmi_data *gpio, u8 ch)
{
    struct proc_item *item;

    if (!gpio || !gpio->items || ch >= gpio->size) {
        return -EFAULT;
    }

    item = &gpio->items[ch];
    if (item && item->read_data) {
        return item->read_data(ch);
    } else if (item->pin) {
        return at91_get_gpio_value(item->pin);
    }

    return -EFAULT;
}

static inline int rmi_read_smcbus_chan(struct smcbus_rmi_data *bus, u8 ch)
{
    int ret = 0;
    u32 reslt = 0;

    if (!bus || !bus->read) {
        return -EFAULT;
    }

    ret =  bus->read(bus, ch, &reslt);
    if (ret) {
        return -EIO;
    }

    return reslt;
}

static inline int rmi_is_gpio(struct hndl_rmi_device *rmi, u8 ch)
{
    return ( ( (ch >= rmi->gpio_offset) && (ch <= rmi->gpio_end) ) ? 1 : 0 ); 
}

static inline int rmi_is_smcbus(struct hndl_rmi_device *rmi, u8 ch)
{
    return ( ( (ch >= rmi->smcbus_offset) && (ch <= rmi->smcbus_end) ) ? 1 : 0 ); 
}

static int rmi_read_channel(struct hndl_rmi_device *rmi, u8 ch)
{
    int reslt = -EFAULT;

    if (!rmi || ch > INPUT_CHAN_MAX) {
        return -EFAULT;
    }

    down(&rmi->lock);

    if (rmi->gpio && rmi_is_gpio(rmi, ch)) {
        reslt = rmi_read_gpio_chan(rmi->gpio, ch);
    } else if (rmi->smcbus && rmi_is_smcbus(rmi, ch)) {
        reslt = rmi_read_smcbus_chan(rmi->smcbus, ch);
    }

    up(&rmi->lock);
    return reslt;
}

static int rmi_gpio_read_all(struct gpio_rmi_data *gpio, u32 *reslt)
{
    int i = 0, tmp = 0;

    if (!gpio) {
        return -EFAULT;
    }

    for (i=0; i<gpio->size; i++) {
        tmp = (rmi_read_gpio_chan(gpio, i) ? 1 : 0);
        *reslt |= (tmp << i); 
    }

    return 0;

}

static inline int rmi_smcbus_read_all(struct smcbus_rmi_data *bus, u32 *reslt)
{
    if (bus && bus->read) {
        return bus->read(bus, SMCBUS_CHAN_ALL, reslt);
    }
    return -EFAULT;
}

static int rmi_read_all(struct hndl_rmi_device *rmi, u32 *reslt)
{
    u32 gpio = 0;
    u32 smcbus = 0;
    int ret = 0;

    if (!rmi) {
        return -EFAULT;
    }

    down(&rmi->lock);

    if (rmi->gpio) {
        ret = rmi_gpio_read_all(rmi->gpio, &gpio);
        if (ret < 0) {
            goto fail;
        }
    }

    if (rmi->smcbus) {
        ret = rmi_smcbus_read_all(rmi->smcbus, &smcbus);
        if (ret < 0) {
            goto fail;
        }
    }

    *reslt = ( smcbus << rmi->smcbus_offset ) | gpio;

   // printk("%s: reslt %x, smcbus %x, gpio %x, shift %d.\n", __FUNCTION__,
   //         *reslt, smcbus, gpio, rmi->smcbus_offset);
fail:
    up(&rmi->lock);
    return ret;
}

static int rmi_open(struct inode *inode, struct file *filp)
{
    static struct hndl_rmi_device *dev;

    dev = container_of(inode->i_cdev, struct hndl_rmi_device, cdev);
    filp->private_data = dev; /* for other methods */

    if (test_and_set_bit(0, &dev->is_open) != 0) {
        return -EBUSY;       
    }

    return 0; /* success. */
}

static int rmi_release(struct inode *inode, struct file *filp)
{
    struct hndl_rmi_device *dev = filp->private_data; 

    if (test_and_clear_bit(0, &dev->is_open) == 0) { /* release lock, and check... */
        return -EINVAL; /* already released: error */
    }

    return 0;
}

static ssize_t rmi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    //	struct hndl_rmi_device *dev = filp->private_data; 
    short reslt = 0;
    /* To be implemented */

    return sizeof(reslt);
}

/*The ioctl() implementation */
static int rmi_ioctl (struct inode *inode, struct file *filp,
        unsigned int cmd, unsigned long arg)
{
    int err = 0; 
    int stat = 0;
    struct hndl_rmi_device *dev = filp->private_data; 
    int chan_max = (dev->smcbus_end ? dev->smcbus_end : dev->gpio_end); 

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC) {
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR ) {
        return -ENOTTY;
    }

    dprintk("%s: cmd %02x ", __FUNCTION__, cmd);
    switch(cmd) {
    
    	
        case IOC_INPUT_CHAN_MAX:	/* The Maximum channels of rmi signal.*/
            //	dprintk("IOC_INPUT_CHAN_MAX.\n");
            return (chan_max + 1);

        case IOC_INPUT_STATE_GET:	/* Get the state of the spcified channel.*/
            //	dprintk("IOC_INPUT_STATE_GET.\n");
            if (arg > chan_max) {
                return -EFAULT;
            }
            stat = rmi_read_channel(dev, arg);
            return stat;

        case IOC_INPUT_ALL_CHAN:	/* Get the state of all the channels.*/
            //	dprintk("IOC_INPUT_ALL_CHAN.\n");
            err = rmi_read_all(dev, &stat);
            if (err) {
                return -EIO;
            }

            if (put_user(stat, (unsigned long __user *) arg)) {
                return -EFAULT;
            }

            return 0;

        default:  
            return -ENOTTY;
    }

    return 0;
}


struct file_operations hndl_rmi_fops = {
    .owner =    THIS_MODULE,
    .open = rmi_open,
    .release = rmi_release,
    .read = rmi_read,
    .ioctl = rmi_ioctl,
};

struct hndl_rmi_device *rmi_device_alloc(void)
{
    struct hndl_rmi_device *dev;

    dev = kmalloc(sizeof(struct hndl_rmi_device), GFP_KERNEL);
    if (!dev) {
        printk(KERN_ERR "%s: No memory available.\n", __FUNCTION__);
        return NULL;
    }	

    memset(dev, 0, sizeof(struct hndl_rmi_device));
    init_MUTEX(&dev->lock);	
    return dev;
}

void rmi_device_free(struct hndl_rmi_device *dev)
{
    kfree(dev);
    dev = NULL;
    return;
}

int rmi_device_register(struct hndl_rmi_device *dev)
{
    dev_t devno;
    int ret = 0;
    unsigned char name[10] = {0};

    spin_lock(&rmis_lock);
    if ((++rmi_minor) >= NR_INPUT_DEVICES) {		
        spin_unlock(&rmis_lock);		
        return -1;
    }
    rmis[rmi_minor] = dev;
    spin_unlock(&rmis_lock);

    devno = MKDEV(rmi_major, rmi_minor);
    cdev_init(&dev->cdev, &hndl_rmi_fops);
    dev->cdev.owner = THIS_MODULE;

    ret = cdev_add(&dev->cdev, devno, 1);
    if (ret) {   
        printk(KERN_NOTICE "Error %d adding major=%d \n", ret, MAJOR(devno));
        return ret;
    }

    sprintf(name, "in%d", rmi_minor);
    class_device_create(class, NULL, devno, NULL, name);

    if (dev->smcbus) {
        ret = rmi_smcbus_proc_create(dev);
        if (ret) {   
            printk(KERN_WARNING"Error %d adding major=%d \n", ret, MAJOR(devno));
            return ret;
        }
    }

    if (dev->gpio) {
        ret = rmi_gpio_proc_create(dev);
        if (ret) {   
            printk(KERN_WARNING "Error %d adding major=%d \n", ret, MAJOR(devno));
            return ret;
        }
    }

    HNOS_DEBUG_INFO("Input device %s registerd.\n", name);	
    return ret;
}

int rmi_device_unregister(struct hndl_rmi_device *dev)
{
    dev_t devno = dev->cdev.dev;

    cdev_del(&dev->cdev);
    class_device_destroy(class, devno);

    spin_lock(&rmis_lock);
    if ((MINOR(devno)) >= NR_INPUT_DEVICES) {		
        spin_unlock(&rmis_lock);		
        return -1;
    }
    rmis[MINOR(devno)] = NULL;
    spin_unlock(&rmis_lock);


    HNOS_DEBUG_INFO("Input device in%d unregistered.\n", MINOR(devno));	
    return 0;
}

static void  rmi_module_cleanup(void)
{
    dev_t devno = MKDEV(rmi_major, 0);
    unregister_chrdev_region(devno, NR_INPUT_DEVICES);	

    if (class) {
        class_destroy(class);
        class = NULL;
    }
    if (hndl_proc_dir) {
        hnos_proc_rmdir();
    }

    HNOS_DEBUG_INFO("Cleanup device %s, major %d \n", DEVICE_NAME, rmi_major);	
    return;
}


/*
 * Finally, the module stuff
 */
static int __init  rmi_module_init(void)
{
    int result = 0;
    dev_t dev = 0;

    result = alloc_chrdev_region(&dev, 0, NR_INPUT_DEVICES, DEVICE_NAME);
    rmi_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "hndl_rmi_device: can't get major %d\n", rmi_major);
        return result;
    }

    class = class_create(THIS_MODULE, DEVICE_NAME);
    if (!class){
        printk(KERN_ERR "Can't create class.\n");
        result = -1;
        goto fail;
    }


    HNOS_DEBUG_INFO("Initialized device %s, major %d \n", DEVICE_NAME, rmi_major);	

    hndl_proc_dir = hnos_proc_mkdir();
    if (!hndl_proc_dir) {
        result = -ENODEV;
        goto fail;
    } 
    return result;

fail:
    HNOS_DEBUG_INFO("Initialize device %s failed.\n", DEVICE_NAME);

    rmi_module_cleanup();
    return result;

}


EXPORT_SYMBOL(rmi_device_alloc);
EXPORT_SYMBOL(rmi_device_free);
EXPORT_SYMBOL(rmi_device_register);
EXPORT_SYMBOL(rmi_device_unregister);
EXPORT_SYMBOL(rmi_smcbus_register);
EXPORT_SYMBOL(rmi_gpio_register);
EXPORT_SYMBOL(rmi_smcbus_unregister);
EXPORT_SYMBOL(rmi_gpio_unregister);
EXPORT_SYMBOL(rmi_get_smcbus_offset);

module_init(rmi_module_init);
module_exit(rmi_module_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

