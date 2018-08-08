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
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include "hnos_output.h" 
#include "hnos_iomem.h"                /* iomem object  */
#include "hnos_hndl1000x.h"


#define	OUTPUT_SMCBUS_OFFSET	8              /* (总线扩展)遥控输出从第16路开始 */
#define	OUTPUT_SMCBUS_SIZE	24              /* (总线扩展)遥信共计16路 */ 
#define NCHANNEL_PER_SMCBUS	8
#define NR_SMCBUS		3

static struct iomem_object *bases[NR_SMCBUS] = {NULL,NULL,NULL};

static struct hndl_rmc_device *rmc_hndl1000x;
struct smcbus_rmc_data rmc_smcbus_hndl1000x;

static struct proc_item items_hndl1000x[] = 
{
	{
		.name = "PRC_OUT", 
		.pin = AT91_PIN_PB19,
		.settings = GPIO_OUTPUT_MASK , /* output,  */
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "P_OUT", 
		.pin = AT91_PIN_PA20,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "Q_OUT", 
		.pin = AT91_PIN_PA21,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},
	{
		.name = "RELAY_OUTPUT_ENABLE", 
		.pin = AT91_PIN_PA0,
		.settings = GPIO_OUTPUT_MASK , 
		.write_func = hnos_proc_gpio_set,
		.read_func = hnos_proc_gpio_get,
	},	        
	{ 
		.name = "LED_ENABLE",         
		.pin = AT91_PIN_PA1,        
		.settings = GPIO_OUTPUT_MASK ,           
		.write_func = hnos_proc_gpio_set,        
		.read_func = hnos_proc_gpio_get,       
	},
	{ 
		.name = "LED_RUN",         
		.pin = AT91_PIN_PA19,        
		.settings = GPIO_OUTPUT_MASK ,           
		.write_func = hnos_proc_gpio_set,        
		.read_func = hnos_proc_gpio_get,       
	},
	{ 
		.name = "TEMP_COMPENSATE",         
		.pin = AT91_PIN_PA15,        
		.settings = GPIO_OUTPUT_MASK ,           
		.write_func = hnos_proc_gpio_set,        
		.read_func = hnos_proc_gpio_get,       
	},	

};

static struct gpio_rmc_data gpio_hndl1000x =
{
	.items = items_hndl1000x,
	.size = ARRAY_SIZE(items_hndl1000x),
};

static int __devinit gpio_channels_init(struct hndl_rmc_device *output)
{
	u8 offset = 0;
	u8 size = ARRAY_SIZE(items_hndl1000x);
	return rmc_gpio_register(output, &gpio_hndl1000x, offset, size);
}

/* SMCBUS read not supported by the hardware. */
static int smcbus_read(struct smcbus_rmc_data *bus, u32 *reslt)
{	
	int ret =0;
	u8 ch0 = 0, ch1 = 0, ch2 = 0;        

	ret = bases[0]->read_byte(bases[0], &ch0, IO_WRONLY);
	ret = bases[1]->read_byte(bases[1], &ch1, IO_WRONLY);
	ret = bases[2]->read_byte(bases[2], &ch2, IO_WRONLY);

	return ( ( ch2 << 16 ) | (ch1 << 8) | ch0);
}

static inline int smcbus_write_channel(struct iomem_object *base, u8 bitmap, int is_set)
{
	return base->write_bit(base, bitmap, is_set);
}

static int smcbus_write_bit(struct smcbus_rmc_data *bus, u32 bitmap, int is_set)
{
	int ret = 0;

	ret = smcbus_write_channel(bases[0], (bitmap & 0xff), is_set);
	ret |= smcbus_write_channel(bases[1], ((bitmap >> 8) & 0xff), is_set);
	ret |= smcbus_write_channel(bases[2], ((bitmap >> 16) & 0xff), is_set);
	if(ret)
		printk("smcbus_write_bit ret= %x \n",  ret);	
	return ret;
}


void smcbus_auto_write(void)
{               
	int ret,i; 
	u8 write_value = 0;
	for(i =0;i<NR_SMCBUS;i++){
		if( bases[i]){
			ret = bases[i]->read_byte(bases[i], &write_value, IO_WRONLY);
			ret = bases[i]->write_byte(bases[i], write_value);
		}
	}

	return;

}
EXPORT_SYMBOL(smcbus_auto_write);


static int smcbus_proc_read(struct smcbus_rmc_data *bus, char *buf)
{	
	int len = 0,i,k=0;
	int reslt = smcbus_read(bus,0);
    char *OutputName[NR_SMCBUS]={"RELAY_CS1","RELAY_CS2","GPRS_LCD_BEEP_CS"};
    
	for(i=0;i<OUTPUT_SMCBUS_SIZE;i++){
		if((i+1)%8 == 0){
			len += sprintf(buf + len, "%d:%d,\n", i,(reslt & (1<<i))?1:0);
            len += sprintf(buf + len, "%s = %02x\n",OutputName[k],(reslt >> (8*k)) & 0xff);
            k++;
		}else
			len += sprintf(buf + len, "%d:%d,\t", i,(reslt & (1<<i))?1:0);
	}
	len += sprintf(buf + len, "\noutput0 = %02x\n",reslt);
	return len;
}

static int smcbus_proc_write(struct smcbus_rmc_data *bus, const char __user *userbuf, unsigned long count)
{
	u32 value = 0;
	char val[14] = {0};       
	int ret = 0;        
	u8 write_value =0;

	if (count >= 14){
		return -EINVAL;
	}
	if (copy_from_user(val, userbuf, count)){
		return -EFAULT;
	}
	value = (unsigned int)simple_strtoull(val, NULL, 16);

	//==============负控扩展总线操作
	write_value = (value & 0xff);
	ret = bases[0]->write_byte(bases[0], write_value);
	write_value = ((value >> 8) & 0xff);
	ret = bases[1]->write_byte(bases[1], write_value);
	//===============

	write_value = ((value >> 16) & 0xff);
	ret = bases[2]->write_byte(bases[2], write_value);

	return 0;
}
static void  smcbus_channels_unregister(struct hndl_rmc_device *output)
{
	rmc_smcbus_unregister(output, &rmc_smcbus_hndl1000x);

	iomem_object_put(bases[0]);
	iomem_object_put(bases[1]);
	iomem_object_put(bases[2]);

	return;
}

static int __devinit smcbus_channels_init(struct hndl_rmc_device *output)
{
	int reslt = -1;

	bases[0] = iomem_object_get(RELAY_CS1, 0);
	if (!bases[0]) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) RELAY_CS1);
		return -1;		
	}
	bases[1] = iomem_object_get(RELAY_CS2, 0);
	if (!bases[1]) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) RELAY_CS2);

		reslt = -1;
		goto base0_request_failed;		

	}

	bases[2] = iomem_object_get(GPRS_LCD_BEEP_CS, 0);

	if (!bases[2]) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) GPRS_LCD_BEEP_CS);
		reslt = -1;
		goto base1_request_failed;		
	}	

	rmc_smcbus_hndl1000x.read = smcbus_read;
	rmc_smcbus_hndl1000x.smcbus_stat = 0xffffffff;
	rmc_smcbus_hndl1000x.write = smcbus_write_bit;
	rmc_smcbus_hndl1000x.proc_read = smcbus_proc_read;
	rmc_smcbus_hndl1000x.proc_write = smcbus_proc_write;
	reslt = rmc_smcbus_register(output, &rmc_smcbus_hndl1000x, 
			OUTPUT_SMCBUS_OFFSET, OUTPUT_SMCBUS_SIZE);

	if (reslt < 0) {
		goto base2_request_failed;
	}
	return 0;

base2_request_failed:
	iomem_object_put(bases[2]);
base1_request_failed:
	iomem_object_put(bases[1]);
base0_request_failed:
	iomem_object_put(bases[0]);
	HNOS_DEBUG_INFO("rmc_smcbus_channels_init: fail.\n");
	return reslt;
}

static void  rmc_hndl1000x_remove(void)
{
	rmc_gpio_unregister(rmc_hndl1000x, &gpio_hndl1000x);
	smcbus_channels_unregister(rmc_hndl1000x);
	rmc_device_unregister(rmc_hndl1000x);
	rmc_device_free(rmc_hndl1000x);
	return;
}

static int __devinit rmc_hndl1000x_init(void)
{       int ret = 0;
	rmc_hndl1000x = rmc_device_alloc();
	if (!rmc_hndl1000x) {
		return -1;
	}
	ret = gpio_channels_init(rmc_hndl1000x);
	if(ret <0 )
		goto   gpio_init_fail;
	ret = smcbus_channels_init(rmc_hndl1000x);
	if(ret <0 )
		goto   smcbus_init_fail;
	ret = rmc_device_register(rmc_hndl1000x);
	if(ret <0 )
		goto   device_init_fail;

#ifdef HW_DEBUG_OPT
	ret = bases[2]->write_bit(bases[2], 0x08, 0);
	ret = bases[2]->write_bit(bases[2], 0x10, 0);
	ret = bases[2]->write_bit(bases[2], 0x20, 1);
	ret = bases[2]->write_bit(bases[2], 0x40, 1);
	ret = bases[2]->write_bit(bases[2], 0x80, 1);
#endif 


	HNOS_DEBUG_INFO("rmc_hndl1000x registered ok \n");

	return 0;

gpio_init_fail:
	rmc_device_free(rmc_hndl1000x);   
smcbus_init_fail:
	rmc_gpio_unregister(rmc_hndl1000x, &gpio_hndl1000x);
device_init_fail:
	smcbus_channels_unregister(rmc_hndl1000x);
	HNOS_DEBUG_INFO("rmc_hndl1000x registered fail \n");
	return ret;

}
module_init(rmc_hndl1000x_init);
module_exit(rmc_hndl1000x_remove);
MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
