/*
 *  drivers/char/hndl_char_devices/hnos_battery.c
 *
 * Author ZhangRM, peter_zrm@163.com
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "hnos_generic.h"
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include "hnos_bat_adc.h"

#define NR_ADC_TIMES			4
#define DEVICE_NAME		        "battery"
#define BATTERY_CHARGE_CONTRL    AT91_PIN_PA17
#define VCC5V_INVALID       0
#define VCC5V_AVAILABLE     1
#define VCC5V_DETECT_MAX    10

#define VCC5V_MIN_VALUE     450
struct hndl_vcc5v_detect
{
    u8 detect_cnt;
    u8 available;
};
static struct hndl_vcc5v_detect *vcc5v_detect = NULL;

struct hndl_battery_cdev 
{
        spinlock_t lock;                
        unsigned long is_open;
        struct iomem_object *iomem;
        struct class *myclass;
        struct cdev cdev; 
};

static struct hndl_battery_cdev *battery_dev = NULL;
static int battery_major =   0;
static int battery_minor =   0;
int battery_open(struct inode *inode, struct file *filp);
int battery_release(struct inode *inode, struct file *filp);
ssize_t battery_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
static int battery_ioctl (struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg);
static  struct proc_dir_entry	*hndl_proc_dir = NULL;
static int battery_proc_voltage_get(struct proc_item *item, char *page);
static int vcc5v_proc_adc_get(struct proc_item *item, char *page);

/* 
 * Sample the Battery voltage.
 * The ADC is 10 bit resolution and range from 0V to VREFP(3.3V).
 */
struct file_operations hndl_battery_fops =
{
        .owner =    THIS_MODULE,
        .open =     battery_open,
        .release =  battery_release,
        .read = battery_read,
        .ioctl = battery_ioctl,
};

static struct proc_item battery_manage_items[] = 
{
        [0] = {
                .name = "battery_supply", //电池关断信号输出。电池供电时，通信模块发送完相应的数据后用以关断电池供电
                .pin = AT91_PIN_PA11,
                .settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
                .read_func = hnos_proc_gpio_get,
                .write_func = hnos_proc_gpio_set,
        },
        [1] = {
                .name = "battery_voltage", //检测电池输入口
                .pin = 0,    //AT91_PIN_PC0,
                .read_func = battery_proc_voltage_get,		
        },
        [2] = {
                .name = "battery_charge", 
                .pin = BATTERY_CHARGE_CONTRL,
                .settings = GPIO_OUTPUT_MASK | GPIO_OUTPUT_HIGH, /* output, init value 1 */
                .read_func = hnos_proc_gpio_get,
                .write_func = hnos_proc_gpio_set,
        },	
        [3] = {
                .name = "vcc5v_adc",
                .pin =0, ////AT91_PIN_PC2
                .read_func = vcc5v_proc_adc_get,
        },
        {NULL}
};

static void vcc5v_adc_value(int *voltage)
{
    if (0 != adc_channel_read(ADC_CH_VCC5V, voltage)) {
	    *voltage = ADC_RESLT_INVALID;
	}
    return;
}

static int vcc5v_proc_adc_get(struct proc_item *item, char *page)
{
	int voltage = 0;
	unsigned int len = 0;

    if(vcc5v_detect->available == VCC5V_AVAILABLE){
            vcc5v_adc_value(&voltage);
	}
	else{
		    voltage = ADC_RESLT_INVALID;
	}   
	    
	len = sprintf(page, "%d\n", voltage);
	return len;
}



int vcc5v_poweroff(void)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&battery_dev->lock,flags);
    if(vcc5v_detect->available == VCC5V_AVAILABLE){
        at91_set_gpio_input(AT91_PIN_PC2,0);
        ret = at91_get_gpio_value(AT91_PIN_PC2);
        if(ret < 0 )
            ret = 0;
	}
	spin_unlock_irqrestore(&battery_dev->lock,flags);

	return ret;
	    
}
EXPORT_SYMBOL(vcc5v_poweroff);

static void vcc5v_detect_input(void)
{
    u8 k;
    u8 invalid_cnt =0;
    int vcc5v_adc = 0;
   
    for(k=0;k<VCC5V_DETECT_MAX;k++){
        vcc5v_adc_value(&vcc5v_adc);
        if(vcc5v_adc < VCC5V_MIN_VALUE ){
            invalid_cnt++;
        }    
    }
    if(invalid_cnt > 3)
        vcc5v_detect->available = VCC5V_INVALID;
    else             
        vcc5v_detect->available =VCC5V_AVAILABLE;
    
    return;
}


int battery_open(struct inode *inode, struct file *filp)
{
        struct hndl_battery_cdev *dev; 
        dev = container_of(inode->i_cdev, struct hndl_battery_cdev, cdev);
        filp->private_data = dev; 
        if (test_and_set_bit(0,&dev->is_open)!=0) {
                return -EBUSY;       
        }
        return 0; 
}

int battery_release(struct inode *inode, struct file *filp)
{
        struct hndl_battery_cdev *dev = filp->private_data; 

        if (test_and_clear_bit(0, &dev->is_open) == 0) {
                return -EINVAL; 
        }

        return 0;
}

ssize_t battery_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos)
{
        unsigned char redata[4] = {0};
        int k,len;
        unsigned int voltage_value = 0;

        k = sizeof(unsigned int);
        len=(count > k)? k : count;

        if (0 != adc_channel_read(ADC_CH_BATTERY, &voltage_value))
                voltage_value = ADC_RESLT_INVALID;

        memcpy(&redata[0],&voltage_value,len);

        if (copy_to_user(buf,redata,len)){
                return (-EFAULT);
        }
        return len;   
}

static int battery_ioctl (struct inode *inode, struct file *filp,
                unsigned int cmd, unsigned long arg)
{
        int ret = 0;
        u8 charge_opt = ((arg & 0xff)? 1 : 0); //电池充电开关 0:关       1：开
        struct hndl_relay_cdev *dev = filp->private_data; /* device information */

        if (!dev) {
                printk("%s: no such device.\n", __FUNCTION__);
                return -ENODEV;
        }
        switch(cmd){
                case IOC_BATTERY_CHARGE://是否给电池充电
                        spin_lock(&battery_dev->lock);
                        at91_set_gpio_value(BATTERY_CHARGE_CONTRL,charge_opt);
                        spin_unlock(&battery_dev->lock);
                        break;
                case IOC_BATTERY_ENABLE://是否使用电池供电
                        spin_lock(&battery_dev->lock);
                        at91_set_gpio_value(AT91_PIN_PA11,charge_opt);
                        spin_unlock(&battery_dev->lock);
                        break;
                default:
                        break;
        }
        return ret;
}

static int battery_proc_voltage_get(struct proc_item *item, char *page)
{
	int voltage = 0;
	unsigned int len = 0;
    int charge_state =0;

	charge_state = at91_get_gpio_value(BATTERY_CHARGE_CONTRL);
	if(charge_state)
		at91_set_gpio_value(BATTERY_CHARGE_CONTRL,0);/*测量电压时，先关闭充电功能*/

	if (0 != adc_channel_read(ADC_CH_BATTERY, &voltage))
		voltage = ADC_RESLT_INVALID;

	if(charge_state)
		at91_set_gpio_value(BATTERY_CHARGE_CONTRL,1);

	len = sprintf(page, "%d\n", voltage);
	return len;

}

static int __init  battery_proc_devices_add(void)
{
        struct proc_item *item;
        int ret = 0;

        for (item = battery_manage_items; item->name; ++item) {
                if(item->pin)
                        at91_set_GPIO_periph(item->pin,0);

                hnos_gpio_cfg(item->pin, item->settings);
                ret += hnos_proc_entry_create(item);
        }

        return ret;
}

static int battery_proc_devices_remove(void)
{
        struct proc_item *item;

        for (item = battery_manage_items; item->name; ++item) {
                remove_proc_entry(item->name, hndl_proc_dir);
        }

        return 0;
}


static void  battery_hndl1000_module_exit(void)
{
        dev_t devno = MKDEV(battery_major, battery_minor);    
        struct class * myclass;

        if (hndl_proc_dir) {
                battery_proc_devices_remove();
                hnos_proc_rmdir();
        }
        
        if(vcc5v_detect){
    	    kfree(vcc5v_detect);
		    vcc5v_detect = NULL;
	    }
        if (battery_dev){

                /* Get rid of our char dev entries */    
                cdev_del(&battery_dev->cdev); 

                myclass = battery_dev->myclass;
                if (myclass){
                        class_device_destroy(myclass, devno);
                        class_destroy(myclass);
                }

                kfree(battery_dev);
                battery_dev = NULL;
        }
        /* cleanup_module is never called if registering failed */
        unregister_chrdev_region(devno, 1);

        HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management exit.\n");
        return ;
}

/* proc module init */
static int __init battery_hndl1000_module_init(void)
{
        int result = 0;
        dev_t dev = 0;
        struct class *myclass;

        /*
         * Get a range of minor numbers to work with, asking for a dynamic
         * major unless directed otherwise at load time.
         */
        at91_set_A_periph(AT91_PIN_PC0,0);
        at91_set_A_periph(AT91_PIN_PC2,0);

        if (battery_major){ 
                dev = MKDEV(battery_major, battery_minor);
                result = register_chrdev_region(dev, 1, DEVICE_NAME);
        } 
        else{ 

                result = alloc_chrdev_region(&dev, battery_minor, 1, DEVICE_NAME);
                battery_major = MAJOR(dev);
        }
        if (result < 0) {
                printk(KERN_WARNING "BATTERY: can't get major %d\n", battery_major);
                return result;
        }
        /************/        

        battery_dev = kmalloc(sizeof(struct hndl_battery_cdev), GFP_KERNEL);
        if (!battery_dev) {
                pr_debug("BATTERY: can't alloc battery_dev\n");
                result = -ENOMEM;
                goto fail;  /* Make this more graceful */
        }
        memset(battery_dev, 0, sizeof(struct hndl_battery_cdev)); 

        spin_lock_init(&battery_dev->lock);


        /* Register a class_device in the sysfs. */
        myclass = class_create(THIS_MODULE, DEVICE_NAME);
        if (myclass == NULL) {
                pr_debug("BATTERY: can't creat class\n");
                result = -ENODEV;
                goto fail;
        }
        class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
        battery_dev->myclass = myclass;
        cdev_init(&battery_dev->cdev, &hndl_battery_fops);
        battery_dev->cdev.owner = THIS_MODULE;
        result = cdev_add(&battery_dev->cdev, dev, 1);
        if (result) {
                printk(KERN_NOTICE "Error %d adding battery device, major_%d.\n", result, MAJOR(dev));
                goto fail;
        }   

        hndl_proc_dir = hnos_proc_mkdir();
        if (!hndl_proc_dir) {
                result = -ENODEV;
                goto fail;
        } else {
                result = battery_proc_devices_add();
                if (result) {
                        goto  fail;
                }
        }

        vcc5v_detect = kmalloc(sizeof(struct hndl_vcc5v_detect), GFP_KERNEL);
	    if (!vcc5v_detect) {
		    pr_debug("BATTERY: can't alloc hndl_vcc5v_detect\n");
		    result = -ENOMEM;
		    goto fail;  /* Make this more graceful */
	    }
	    memset(vcc5v_detect, 0, sizeof(struct hndl_vcc5v_detect));
	    vcc5v_detect_input();

        HNOS_DEBUG_INFO("Proc Filesystem Interface of Battery/Power Management init.\n");
        return 0;

fail:
        battery_hndl1000_module_exit();
        return result;
}

module_init(battery_hndl1000_module_init);
module_exit(battery_hndl1000_module_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

