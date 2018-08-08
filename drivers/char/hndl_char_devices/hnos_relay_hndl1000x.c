/*
 *  drivers/char/hndl_char_devices/hnos_74lv595.c
 *
 *  For HNDL1000S .
 *  led control,parallel to serial.
 *  ZhangRM, peter_zrm@163.com, 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "hnos_generic.h" 
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include "hnos_input.h"
#include "hnos_iomem.h"                /* iomem object  */
#include "hnos_hndl1000x.h"
#include "hnos_ioctl.h"

#define  DATA_NE	LEDDK      /* (总线扩展)数据 */
/*clk由以下组合产生
  CLK_A_NE CLK_B_NE  ->
  0               0  = 0
  0               1  = 1 //高
  1               0  = 0 //低
  1               1  = 0
  */
#define  CLK_A_NE	LEDGK             /* (总线扩展)时钟 */
#define  CLK_B_NE	LEDBD             /* (总线扩展)时钟 */
#define  ST_CS_NE	AT91_PIN_PA1      /* (总线扩展)锁存 */

#define CLK_OUT(level) \
	do { \
		if(level) {\
			lv595_dev->iomem->write_bit(lv595_dev->iomem, CLK_A_NE,SIG_LOW);\
			lv595_dev->iomem->write_bit(lv595_dev->iomem, CLK_B_NE,SIG_HIGH);\
		} \
		else {\
			lv595_dev->iomem->write_bit(lv595_dev->iomem, CLK_A_NE,SIG_HIGH);\
			lv595_dev->iomem->write_bit(lv595_dev->iomem, CLK_B_NE,SIG_LOW);\
		} \
	} while(0)



#define DEVICE_NAME		        "parallel_led"
#define HNDL_LV595_DEBUG        1
#define RELAY_OUTPUT_DISABLE    1
#define RELAY_OUTPUT_ENABLE     0
#define RELAY_START_IO		AT91_PIN_PA0

#define RELAY_LED_DATA		AT91_PIN_PA1

static int lv595_parallel2serail_opt(u8 ledvalue);

struct hndl_lv595_cdev 
{
	spinlock_t lock;                
	unsigned long is_open;
	struct iomem_object *iomem;
	struct class *myclass;
	struct cdev cdev; 
	u8 led_value;
	u8 output_enable;
};


static struct hndl_lv595_cdev *lv595_dev = NULL;
static int lv595_major =   0;
static int lv595_minor =   0;
int lv595_open(struct inode *inode, struct file *filp);
int lv595_release(struct inode *inode, struct file *filp);
ssize_t lv595_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
static int lv595_ioctl (struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);
static int lv595_proc_write(struct proc_item *, const char *, unsigned long);
static int lv595_proc_read(struct proc_item *item, char *page);

static  struct proc_dir_entry	*hndl_proc_dir = NULL;
static struct proc_item items[] = 
{
	[0] = {
		.name = "parallel_led", 
		.write_func = lv595_proc_write,
		.read_func = lv595_proc_read,
	},
	{NULL}
};

struct file_operations hndl_lv595_fops =
{
	.owner =    THIS_MODULE,
	.open =     lv595_open,
	.release =  lv595_release,
	.read = lv595_read,
	.ioctl = lv595_ioctl,
};

int lv595_open(struct inode *inode, struct file *filp)
{
	struct hndl_lv595_cdev *dev; 
	dev = container_of(inode->i_cdev, struct hndl_lv595_cdev, cdev);
	filp->private_data = dev; 
	if (test_and_set_bit(0,&dev->is_open)!=0) {
		return -EBUSY;       
	}
	return 0; 
}

int lv595_release(struct inode *inode, struct file *filp)
{
	struct hndl_lv595_cdev *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) {
		return -EINVAL; 
	}

	return 0;
}

static int lv595_parallel2serail_opt(u8 ledvalue)
{      
	int ret = 0;
	int i = 0;
	u8  opt_bit = 1;

	ret = lv595_dev->iomem->write_bit(lv595_dev->iomem, DATA_NE,SIG_LOW);
	CLK_OUT(SIG_LOW);
	at91_set_gpio_value(ST_CS_NE,SIG_LOW);

	for(i = 0; i < 8; i++)
	{
		ret = lv595_dev->iomem->write_bit(lv595_dev->iomem, DATA_NE,(ledvalue&opt_bit));
		CLK_OUT(SIG_HIGH);		
		opt_bit =opt_bit << 1;
		CLK_OUT(SIG_LOW);

	}
	at91_set_gpio_value(ST_CS_NE,SIG_HIGH);

	return ret;

}

void parallel2serail_led_autoscan(void)
{
	int ret =0;

	spin_lock(&lv595_dev->lock);
	if(!lv595_dev->output_enable){
		at91_set_gpio_value(RELAY_START_IO,RELAY_OUTPUT_ENABLE);
	}
	ret = lv595_parallel2serail_opt(lv595_dev->led_value);
	if(!lv595_dev->output_enable){
		at91_set_gpio_value(RELAY_START_IO,RELAY_OUTPUT_DISABLE);
	}        
	spin_unlock(&lv595_dev->lock);
}
EXPORT_SYMBOL(parallel2serail_led_autoscan);

ssize_t lv595_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos)
{
	unsigned char redata[4];
	struct hndl_lv595_cdev *dev = filp->private_data;

	spin_lock(&dev->lock);
	redata[0] = lv595_dev->led_value;
	spin_unlock(&dev->lock);

	//printk("kernel led_value=%02x,redata=%02x,\n", lv595_dev->led_value,redata[0]);
	if (copy_to_user(buf,redata,1)){
		return (-EFAULT);
	}
	return 1;   
}

static int lv595_ioctl (struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	u8 data = arg & 0xff;//8个灯控制数据
	u8 bitmap = (arg >> 8) & 0xff; //需要操作的灯相应位为1
	u8 bit =0,i;
	struct hndl_relay_cdev *dev = filp->private_data; /* device information */

	if (!dev) {
		printk("%s: no such device.\n", __FUNCTION__);
		return -ENODEV;
	}

	switch(cmd){
		case IOC_RELAY_LED_VALUE:
			spin_lock(&lv595_dev->lock);
			for(i = 0; i < 8; i++){
				bit = 1 << i;
				if(bitmap & bit){
					if(data & bit)
						lv595_dev->led_value |=bit;
					else
						lv595_dev->led_value &=~bit;
				}
			}
			spin_unlock(&lv595_dev->lock);
			break;
		case IOC_RELAY_OUTPUT_ENABLE:
			spin_lock(&lv595_dev->lock);
			at91_set_gpio_value(RELAY_START_IO,RELAY_OUTPUT_ENABLE);
			ret = lv595_parallel2serail_opt(lv595_dev->led_value);
			lv595_dev->output_enable = 1;
			spin_unlock(&lv595_dev->lock);
			break;  
		case IOC_RELAY_OUTPUT_DISABLE:
			spin_lock(&lv595_dev->lock);
			at91_set_gpio_value(RELAY_START_IO,RELAY_OUTPUT_DISABLE);
			lv595_dev->output_enable = 0;
			spin_unlock(&lv595_dev->lock);
			break;
	}
	return ret;
}


#ifdef HNDL_LV595_DEBUG 

static int __init  lv595_proc_devices_add(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = items; item->name; ++item) {
		ret += hnos_proc_entry_create(item);
	}

	return ret;
}
static int lv595_proc_devices_remove(void)
{
	struct proc_item *item;

	for (item = items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}

static int lv595_proc_read(struct proc_item *item, char *page)
{
	char my_buffer[8];
	char *buf;
	u8 ledvalue = 0;

	buf = my_buffer; 
	spin_lock(&lv595_dev->lock);
	ledvalue = lv595_dev->led_value;
	spin_unlock(&lv595_dev->lock);

	buf += sprintf(buf," %02x\n", ledvalue);
	strcpy(page,my_buffer);
	return strlen(my_buffer);
}


static int lv595_proc_write(struct proc_item *item, const char __user * userbuf,
		unsigned long count) 
{
	u32 value = 0;
	char val[14] = {0};

	if (count >= 14){
		return -EINVAL;
	}

	if (copy_from_user(val, userbuf, count)){
		return -EFAULT;
	}

	value = (unsigned int)simple_strtoull(val, NULL, 16);

	spin_lock(&lv595_dev->lock);

	if(!lv595_dev->output_enable){
		at91_set_gpio_value(RELAY_START_IO,RELAY_OUTPUT_ENABLE);
	}
	lv595_dev->led_value = (value & 0xff);
	lv595_parallel2serail_opt(lv595_dev->led_value);
	if(!lv595_dev->output_enable){
		at91_set_gpio_value(RELAY_START_IO,RELAY_OUTPUT_DISABLE);
	}

	spin_unlock(&lv595_dev->lock);
	return 0;
}

#endif



static void __exit lv595_exit(void)
{
	dev_t devno = MKDEV(lv595_major, lv595_minor);    
	struct class *myclass;

#ifdef HNDL_LV595_DEBUG
	if (hndl_proc_dir) {
		lv595_proc_devices_remove();
		hnos_proc_rmdir();
	}
#endif

	if (lv595_dev != NULL){

		iomem_object_put(lv595_dev->iomem);

		/* Get rid of our char dev entries */  

		cdev_del(&lv595_dev->cdev);    

		myclass = lv595_dev->myclass;
		if (myclass){
			class_device_destroy(myclass, devno);
			class_destroy(myclass);
		}

		kfree(lv595_dev);
		lv595_dev = NULL;
	}

	unregister_chrdev_region(devno, 1);
	return;    

}

//no test
static int __init lv595_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *myclass;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	//ENABLE LV595 OE

	if (lv595_major){ 
		dev = MKDEV(lv595_major, lv595_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} 
	else{ 

		result = alloc_chrdev_region(&dev, lv595_minor, 1, DEVICE_NAME);
		lv595_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "RELAY: can't get major %d\n", lv595_major);
		return result;
	}
	/************/        

	lv595_dev = kmalloc(sizeof(struct hndl_lv595_cdev), GFP_KERNEL);
	if (!lv595_dev) {
		pr_debug("RELAY: can't alloc lv595_dev\n");
		result = -ENOMEM;
		goto kmalloc_fail;  /* Make this more graceful */
	}
	memset(lv595_dev, 0, sizeof(struct hndl_lv595_cdev)); 

	spin_lock_init(&lv595_dev->lock);


	lv595_dev->iomem = iomem_object_get(RELAY_CS1, 0);
	if (!lv595_dev->iomem) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) RELAY_CS2);
		result = -1;
		goto iomem_fail;
	}

	/************/

	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if (myclass == NULL) {
		pr_debug("RELAY: can't creat class\n");
		result = -ENODEV;
		goto class_create_fail;
	}
	class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	lv595_dev->myclass = myclass;
	cdev_init(&lv595_dev->cdev, &hndl_lv595_fops);
	lv595_dev->cdev.owner = THIS_MODULE;
	result = cdev_add(&lv595_dev->cdev, dev, 1);
	if (result) {
		printk(KERN_NOTICE "Error %d adding lv595 device, major_%d.\n", result, MAJOR(dev));
		goto cdev_add_fail;
	}   

	/************/

#ifdef HNDL_LV595_DEBUG 
	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		result = -ENODEV;
		goto proc_fail;
	} else {
		result = lv595_proc_devices_add();
		if (result) {
			goto proc_fail;
		}
	}
#endif
	at91_set_GPIO_periph(ST_CS_NE,0);
	at91_set_gpio_value(ST_CS_NE,1);
	lv595_dev->led_value = 0;
	lv595_dev->output_enable = 0;
	lv595_parallel2serail_opt(0);

	HNOS_DEBUG_INFO("lv595_hntt1000x registered.\n");
	return 0;

proc_fail:
	cdev_del(&lv595_dev->cdev);    
cdev_add_fail:
	class_device_destroy(myclass, dev);
	class_destroy(myclass);
class_create_fail:        
	iomem_object_put(lv595_dev->iomem);
iomem_fail:
	kfree(lv595_dev);
kmalloc_fail:
	unregister_chrdev_region(dev, 1);

	return result;

}

module_init(lv595_init);
module_exit(lv595_exit);
MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

