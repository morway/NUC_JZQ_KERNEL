/*
 *  drivers/char/hndl_char_devices/hnos_rmc_hntt1000x.c
 *
 *  For HNTT1000x .
 *
 *  Author ZhangRM,peter_zrm@163.com
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
#include "hnos_hndl1000x.h"


#define  SCAN_TIMEOUT          7

struct detect_driver
{
	u8 commu_state;            
	struct timer_list timer;
	unsigned long timeout;
};
struct detect_driver *detect_dev = NULL;

static void timer_hndl1000x_opt(unsigned long data)
{
	rmi_detect_opt(SCAN_FIRST);//输入检测
	kbd_hndl1000x_scan();//键盘扫描
	smcbus_auto_write();//输出重置
	parallel2serail_led_autoscan();//595控制的串转并行的led

	detect_dev->timer.expires = jiffies + detect_dev->timeout;
	add_timer(&detect_dev->timer);
	rmi_detect_opt(SCAN_SECOND);//输入检测
}

static void __devexit timer_hndl1000x_remove(void)
{
	if(detect_dev){    
		del_timer_sync(&detect_dev->timer);
		kfree(detect_dev);
	}

}

static int __devinit timer_hndl1000x_init(void)
{

	if (!(detect_dev = kzalloc(sizeof(struct detect_driver), GFP_KERNEL))) {
		return( -ENOMEM);
	}        
	detect_dev->timeout = SCAN_TIMEOUT;
	init_timer(&detect_dev->timer);
	detect_dev->timer.function = timer_hndl1000x_opt;
	detect_dev->timer.data = (unsigned long)detect_dev;
	detect_dev->timer.expires = jiffies + detect_dev->timeout;
	add_timer(&detect_dev->timer); 
	HNOS_DEBUG_INFO("timer_hndl1000x registered.\n");
	return 0;
}

module_init(timer_hndl1000x_init);
module_exit(timer_hndl1000x_remove);
MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
