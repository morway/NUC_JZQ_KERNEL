/*
 * drivers/char/hndl_char_devices/hnos_kdb_core.c
 *  drivers for the keyboard on AT91SAM9260EK.
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

#include  "hnos_generic.h"
#include  "hnos_proc.h"
#include  "hnos_kbd.h"
#include "hnos_debug.h"


/*
 *Global macros.
 */
#define HNDL_KBD_DEBUG		1
#define DEVICE_NAME		"hndl1000kb"
#define KB_BUF_LEN		(1 << 8)

/*
 * Our parameters which can be set at load time.
 */
static int kb_major =   0;
static int kb_minor =   0;
static  struct proc_dir_entry *hndl_proc_dir = NULL;

/*
 *Definition for the hndl_kb_cdev.
 */
struct hndl_kb_cdev 
{
	spinlock_t lock;                 /* mutual exclusion lock*/
	unsigned long is_open;	  
	time_t lastkey_time;

	wait_queue_head_t hndl_kb_queue; 
	struct circ_buf *cir_buf;        /*circular buf*/

	struct class *myclass;
	struct cdev cdev;	         /* Char device structure*/
};

static blt_func_t   lcd_back_light = NULL;

static struct hndl_kb_cdev *hndl_kb_device;

ssize_t kb_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos);
ssize_t kb_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos);
int kb_open(struct inode *inode, struct file *filp);
int kb_release(struct inode *inode, struct file *filp);
void kb_module_cleanup(void);

struct file_operations hndl_kb_fops =
{
	.owner =    THIS_MODULE,
	.read =     kb_read,
	.write =    kb_write,
	.open =     kb_open,
	.release =  kb_release,
};

module_param(kb_major, int, S_IRUGO);
module_param(kb_minor, int, S_IRUGO);


/* Circular Buffer Functions */

/*
 * kb_buf_clear
 *
 * Clear out all data in the circular buffer.
 */

static void kb_buf_clear(struct circ_buf *cb)
{
	cb->head = cb->tail = 0;
}


/*
 * kb_buf_data_avail
 *
 * Return the number of bytes of data available in the circular
 * buffer.
 */

static int kb_buf_data_avail(struct circ_buf *cb)
{
	return CIRC_CNT(cb->head,cb->tail,KB_BUF_LEN);
}

/*
 * kb_buf_alloc
 *
 * Allocate a circular buffer and all associated memory.
 */

static struct circ_buf *kb_buf_alloc(void)
{
	struct circ_buf *cb;

	cb = (struct circ_buf *)kmalloc(sizeof(struct circ_buf), GFP_KERNEL);
	if (cb == NULL)
		return NULL;

	cb->buf = kmalloc(KB_BUF_LEN, GFP_KERNEL);
	if (cb->buf == NULL) {
		kfree(cb);
		return NULL;
	}

	kb_buf_clear(cb);

	return cb;
}


/*
 * kb_buf_free
 *
 * Free the buffer and all associated memory.
 */

static void kb_buf_free(struct circ_buf *cb)
{
	kfree(cb->buf);
	kfree(cb);
}

/*
 * kb_buf_space_avail
 *
 * Return the number of bytes of space available in the circular
 * buffer.
 */

static int kb_buf_space_avail(struct circ_buf *cb)
{
	return CIRC_SPACE(cb->head,cb->tail,KB_BUF_LEN);
}


/*
 * kb_buf_put
 *
 * Copy data data from a user buffer and put it into the circular buffer.
 * Restrict to the amount of space available.
 *
 * Return the number of bytes copied.
 */

static int kb_buf_put(struct circ_buf *cb, const char *buf, int count)
{
	int c, ret = 0;
	while (1) {
		c = CIRC_SPACE_TO_END(cb->head, cb->tail, KB_BUF_LEN);
		if (count < c)
			c = count;
		if (c <= 0)
			break;
		memcpy(cb->buf + cb->head, buf, c);
		cb->head = (cb->head + c) & (KB_BUF_LEN-1);
		buf += c;
		count -= c;
		ret += c;
	}
#if 0
	printk("space(%d,%d,%d)=%d now...\n",
			cb->head,
			cb->tail,
			KB_BUF_LEN,
			CIRC_SPACE_TO_END(cb->head,cb->tail,KB_BUF_LEN));
#endif 
	return ret;
}


/*
 * kb_buf_get
 *
 * Get data from the circular buffer and copy to the given buffer.
 * Restrict to the amount of data available.
 *
 * Return the number of bytes copied.
 */

static int kb_buf_get(struct circ_buf *cb, char *buf, int count)
{
	int c, ret = 0;

	while (1) {
		c = CIRC_CNT_TO_END(cb->head, cb->tail, KB_BUF_LEN);
		if (count < c)
			c = count;
		if (c <= 0)
			break;
		memcpy(buf, cb->buf + cb->tail, c);
		cb->tail = (cb->tail + c) & (KB_BUF_LEN-1);
		buf += c;
		count -= c;
		ret += c;
	}

	return ret;
}

#if 0
/* FIX ME  */
int magnet_status_read(void) 
{
	struct hndl_kb_cdev *dev = hndl_kb_device; 
	int status = 0;

	if (dev == NULL) {
		printk(KERN_ERR "%s: NO keyboard device found.\n", __FUNCTION__);
		return -ENODEV;
	}

	if (dev->iomem == NULL) {
		printk(KERN_ERR "%s: IO memory not mapped.\n", __FUNCTION__);
		return -EFAULT;
	}

	status = readb(dev->iomem); 
	status = (status >> 6) & 0x1;

	return status;
}
EXPORT_SYMBOL(magnet_status_read);
#endif


/*
 * Set up the char_dev structure for this device.
 */
static void  kb_cdev_setup(struct hndl_kb_cdev *dev, dev_t devno)
{
	int err;

	cdev_init(&dev->cdev, &hndl_kb_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&dev->cdev, devno, 1);

	if (err) { /* Fail gracefully if need be */
		printk(KERN_NOTICE "Error %d adding hndl_kb_%d", err, MAJOR(devno));
	}
        
        return;
}

void kb_key_insert(unsigned char key)
{
	struct hndl_kb_cdev *dev = hndl_kb_device;
	unsigned long flags; 

	if (!dev) {
		return;
	}

	spin_lock_irqsave(&dev->lock, flags);
	kb_buf_put(dev->cir_buf, &key, 1);
	spin_unlock_irqrestore(&dev->lock, flags);

	wake_up_interruptible(&dev->hndl_kb_queue);
        kb_lastkey_time_update();
	return;
}

void kb_custom_key_insert(void *key, size_t size)
{
	struct hndl_kb_cdev *dev = hndl_kb_device;
	unsigned long flags; 

	if (!dev || !key) {
		return;
	}

	spin_lock_irqsave(&dev->lock, flags);
	kb_buf_put(dev->cir_buf, key, size);
	spin_unlock_irqrestore(&dev->lock, flags);
	
	wake_up_interruptible(&dev->hndl_kb_queue);
	return;
}

int lcd_blt_register(blt_func_t blt_routine)
{
    if (lcd_back_light != NULL) {
        printk("%s: error register blt routine, the callback already exsited.\n", __FUNCTION__);
        return -EBUSY;
    }

    if (blt_routine == NULL) {
        printk("%s: error register blt routine, argument is invalid.\n", __FUNCTION__);
        return -EINVAL;
    }

    lcd_back_light = blt_routine;
    return 0;
}

int lcd_blt_unregister(blt_func_t blt_routine)
{
    if (lcd_back_light == NULL) {
        printk("%s: error register blt routine, the callback is NULL.\n", __FUNCTION__);
        return -EBUSY;
    }

    if (blt_routine == NULL) {
        printk("%s: error register blt routine, argument is invalid.\n", __FUNCTION__);
        return -EINVAL;
    }

    lcd_back_light = NULL;
    return 0;
}

/*
 * Open and close
 */
int kb_open(struct inode *inode, struct file *filp)
{
	struct hndl_kb_cdev *dev; 

	dev = container_of(inode->i_cdev, struct hndl_kb_cdev, cdev);
	filp->private_data = dev; /* for other methods */

	if (test_and_set_bit(0,&dev->is_open)!=0) {
		return -EBUSY;       
	}

	return 0; /*success.*/
}

int kb_release(struct inode *inode, struct file *filp)
{
	struct hndl_kb_cdev *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}

/* return 1 if the buffer is not empty. */
static inline int kb_buf_not_empty(struct hndl_kb_cdev *dev)
{
	return (kb_buf_data_avail(dev->cir_buf) > 0);
}

/*
 * Data management: read and write
 */
ssize_t kb_read(struct file *filp, char __user *buf, size_t count,
		loff_t *f_pos)
{
	struct hndl_kb_cdev *dev = filp->private_data; 
	u32 key_v = 0xffffffff;
	int retval = 0;
	unsigned long flags; 

	size_t sz_key_kernel = 1;
	size_t sz_key_userspace = 2;
	
	if (count == 4) {
		sz_key_kernel = 4;
		sz_key_userspace = 4;
	}
	
	spin_lock_irqsave(&dev->lock, flags);

	while (!kb_buf_not_empty(dev)){	/*buf is empty*/
		spin_unlock_irqrestore(&dev->lock, flags);

		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		}

		if(wait_event_interruptible(dev->hndl_kb_queue, kb_buf_not_empty(dev))) {
			return -ERESTARTSYS;
		}

		spin_lock_irqsave(&dev->lock, flags);
	}

	retval = kb_buf_get(dev->cir_buf, (char *)&key_v, sz_key_kernel);
	if (retval != sz_key_kernel) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -EFAULT;	
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	if (copy_to_user((char __user *)buf, (char *)&key_v, sz_key_userspace)) {
		return -EFAULT;
	}

	return sz_key_userspace;
}

ssize_t kb_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *f_pos)
{
	int retval=0;
	unsigned char data;                                                                               
	struct hndl_kb_cdev *dev = filp->private_data; 
	unsigned long flags; 

	if (count != 1) {
		return -EINVAL;		
	}

	if (lcd_back_light == NULL) {
		return -EINVAL;		
	}

	if (get_user(data, (unsigned char __user *) buf)){
		retval= -EFAULT;
		goto out;
	}
	retval = sizeof(unsigned char);

	spin_lock_irqsave(&dev->lock, flags);

	if (data != 0x00) {
		lcd_back_light(LCD_BLT_ON);
	} else {
		lcd_back_light(LCD_BLT_OFF);
	}

	spin_unlock_irqrestore(&dev->lock, flags);

out:
	return retval;
}

void kb_lastkey_time_update(void)
{
	struct hndl_kb_cdev *dev = hndl_kb_device;
	struct timespec uptime;
	unsigned long flags; 

	do_posix_clock_monotonic_gettime(&uptime);

	spin_lock_irqsave(&dev->lock, flags);
	dev->lastkey_time = uptime.tv_sec;
	spin_unlock_irqrestore(&dev->lock, flags);

	return;
}

static int kb_lastkey_time_read(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct hndl_kb_cdev *dev = (struct hndl_kb_cdev *)data;
	unsigned long flags; 
	time_t tv_sec;
	int len;

	spin_lock_irqsave(&dev->lock, flags);
	tv_sec = dev->lastkey_time;
	spin_unlock_irqrestore(&dev->lock, flags);

	len = sprintf(page, "%lu\n", (unsigned long) tv_sec);
	if (len <= off+count) {
		*eof = 1;
	}
	*start = page + off;
	len -= off;
	if (len > count) {
		len = count;
	}
	if (len < 0) {
		len = 0;
	}
	return len;
}

#ifdef HNDL_KBD_DEBUG 

/*
 * The proc filesystem: function to read and entry
 */
int kb_proc_read(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	int len = 0, i = 0, cnt = 0;
	struct hndl_kb_cdev *dev = data;
	char key_v;
	unsigned long flags; 

	spin_lock_irqsave(&dev->lock, flags);

	len = kb_buf_data_avail(dev->cir_buf);	
	cnt += sprintf(buf+cnt, "data available=%d, space available=%d\n",
			len, kb_buf_space_avail(dev->cir_buf));

	for(i=0; i<len; i++){
		kb_buf_get(dev->cir_buf, &key_v, sizeof(key_v));
		cnt += sprintf(buf+cnt, "%2x\n", key_v);
	}
	cnt += sprintf(buf+cnt, "\n");

	spin_unlock_irqrestore(&dev->lock, flags);

	*eof = 1;
	return cnt;
}

#endif 

static struct file_operations hndl_file_ops = {
	.read = kb_proc_read,
	//.write = demo_write,
  //  .open = module_open,
  //  .release = module_close,
};




static struct file_operations time_file_ops = {
	.read = kb_lastkey_time_read,
	.write = kb_lastkey_time_update,
  //  .open = module_open,
  //  .release = module_close,
};





/*
 * Finally, the module stuff
 */
static int __init  kb_module_init(void)
{
	int result=0;
	dev_t dev = 0;
	struct class *myclass;
	struct proc_dir_entry *proc;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (kb_major) {
		dev = MKDEV(kb_major, kb_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} else {
		result = alloc_chrdev_region(&dev, kb_minor, 1,	DEVICE_NAME);
		kb_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "hndl_kb: can't get major %d\n", kb_major);
		return result;
	}else{
	    printk("hndl_kb: get major %d\n", kb_major);
	}

	hndl_kb_device = kmalloc(sizeof(struct hndl_kb_cdev), GFP_KERNEL);
	if (!hndl_kb_device) {
		result = -ENOMEM;
		printk("kmalloc fail!\n");
		goto fail;  /* Make this more graceful */
	}
	
	memset(hndl_kb_device, 0, sizeof(struct hndl_kb_cdev));	

	spin_lock_init(&hndl_kb_device->lock);
	init_waitqueue_head(&hndl_kb_device->hndl_kb_queue);

	hndl_kb_device->cir_buf = kb_buf_alloc();
	if (hndl_kb_device->cir_buf == NULL){
		printk(KERN_ERR "hndl_kb: can't alloc cir_buf.\n");
		result = -ENOMEM;
		goto fail;
	}
	
	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if (myclass == NULL) {
		goto fail;
	}
	//class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	hndl_kb_device->myclass = myclass;
	kb_cdev_setup(hndl_kb_device, dev);	

#ifdef HNDL_KBD_DEBUG 
	//proc = create_proc_read_entry(DEVICE_NAME, 0, NULL,
	//		kb_proc_read, hndl_kb_device);
	proc=proc_create(DEVICE_NAME, 0,  NULL, &hndl_file_ops);	
#endif
#if 1
	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		result = -ENODEV;
		goto fail;
	} 

//	proc = create_proc_read_entry("last_key_time", S_IFREG | S_IRUGO | S_IWUSR, 
  //hndl_proc_dir, kb_lastkey_time_read, hndl_kb_device);
	proc=proc_create("last_key_time", S_IFREG | S_IRUGO | S_IWUSR,  hndl_proc_dir, &time_file_ops);

	if (!proc) {
		result = -1;
		goto fail;
	}
#endif
	HNOS_DEBUG_INFO("Initialized keyboard core, device %s, major %d \n",
                        DEVICE_NAME, kb_major);
	return 0;

fail:
	kb_module_cleanup();
	return result;
}


/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void kb_module_cleanup(void)
{
	dev_t devno = MKDEV(kb_major, kb_minor);	
	struct class *myclass;

#ifdef HNDL_KBD_DEBUG
	remove_proc_entry(DEVICE_NAME, NULL);
#endif

	if (hndl_proc_dir) {
		remove_proc_entry("last_key_time", hndl_proc_dir);
		hnos_proc_rmdir();
	}

	if (hndl_kb_device){
		/* Get rid of our char dev entries */	
		cdev_del(&hndl_kb_device->cdev);	

		if (hndl_kb_device->cir_buf) {
			kb_buf_free(hndl_kb_device->cir_buf);
		}

		myclass = hndl_kb_device->myclass;
		if (myclass){
			//class_device_destroy(myclass, devno);
			device_destroy(myclass, devno);
			class_destroy(myclass);
		}

		kfree(hndl_kb_device);
		hndl_kb_device = NULL;
	}

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);

	HNOS_DEBUG_INFO("Unregistered keyboard core, device %s, major %d \n",
                        DEVICE_NAME, kb_major);
	return;
}

EXPORT_SYMBOL(kb_key_insert);
EXPORT_SYMBOL(kb_custom_key_insert);
EXPORT_SYMBOL(kb_lastkey_time_update);

EXPORT_SYMBOL(lcd_blt_register);
EXPORT_SYMBOL(lcd_blt_unregister);

module_init(kb_module_init);
module_exit(kb_module_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

