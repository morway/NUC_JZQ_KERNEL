/*
 * drivers/char/hndl_char_devices/hnos_ad79x1.c 
 * drivers for the AD7921/AD7911, which are 10-bit and 12-bit, high speed, low
 * power, 2-channel successive approximation ADCs.
 *
 *
 * 2008.01.11, first created.
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

#define AD79X1_DEBUG	1
#define DEVICE_NAME	"ad79x1"

struct  ad79x1_device
{
	unsigned long is_open;
	struct class *myclass;
	struct cdev cdev;
	struct spi_device *spi;
};

static struct ad79x1_device *ad79x1;
static int ad79x1_major =   0;
static int ad79x1_minor =   0;

int ad79x1_open(struct inode *inode, struct file *filp);
int ad79x1_release(struct inode *inode, struct file *filp);
ssize_t ad79x1_read(struct file *filp, char __user *buf, size_t count,	loff_t *f_pos);

struct file_operations ad79x1_fops =
{
	.owner =    THIS_MODULE,
	.read =     ad79x1_read,
	.open =     ad79x1_open,
	.release =  ad79x1_release,
};

static  short ad79x1_read_chan(struct ad79x1_device *dev, unsigned int ch)
{
	struct spi_device *spi = dev->spi;
	struct spi_message message;
	struct spi_transfer xfer;
	u8 tx_buf[2] = {0, 0};
	u8 rx_buf[2] = {0, 0};
	int status = 0;
	short reslt = 0;

	/* Build our spi message */
	spi_message_init(&message);
	memset(&xfer, 0, sizeof(xfer));
	xfer.len = 2;
	xfer.tx_buf = tx_buf;
	xfer.rx_buf = rx_buf;

	if (ch != 0) {
		tx_buf[0] = (0x1 << 5);
	}

	spi_message_add_tail(&xfer, &message);

	/* do the i/o */
	status = spi_sync(spi, &message);
	if (status == 0) {
		status = message.status;
	} else {
		return status;
	}

	dprintk("%s: ch %d, [%2x, %2x].\n", __FUNCTION__, ch, rx_buf[0], rx_buf[1]);

	reslt = (rx_buf[0] << 8) | (rx_buf[1]);
	return reslt;
}

static  short ad79x1_read_chan_ntimes(struct ad79x1_device *dev, unsigned int ch, unsigned int n)
{
	int reslt = 0;
	short total = 0;
	int i = 0;

	/* The first one is discarded. */
	ad79x1_read_chan(dev, ch);

	for (i=0; i<n; i++) {
		reslt = ad79x1_read_chan(dev, ch);
		msleep(10);
		if ((reslt >> 13) != ch || (reslt & 0x3) != 0) {
			printk("WARING: ad79x1 read channel %d error.\n", ch);
			return -1;
		}

		reslt = (reslt & 0xfff) >> 2; /* See the Datasheet of AD7911. */
		total += reslt;
	}

	return (short)(total/n);
}

static void  ad79x1_cdev_setup(struct ad79x1_device *dev, dev_t devno)
{
	int err;

	cdev_init(&dev->cdev, &ad79x1_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);

	if (err) {
		printk(KERN_NOTICE "Error %d adding ad79x1 device, major_%d.\n", err, MAJOR(devno));
	}

	return;
}

int ad79x1_open(struct inode *inode, struct file *filp)
{
	static struct ad79x1_device *dev;

	dev = container_of(inode->i_cdev, struct ad79x1_device, cdev);
	filp->private_data = dev; /* for other methods */

	if (test_and_set_bit(0, &dev->is_open) != 0) {
		return -EBUSY;       
	}

	return 0; /* success. */
}

int ad79x1_release(struct inode *inode, struct file *filp)
{
	struct ad79x1_device *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) { /* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}

ssize_t ad79x1_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct ad79x1_device *dev = filp->private_data; 
	short reslt = 0;
	unsigned char channel = 0;

	if (get_user(channel, buf)) {
		return -EFAULT;
	}

	reslt = ad79x1_read_chan_ntimes(dev, channel, 3);
	if (reslt < 0) {
		return -EIO;
	}

	if (copy_to_user(buf, &reslt, sizeof(reslt))) {
		return -EFAULT;
	}

	return sizeof(reslt);
}

static int  ad79x1_remove(struct spi_device *spi)
{
	dev_t devno = MKDEV(ad79x1_major, ad79x1_minor);	
	struct class * myclass;

#ifdef AD79X1_DEBUG
	remove_proc_entry(DEVICE_NAME, NULL);
#endif
	if (ad79x1){
		/* Get rid of our char dev entries */	
		cdev_del(&ad79x1->cdev);	

		myclass = ad79x1->myclass;
		if (myclass){
			class_device_destroy(myclass, devno);
			class_destroy(myclass);
		}

		kfree(ad79x1);
		ad79x1 = NULL;
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);
	return 0;
}

#ifdef AD79X1_DEBUG
int ad79x1_proc_read(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	int cnt = 0;
	struct ad79x1_device *dev = data;
	short reslt_ch0 = ad79x1_read_chan_ntimes(dev, 0, 3);
	short reslt_ch1 = ad79x1_read_chan_ntimes(dev, 1, 3);

	cnt += sprintf(buf + cnt, "CH\t\tADC\n");
	cnt += sprintf(buf + cnt, "%d\t\t%d\n", 0, reslt_ch0); 
	cnt += sprintf(buf + cnt, "%d\t\t%d\n", 1, reslt_ch1); 

	*eof = 1;
	return cnt;
}
#endif

static int __devinit ad79x1_probe(struct spi_device *spi)
{
	int result = 0;
	dev_t dev = 0;
	struct class *myclass;
	struct proc_dir_entry *proc;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (ad79x1_major) {
		dev = MKDEV(ad79x1_major, ad79x1_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} else {
		result = alloc_chrdev_region(&dev, ad79x1_minor, 1, DEVICE_NAME);
		ad79x1_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "hndl_kb: can't get major %d\n", ad79x1_major);
		return result;
	}	

	/* allocate the devices -- we do not have them static. */
	ad79x1 = kmalloc(sizeof(struct ad79x1_device), GFP_KERNEL);
	if (!ad79x1) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(ad79x1, 0, sizeof(struct ad79x1_device));	

	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if (myclass == NULL) {
		goto fail;
	}

	class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	ad79x1->myclass = myclass;

	ad79x1_cdev_setup(ad79x1, dev);	

#ifdef AD79X1_DEBUG
	proc = create_proc_read_entry(DEVICE_NAME, 0, NULL, ad79x1_proc_read, ad79x1);
#endif
	ad79x1->spi = spi;

	HNOS_DEBUG_INFO("Initialized device %s, major %d.\n", DEVICE_NAME, ad79x1_major);
	return 0;

fail:
	ad79x1_remove(spi);
	return result;
}

static struct spi_driver ad79x1_driver = {
	.driver = {
		.name	= "ad79x1",
		.owner	= THIS_MODULE,
	},
	.probe	= ad79x1_probe,
	.remove	= __devexit_p(ad79x1_remove),
};

static int __init  ad79x1_init(void)
{
	return spi_register_driver(&ad79x1_driver);
}

static void __exit ad79x1_exit(void)
{
	spi_unregister_driver(&ad79x1_driver);
}

module_init(ad79x1_init);
module_exit(ad79x1_exit);

MODULE_LICENSE("Dual BSD/GPL");
