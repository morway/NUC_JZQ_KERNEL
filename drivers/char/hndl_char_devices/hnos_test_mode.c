/*
 *  drivers/char/hndl_char_devices/hnos_test_mode.c
 *
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

int tm_gpio_test(struct proc_item *item, const char __user * userbuf, unsigned long count);
static  struct proc_dir_entry	*hndl_proc_dir = NULL;

static char gpio_desc[6] = "PA27";
module_param_string(gpio_desc, gpio_desc, sizeof(gpio_desc), 0);
MODULE_PARM_DESC(gpio_desc, "GPIO usage, default PA27");

/* 
 * Default gpio PA27.
 * */
static int gpio = AT91_PIN_PA27;

/* 
 * PA27 : 1, test mode.
 * PA27 : 0, normal mode.
 * */
static struct proc_item items[] = 
{
	{
		.name = "test_mode",
		.pin = AT91_PIN_PA27,
		.read_func = hnos_proc_gpio_get,
		.write_func = tm_gpio_test,
	},
	{NULL},
};

#define		TM_TIMER_FREQ	50	/* 0.5 s */

static DEFINE_SPINLOCK(lock);
static unsigned long timer_start = 0;
static unsigned int timer_freq = TM_TIMER_FREQ;
static struct timer_list tm_timer;

//static unsigned int test_pins[] =
//{
//	AT91_PIN_PA0,		/* PA0/SPI0_MISO/MCDB0B */
//	AT91_PIN_PA1,		/* PA1/SPI0_MOSI/MCCDB */
//	AT91_PIN_PA2,		/* PA2/SPI0_SPCK */
//	AT91_PIN_PA3,		/* PA3/SPI0_NPCS0/MCDB3 */
//	AT91_PIN_PA6,		/* PA6/MCDA0 */
//	AT91_PIN_PA7,		/* PA7/MCCDA */
//	AT91_PIN_PA8,		/* PA8/MCCK */
//	AT91_PIN_PA9,		/* PA9/MCDA1 */
//	AT91_PIN_PA10,		/* PA10/MCDA2/ETX2 */
//	AT91_PIN_PA11,		/* PA11/MCDA3/ETX3 */
//	AT91_PIN_PA12,		/* ETX0/ PA12 */
//	AT91_PIN_PA13,		/* ETX1/ PA13 */
//	AT91_PIN_PA14,		/* ERX0/PA14 */
//	AT91_PIN_PA15,		/* ERX1/PA15 */
//	AT91_PIN_PA16,		/* ETXEN/PA16 */
//	AT91_PIN_PA17,		/* ERXDV/PA17 */
//	AT91_PIN_PA18,		/* ERXER/PA18 */
//	AT91_PIN_PA19,		/* ETXCK/PA19 */
//	AT91_PIN_PA20,		/* EMDC/PA20 */
//	AT91_PIN_PA21,		/* EMDIO/PA21 */
//	AT91_PIN_PA23,		/* ETX2/TWD/PA23 */
//	AT91_PIN_PA24,		/* EXT3/TWCK/PA24 */
//
//	AT91_PIN_PB0,		/* PB0/SPI1_MISO/TIOA3 */
//	AT91_PIN_PB1,		/* PB1/SPI1_MOSI/TIOB3 */
//	AT91_PIN_PB2,		/* PB2/SPI1_SPCK/TIOA4 */
//	AT91_PIN_PB3,		/* PB3/SPI1_NPCS0/TIOA5 */
//	AT91_PIN_PB16,		/* PB16/TCLK3 */
//	AT91_PIN_PB17,		/* PB17/TCLK4 */
//	AT91_PIN_PB18,		/* PB18/TIOB4 */
//	AT91_PIN_PB19,		/* PB19//TIOB4 */
//	AT91_PIN_PB22,		/* PB22/DSR0 */
//	AT91_PIN_PB23,		/* PB23/DCD0 */
//	AT91_PIN_PB24,		/* PB27/DTR0 */
//	AT91_PIN_PB25,		/* PB25/RI0/ISI_D5 */
//	AT91_PIN_PB26,		/* PB26/RTS0 */
//	AT91_PIN_PB27,		/* PB27/CTS0 */
//
//	AT91_PIN_PC0,		/* SCK3/AD0/PC0 */
//	AT91_PIN_PC1,		/* PCK0/AD1/PC1 */
//	AT91_PIN_PC2,		/* PCK1/AD2/PC2 */
//	AT91_PIN_PC3,		/* SPI1_NPCS3/AD3/PC3 */
//};

static unsigned int test_pins[] =
{
	//第一排从上到下，红灯
	AT91_PIN_PC12,
	AT91_PIN_PB19,	
	//NRST
	AT91_PIN_PB17,	
	AT91_PIN_PB18,	
	AT91_PIN_PB16,	
	AT91_PIN_PB9 ,	
	AT91_PIN_PB8 ,	
	AT91_PIN_PB1 ,
	AT91_PIN_PB2 ,	
	AT91_PIN_PB3 ,	
	AT91_PIN_PB0 ,
	AT91_PIN_PB6 ,	
//第二排从上到下，白灯
	AT91_PIN_PA16,
	AT91_PIN_PA17,	
	AT91_PIN_PA18,
	AT91_PIN_PA19,	
	AT91_PIN_PA20,	
	AT91_PIN_PA21,	
	AT91_PIN_PA24,	
	AT91_PIN_PA23,	
	AT91_PIN_PA22,
//AT91_PIN_PA27,	测试使能脚
	AT91_PIN_PA31,	
	AT91_PIN_PA30,
	AT91_PIN_PB7,
//第三排从上到下，黄灯   	
  AT91_PIN_PA15,
	AT91_PIN_PA14,
	AT91_PIN_PA13,
	AT91_PIN_PA12,
	AT91_PIN_PB30,
	AT91_PIN_PB21,
	AT91_PIN_PB20,
	AT91_PIN_PB11,
	AT91_PIN_PB10,
	AT91_PIN_PC3 ,
	AT91_PIN_PC1 ,
	AT91_PIN_PC2 ,
	AT91_PIN_PC0 ,
//第四排从上到下，绿灯   	
  AT91_PIN_PB25,
	AT91_PIN_PA3,
	AT91_PIN_PA1,
	AT91_PIN_PA0,
	AT91_PIN_PA2,
	//AT91_PIN_PA9, //硬件看门狗
	AT91_PIN_PA10,
	AT91_PIN_PA11,
	AT91_PIN_PA6,
	AT91_PIN_PA7 ,
	AT91_PIN_PA8 ,
	AT91_PIN_PB12,
	AT91_PIN_PB13,
//第五排从上到下，红灯   	
  AT91_PIN_PB23,
	AT91_PIN_PB22,
	AT91_PIN_PB24,
	AT91_PIN_PB27,
	AT91_PIN_PB26,
	AT91_PIN_PB5,
	AT91_PIN_PB4,
	//1.8V
	AT91_PIN_PC5,
	AT91_PIN_PC6 ,
	AT91_PIN_PC4 ,
	AT91_PIN_PC10,
	AT91_PIN_PC8 ,
};


static void tm_timer_func(unsigned long data)
{
	int i = 0;
	static u8 gpio_value = 1;
  static u8 gpio_high_count = 0;
   static u8 cur_pin = 0; 
  
//	gpio_high_count++;	
	
	gpio_value ^= 1;

		at91_set_gpio_value(test_pins[cur_pin], 0);
			
		if (cur_pin<ARRAY_SIZE(test_pins))
		   cur_pin++;	
		else
			{
				cur_pin = 0;
		     for (i=0; i<ARRAY_SIZE(test_pins); i++)
		     {
		        at91_set_gpio_value(test_pins[i], 1);
       	 }
			}	
	
	/* resubmit the timer again */
	tm_timer.expires = jiffies + timer_freq;
	add_timer(&tm_timer);

	return;
}

static int tm_timer_start(void)
{
	int ret = 0;
	unsigned long flags;
	int i = 0;

	spin_lock_irqsave(&lock, flags);
	if (timer_start == 1) {
		printk(KERN_WARNING "Warning: tm_timer already started.\n");
		spin_unlock_irqrestore(&lock, flags);
		ret = -1;
	} else {
		timer_start = 1;
		spin_unlock_irqrestore(&lock, flags);

		for (i=0; i<ARRAY_SIZE(test_pins); i++) {
			
			at91_set_gpio_output(test_pins[i], 1);
		} 
 
		dprintk("%s: start tm_timer.\n", __FUNCTION__);
		init_timer(&tm_timer);
		tm_timer.expires = jiffies + timer_freq;
		tm_timer.function = tm_timer_func;
		add_timer(&tm_timer);
	}

	return ret;
}

static int tm_timer_stop(void)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&lock, flags);
	if (timer_start == 0) {
		printk(KERN_WARNING "Warning: tm_timer already stopped.\n");
		spin_unlock_irqrestore(&lock, flags);
		ret = -1;
	} else {
		timer_start = 0;
		spin_unlock_irqrestore(&lock, flags);

		dprintk("%s: stop tm_timer.\n", __FUNCTION__);
		del_timer_sync(&tm_timer);
	}

	return ret;

}

int tm_gpio_test(struct proc_item *item, const char __user * userbuf, unsigned long count) 
{
	int start = 0;
	char val[20] = {0};

	if (count >= 20){
		return -EINVAL;
	}

	if (copy_from_user(val, userbuf, count)) {
		return -EFAULT;
	}

	if (sscanf(val, "gpio-test %d", &start) != 1){
		return -EINVAL;
	}

	dprintk(KERN_INFO "%s: buf %s, start = %d\n", __FUNCTION__, val, start);

	if (start == 1) {
		tm_timer_start();
	} else {
		tm_timer_stop();
	}

	return 0;
}


static int __init  tm_devices_add(void)
{
	int ret = 0;
	struct proc_item *item;

	for (item=items; item->name; ++item) {
		hnos_gpio_cfg(item->pin, item->settings);
		ret += hnos_proc_entry_create(item);
	}
	return ret;
}

static int tm_devices_remove(void)
{
	struct proc_item *item;

	for (item=items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}


/* proc module init */
static int __init tm_module_init(void)
{
	int status;

	HNOS_DEBUG_INFO("Proc Filesystem Interface for Test Mode init.\n");

    gpio = hnos_gpio_parse(gpio_desc, sizeof(gpio_desc));
    if (gpio < 0) {
        return -EINVAL;
    }
    items[0].pin = gpio;

	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		status = -ENODEV;
	} else {
		status = tm_devices_add();
		if (status) {
			hnos_proc_rmdir();
		}
	}

	return (!status) ? 0 : -ENODEV;
}

static void tm_module_exit(void)
{
	tm_timer_stop();
	tm_devices_remove();
	hnos_proc_rmdir();

	HNOS_DEBUG_INFO("Proc Filesystem Interface for Test Mode exit.\n");
	return;
}

module_init(tm_module_init);
module_exit(tm_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

