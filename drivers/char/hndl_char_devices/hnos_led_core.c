/*
 * drivers/char/hndl_char_devices/hnos_led_core.c
 * 
 * LED control.
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
#include "hnos_ioctl.h"              /* ioctl */
#include "hnos_led.h"                /* led  */
#include "hnos_hndl1000x.h"

#define LED_TIME_OUT   (HZ/2)		/* blinking timeout: default 500ms  */
#define DEVICE_NAME   "led"

static int led_major =   0;
static int led_minor =   0;
module_param(led_major, int, S_IRUGO);
module_param(led_minor, int, S_IRUGO);

struct at91_led_cdev  *led_device;

static LIST_HEAD(leds);
static DECLARE_MUTEX(leds_lock);
static int nr_leds = 0;
static int at91_led_light_generic(struct at91_led_object *led, int action);

static struct at91_led_object *at91_led_find_obj(unsigned int num)
{
	struct at91_led_object *led = NULL;

	down(&leds_lock);
	list_for_each_entry(led, &leds, list) 
	{
    	if(led->id == num) 
		{
        	dprintk("%s:find led: id %d.\n", __FUNCTION__, led->id);
            goto out;
        }
    }

    dprintk("%s: NO led found.\n", __FUNCTION__);
    led = NULL;
out:
    up(&leds_lock);
    return led;
}

int at91_led_add(struct at91_led_object *led) 
{
	if(led == NULL) 
	{
    	printk("%s: NULL led object.\n", __FUNCTION__);
        return -1;
    }
    HNOS_DEBUG_INFO("Register led %i.\n", led->id);

    spin_lock_init(&led->lock);
    init_timer(&led->timer);
    INIT_LIST_HEAD(&led->list);

    if(led->at91_led_light == NULL)
	{
    	led->at91_led_light = at91_led_light_generic;
    }
    led->blinking_timeout = LED_TIME_OUT;
    led->is_timer_started = 0;
    led->last_stat = 0;

    down(&leds_lock);
    list_add_tail(&led->list, &leds);
    nr_leds ++;
    up(&leds_lock);

    return 0;
}
EXPORT_SYMBOL(at91_led_add);

int at91_led_remove(struct at91_led_object *led) 
{
	if(led == NULL) 
	{
    	printk("%s: NULL led object.\n", __FUNCTION__);
    	return -1;
    }

	if(led->is_timer_started) 
	{
    	del_timer_sync(&led->timer);
    }

   	down(&leds_lock);
    list_del(&led->list);
    nr_leds --;
    up(&leds_lock);

    return 0;
}
EXPORT_SYMBOL(at91_led_remove);

static int at91_led_light_generic(struct at91_led_object *led, int action)
{
	int value = 1;
    unsigned pin = 0;

    if(unlikely(!led)) 
	{
    	printk("Empty led object, something error.\n");
    	return -1;
    } 
    if(unlikely((LED_ON != action) && (LED_OFF != action))) 
	{
    	printk("%s: Invalid argument %d.\n", __FUNCTION__, action);
        return -1;
    }

    pin = led->pin;

    if(LOW == led->logic_level) 
	{       /* Low is available */
    	if(LED_ON == action) 
		{
        	value = 0;
        } 
		else 
		{
        	value = 1;
        }
	} 
	else if(HIGH == led->logic_level) 
	{   /* High is available */
    	if(LED_ON == action) 
		{
        	value = 1;
        } 
		else 
		{
        	value = 0;
        }
    }

    at91_set_gpio_output(pin, value);
    return 0;
}

void at91_led_off_all(void)
{
	int i = 0;
	struct at91_led_object *led = NULL;

	for(i = 0; i < nr_leds; i++)
	{
    	led = at91_led_find_obj(i);
        if(led) 
		{
        	led->at91_led_light(led, LED_OFF);
        }
    }

    return;
}
EXPORT_SYMBOL(at91_led_off_all);

/* Whether or not the LED blinking timer is started. */
static unsigned int at91_led_is_blinking (struct at91_led_object *led)
{
	return led->is_timer_started;
}

static void at91_led_timeout_set(struct at91_led_object *led, unsigned long timeout)
{
	unsigned long flags;

	if(0 == timeout) 
	{
    	timeout = 1;
    }

    if(led) 
	{
    	spin_lock_irqsave(&led->lock, flags);
    	led->blinking_timeout = timeout;	
     	spin_unlock_irqrestore(&led->lock, flags);
    }
    return;
}

static void at91_led_blinking_timer(unsigned long data)
{
	struct at91_led_object *led = (struct at91_led_object *)data;
    int action = LED_ON;
    unsigned long flags;

    dprintk("at91_led_blinking_timer: led id %d\n", led->pin);
    spin_lock_irqsave(&led->lock, flags);

    led->last_stat = led->last_stat ^ 1;
    action = led->last_stat ? LED_ON : LED_OFF;
    led->at91_led_light(led, action);

    spin_unlock_irqrestore(&led->lock, flags);

    /* resubmit the timer again */
    led->timer.expires = jiffies + led->blinking_timeout;
    add_timer(&led->timer);

    return;
}

static int at91_led_blinking_start(struct at91_led_object *led)
{
	int ret = 0;
	unsigned long flags;

    spin_lock_irqsave(&led->lock, flags);
    if(led->is_timer_started) 
	{
    	dprintk("led %d, timer already started.\n", led->pin);
        ret  = -1;
        goto out;
    }

    dprintk("led %d, timer start.\n", led->pin);

    /* submit the timer. */
    init_timer(&led->timer);

    led->timer.function = at91_led_blinking_timer;
    led->timer.data = (unsigned long)led;
    led->timer.expires = jiffies + led->blinking_timeout;

    add_timer(&led->timer);

    led->is_timer_started = 1;
out:
    spin_unlock_irqrestore(&led->lock, flags);
    return ret;
}

static int at91_led_blinking_stop(struct at91_led_object *led)
{
    int ret = 0;
    unsigned long flags;

    if(!led->is_timer_started) 
	{
    	dprintk("led %d, timer not started.\n", led->pin);
        ret = -1;
        goto out;
    }

    dprintk("led %d, timer stop.\n", led->pin);
    del_timer_sync(&led->timer);

    spin_lock_irqsave(&led->lock, flags);
    led->is_timer_started = 0;
    spin_unlock_irqrestore(&led->lock, flags);

out:
    return ret;
}


static int at91_led_open(struct inode *inode, struct file *filp)
{
    struct at91_led_cdev *dev; 

    dev = container_of(inode->i_cdev, struct at91_led_cdev, cdev);
    filp->private_data = dev; /* for other methods */

    if(test_and_set_bit(0, &dev->is_open) != 0) 
	{
    	return -EBUSY;       
    }

    return 0; 
}

static int at91_led_release(struct inode *inode, struct file *filp)
{
    struct at91_led_cdev *dev = filp->private_data; 

    if(test_and_clear_bit(0, &dev->is_open) == 0) 
	{/* release lock, and check... */
    	return -EINVAL; /* already released: error */
    }

    return 0;
}

/*The ioctl() implementation */
static int at91_led_ioctl(struct inode *inode, struct file *filp,
                           unsigned int cmd, unsigned long arg)
{
	int ret = 0, err = 0, i = 0;
    struct at91_led_object *led = NULL;
    struct led_blinking_timeout timeout_data;
    unsigned long blinking_timeout = 0;

    int led_num = 0;

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if(_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC) 
	{
    	return -ENOTTY;
    }
    if(_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR ) 
	{
        return -ENOTTY;
    }

    /*
         * the type is a bitmask, and VERIFY_WRITE catches R/W
         * transfers. Note that the type is user-oriented, while
         * verify_area is kernel-oriented, so the concept of "read" and
         * "write" is reversed
         */
    if(_IOC_DIR(cmd) & _IOC_READ) 
	{
    	err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    } 
	else if(_IOC_DIR(cmd) & _IOC_WRITE) 
	{
    	err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if(err) 
	{
    	return -EFAULT;
    }

    dprintk("%s: cmd %d , led %2d \n", __FUNCTION__, cmd, (int)arg);

    switch(cmd) 
	{
    	case IOC_LED_ON:	/* LED on.*/
        	led_num = arg;
            led = at91_led_find_obj(led_num);
            if(!led) 
			{
            	printk("%s: no led device found.\n", __FUNCTION__);
                return -ENODEV;
            }

            if(at91_led_is_blinking(led)) 
			{ // This led is in blinkinging status.
            	printk("LED is blinking now, you should first stop blinking.\n");
                return -EFAULT;
            }

            dprintk("IOC_LED_ON.\n");
            ret = led->at91_led_light(led, LED_ON);
            break;

        case IOC_LEDS_ON:	/* LEDS on.*/
        	if(arg == 0)
			{
            	//printk("IOC_LEDS_ON 's arg can't be zero!\n");
                return -EINVAL;
            }
            for(i = 0; i < nr_leds; i++)
			{
            	if(arg & (1 << i))
				{
                	led = at91_led_find_obj(i);
                    if(!led) 
					{
                    	printk("%s: no led device found.\n", __FUNCTION__);
                        return -ENODEV;
                    } 

                    if(at91_led_is_blinking(led)) 
					{ // This led is in blinkinging status.
                    	printk("LED is blinking now, you should first stop blinking.\n");
                        return -EFAULT;
                    }

                    dprintk("IOC_LED_ON.\n");
                    ret = led->at91_led_light(led, LED_ON);
                    if(ret)
					{ 
                    	break;
                    }
                }
            }
            break;

        case IOC_LED_OFF:	/* LED off.*/
        	led_num = arg;
            led = at91_led_find_obj(led_num);
            if(!led) 
			{
            	printk("%s: no led device found.\n", __FUNCTION__);
                return -ENODEV;
            }

            if(at91_led_is_blinking(led)) 
			{ // This led is in blinkinging status.
            	printk("LED is blinking now, you should first stop blinking.\n");
                return -EFAULT;
            }

            dprintk("IOC_LED_OFF.\n");
            ret = led->at91_led_light(led, LED_OFF);
            break;

        case IOC_LEDS_OFF:	/* LEDS off.*/
        	if(arg == 0)
			{
            	//printk("IOC_LEDS_OFF 's arg can't be zero!\n");
                return -EINVAL;
            }
            for(i = 0; i < nr_leds; i++)
			{
            	if(arg & (1 << i))
				{
                	led = at91_led_find_obj(i);
                    if(!led) 
					{
                    	printk("%s: no led device found.\n", __FUNCTION__);
                        return -ENODEV;
                    } 

                    if(at91_led_is_blinking(led)) 
					{ // This led is in blinkinging status.
                    	printk("LED is blinking now, you should first stop blinking.\n");
                        return -EFAULT;
                    }

                    dprintk("IOC_LED_ON.\n");
                    ret = led->at91_led_light(led, LED_OFF);
                    if(ret)
					{ 
                    	break;
                    }
                }
            }
            break;

		case IOC_LED_BLINKING_START:	/* LED BLINKING start.*/
            led_num = arg;
            led = at91_led_find_obj(led_num);
            if(!led) 
			{
            	printk("%s: no led device found.\n", __FUNCTION__);
                return -ENODEV;
            }

            dprintk("IOC_LED_blinking start.\n");
            ret = at91_led_blinking_start(led);
            break;

        case IOC_LED_BLINKING_STOP:	/* LED BLINKING stop.*/
        	led_num = arg;
            led = at91_led_find_obj(led_num);
            if(!led) 
			{
            	printk("%s: no led device found.\n", __FUNCTION__);
                return -ENODEV;
            }

            dprintk("IOC_LED_blinking stop.\n");
            ret = at91_led_blinking_stop(led);
            break;

        case IOC_BLINKING_SET_TIMEOUT:
        	dprintk("IOC LED Set timeout %d\n",(int) arg);

            if(copy_from_user(&timeout_data, (struct led_blinking_timeout *)arg, 
                              sizeof(struct led_blinking_timeout))) 
            {
            	printk("Error ioctl cmd IOC_BLINKING_SET_TIMEOUT: invalid args.\n");
                return -EINVAL;
            }

            dprintk("led_blinking_timeout: led %d, timeout:%d ms\n", 
                    (int)timeout_data.led, (int)timeout_data.timeout);

            led = at91_led_find_obj(timeout_data.led);
            if(!led) 
			{
            	printk("%s: no led device found.\n", __FUNCTION__);
                return -ENODEV;
            }

            /* timeout unit: ms. */
            blinking_timeout = timeout_data.timeout;
            if(blinking_timeout < 10)
			{
            	blinking_timeout = 10; /* min 10ms */
            } 
			else if(blinking_timeout > 10000) 
			{	/* >10s ? Are you Kidding me? */
            	blinking_timeout = 10000;  /* max 10s */
            }
            at91_led_timeout_set(led, blinking_timeout/10);

            break;

        default:  /* redundant, as cmd was checked against MAXNR */
        	return -ENOTTY;
    }

    return ret;
}

struct file_operations led_fops =
{
    .owner =    THIS_MODULE,
    .open =     at91_led_open,
    .release =  at91_led_release,
    .ioctl = at91_led_ioctl,
};

static void  at91_led_cdev_setup(struct at91_led_cdev *dev, dev_t devno)
{
    int err;

    cdev_init(&dev->cdev, &led_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add (&dev->cdev, devno, 1);

    if(err) 
	{ 
    	printk(KERN_NOTICE "Error %d adding LED device, major_%d", err, MAJOR(devno));
    }
    return;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
void at91_led_module_cleanup(void)
{
    dev_t devno = MKDEV(led_major, led_minor);	
    struct class * myclass;

    if(led_device)
	{
    	/* Get rid of our char dev entries */	
        cdev_del(&led_device->cdev);	

        myclass = led_device->myclass;
        if(myclass)
		{
        	class_device_destroy(myclass, devno);
            class_destroy(myclass);
        }

        kfree(led_device);
        led_device = NULL;
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, 1);

    HNOS_DEBUG_INFO("Cleanup device %s, major %d.\n", DEVICE_NAME, led_major);
    return;
}

/*
 * Finally, the module stuff
 */
static int __init  at91_led_module_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *myclass = NULL;

	/*
         * Get a range of minor numbers to work with, asking for a dynamic
         * major unless directed otherwise at load time.
         */
	if(led_major) 
	{
		dev = MKDEV(led_major, led_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
    } 
	else 
	{
		result = alloc_chrdev_region(&dev, led_minor, 1, DEVICE_NAME);
		led_major = MAJOR(dev);
	}
	if(result < 0) 
	{
		printk(KERN_WARNING "at91_led: can't get major %d\n", led_major);
		return result;
	}	

	/* 
         * allocate the devices -- we do not have them static.
         */
	led_device = kmalloc(sizeof(struct at91_led_cdev), GFP_KERNEL);
	if(!led_device) 
	{
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(led_device, 0, sizeof(struct at91_led_cdev));	

	/* Register a class_device in the sysfs. */
	myclass = class_create(THIS_MODULE, DEVICE_NAME);
	if(myclass == NULL) 
	{
		goto fail;
	}

	class_device_create(myclass, NULL, dev, NULL, DEVICE_NAME);
	led_device->myclass = myclass;

	at91_led_cdev_setup(led_device, dev);	

	HNOS_DEBUG_INFO("Initialized device %s, major %d.\n", DEVICE_NAME, led_major);
	return 0;

fail:
	at91_led_module_cleanup();
	return result;
}

module_init(at91_led_module_init);
module_exit(at91_led_module_cleanup);

MODULE_LICENSE("Dual BSD/GPL");


