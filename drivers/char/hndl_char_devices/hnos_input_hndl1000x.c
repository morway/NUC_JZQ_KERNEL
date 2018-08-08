/*
 *  drivers/char/hndl_char_devices/hnos_rmi_hndl1000x.c
 *
 *  For HNDL1000X VF4.0.
 *
 *  Author ZhangRM <peter_zrm@163.com> 
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

#define  INPUT_SMCBUS_OFFSET	8              /* (总线扩展)遥控输出从第16路开始 */
#define  INPUT_SMCBUS_SIZE	24              /* (总线扩展)遥信共计16路 */
#define  NCHANNEL_PER_SMCBUS	8
#define  NR_SMCBUS		3
#define  CTRL_EXISTED           STATE5  ////摇控摇信模块是否存在位
#define  GPIO_POWEROFF		AT91_PIN_PA3
#define  AT24C02_WR_IO          AT91_PIN_PA6
#define  MAX_DETECT_CNT         20

static struct iomem_object *bases[NR_SMCBUS] = {NULL,NULL,NULL};
static struct hndl_rmi_device *rmi_hndl1000x;
struct smcbus_rmi_data rmi_smcbus_hndl1000x;
static int before_poweroff_sig = DETECT_POWER_ON;
static int now_poweroff_sig = DETECT_POWER_ON; 
static int poweroff_cnt = 0;
static int cf_in_state = 0 ;    //有功输入状态
static int varcf_in_state = 0;  //无功输入状态
static  u8  ctr_existed_state = DETECT_MODULE_ON;
static  u8  scan_at24c02_write_1fr = 1;//检测24c02写保护否开启
static  u8  scan_at24c02_write_2sd = 1;//检测24c02写保护否开启
static  int measure_device = MEASURE_DEVICE_NO;

int hnos_IsPowerOffNow(void)
 {
        int  power_new = (at91_get_gpio_value(GPIO_POWEROFF) | vcc5v_poweroff());
        
        if(before_poweroff_sig == power_new){
                poweroff_cnt++;
                if(poweroff_cnt == MAX_DETECT_CNT){
                        poweroff_cnt =0;
                        now_poweroff_sig = (before_poweroff_sig)?DETECT_POWER_ON:DETECT_POWER_OFF;
                }        
         }
        else 
                before_poweroff_sig = power_new;
        return now_poweroff_sig;
 }
 EXPORT_SYMBOL(hnos_IsPowerOffNow);

//摇控摇信模块插入检测
 int hnos_module_state(void)
 {
       return ctr_existed_state;
 }
 EXPORT_SYMBOL(hnos_module_state);

 //检测24c02是否可写
  int hnos_at24c02_wr_state(void)
  {
        if((scan_at24c02_write_1fr == scan_at24c02_write_2sd)&&(scan_at24c02_write_1fr == 0))
                return 1;//可写
        else
                return 0;
  }
  EXPORT_SYMBOL(hnos_at24c02_wr_state);


 static int poweroff_proc_gpio_get(struct proc_item *item, char *page)
 {
         int len =0;
 
         if(hnos_IsPowerOffNow())
                 len = sprintf(page,"1\n");
         else
                 len = sprintf(page,"0\n");
                 
         return len;
                 
 }

 
 static struct proc_item items_hndl1000x[] = 
{
	{
	.name = "poweroff_state", //掉电检测
	.pin = GPIO_POWEROFF,
	//.settings = GPIO_PULLUP , /* input,  */
	.read_func = poweroff_proc_gpio_get,
	},
	{        
       .name = "SPI_SIG",         
       .pin = AT91_PIN_PA10,        
       .settings = GPIO_PULLUP , /* input,  */        
       .read_func = hnos_proc_gpio_get,        
       },
        {        
       .name = "writeeprom_state",         
       .pin = AT24C02_WR_IO,        
       .settings = GPIO_PULLUP , /* input,  */        
       .read_func = hnos_proc_gpio_get_reverse,        
       },
       {       
       .name = "P_IN", 
       .pin = AT91_PIN_PA7,
       .settings = 0, /* input, pullup */
       .read_func = hnos_proc_gpio_get,
       }, 
       {       
       .name = "Q_IN", 
       .pin = AT91_PIN_PA8,
       .settings = 0, 
       .read_func = hnos_proc_gpio_get,
       },       
};

void rmi_detect_opt(u8 scan_turn)
{
        int ret = 0;
	unsigned char ctr_existed1,ctr_existed2;
        
	if(scan_turn == SCAN_FIRST){      
                cf_in_state = at91_get_gpio_value(CF_IN_PIO);
                varcf_in_state = at91_get_gpio_value(VARCF_IN_PIO);
                scan_at24c02_write_1fr = at91_get_gpio_value(AT24C02_WR_IO);
                return;
        }

      
        //detect commu state bit

        if(bases[0]){
                ret = bases[0]->read_byte(bases[0], &ctr_existed1, IO_RDONLY);
                ctr_existed1 &= CTRL_EXISTED;
        }
        
        //有功无功输入输出

        if(hnos_IsPowerOffNow() || (measure_device == MEASURE_DEVICE_NO)){
                at91_set_gpio_value(CF_OUT_PIO, LED_OFF);
                at91_set_gpio_value(VARCF_OUT_PIO, LED_OFF);
        }
        else{
                ret = at91_get_gpio_value(CF_IN_PIO);
                if(ret == cf_in_state){
	                if(cf_in_state){
	                        if(measure_device == MEASURE_DEVICE_6513)
                                        at91_set_gpio_value(CF_OUT_PIO, LED_OFF);
                                else
                                        at91_set_gpio_value(CF_OUT_PIO, LED_ON);
                                //dprintk("CF_OUT_PIO:high\n");
                        }
                        else{
	                        if(measure_device == MEASURE_DEVICE_6513)
                                        at91_set_gpio_value(CF_OUT_PIO, LED_ON);
                                else
                                        at91_set_gpio_value(CF_OUT_PIO, LED_OFF);
                                
                        }        
                        cf_in_state  = ret;
                }

                ret = at91_get_gpio_value(VARCF_IN_PIO);
                if(ret == varcf_in_state){
	                if(varcf_in_state){
	                        if(measure_device == MEASURE_DEVICE_6513)
                                        at91_set_gpio_value(VARCF_OUT_PIO, LED_OFF);
                                else
                                        at91_set_gpio_value(VARCF_OUT_PIO, LED_ON);
                                //dprintk("VARCF_OUT_PIO:high\n");
                        }
                        else{
	                        if(measure_device == MEASURE_DEVICE_6513)
                                        at91_set_gpio_value(VARCF_OUT_PIO, LED_ON);
                                else
                                        at91_set_gpio_value(VARCF_OUT_PIO, LED_OFF);
                        }        
                        varcf_in_state  = ret;
                }        

        }

        //detect commu state bit
        if(bases[0]){
                ret = bases[0]->read_byte(bases[0], &ctr_existed2, IO_RDONLY);
                ctr_existed2 &= CTRL_EXISTED;
                if(ctr_existed2 == ctr_existed1)
                        ctr_existed_state = (ctr_existed2 > 0)?DETECT_MODULE_OFF:DETECT_MODULE_ON;
       }                
  
        scan_at24c02_write_2sd = at91_get_gpio_value(AT24C02_WR_IO);
	return;
}
EXPORT_SYMBOL(rmi_detect_opt);



static struct gpio_rmi_data gpio_hntt1000x =
{
	.items = items_hndl1000x,
	.size = ARRAY_SIZE(items_hndl1000x),
};


static int __devinit gpio_channels_init(struct hndl_rmi_device *input)
{
	u8 offset = 0;
	u8 size = ARRAY_SIZE(items_hndl1000x);
	int ret = 0;
	
	ret = rmi_gpio_register(input, &gpio_hntt1000x, offset, size);
        return ret;

        
}

static int smcbus_read_all(struct smcbus_rmi_data *bus, u32 *reslt)
{
        int ret =0;
        u8 ch0 = 0, ch1 = 0, ch2 = 0;

        ret = bases[0]->read_byte(bases[0], &ch0, IO_RDONLY);
        ret = bases[2]->read_byte(bases[2], &ch2, IO_RDONLY);
        
        if(ctr_existed_state == DETECT_MODULE_OFF){//摇信摇控模块没有插入
                ch1 = 0xff;
        }
        else{
                ret = bases[1]->read_byte(bases[1], &ch1, IO_RDONLY);
                //ch1 |= 0x0f;//摇信摇控低4路没有使用 
        }
        *reslt = ( ( ch2 << 16 ) | (ch1 << 8) | ch0);
        return 0;
}

static int smcbus_read_channel(struct smcbus_rmi_data *bus, u8 ch, u32 *reslt)
{
	u8 shift = 0;
	u8 index = 0;
	u8 mask = (1 << 0);
	u8 data = 0;
	u8 smcbus_ch = 0;
	u8 smcbus_off = rmi_get_smcbus_offset(rmi_hndl1000x);

	if ((ch >= (smcbus_off + NR_SMCBUS * NCHANNEL_PER_SMCBUS))) {
		printk("%s: Invalid channel %d.\n", __FUNCTION__, ch);
		return -EFAULT;
	}

	smcbus_ch = ch - smcbus_off;
	index = smcbus_ch / NCHANNEL_PER_SMCBUS;
	shift = smcbus_ch % NCHANNEL_PER_SMCBUS;
        if((ctr_existed_state == DETECT_MODULE_OFF)&&(index > 0)){//摇信摇控模块没有插入
                data = 0xff;
        }
        else{
                //if((index ==1) && (shift < 4))//摇信摇控低4路没有使用 
                //        data = 0xff;
                //else        
	                bases[index]->read_byte(bases[index], &data, IO_RDONLY);
	}
	
	mask = (1 << shift);
	data = (data & mask ) >> shift ;
	*reslt = data;
	return 0;
}



/* SMCBUS read not supported by the hardware. */
static int smcbus_read(struct smcbus_rmi_data *bus, u8 ch, u32 *reslt)
{
	int ret = 0;
        
	if (SMCBUS_CHAN_ALL == ch) {
		ret = smcbus_read_all(bus, reslt);
	} else {
		ret = smcbus_read_channel(bus, ch, reslt);
	}
	
	//dprintk("%s: reslt %x.\n", __FUNCTION__, *reslt);
	return ret;



}

static int smcbus_proc_read(struct smcbus_rmi_data *bus, char *buf)
{	
        int len = 0,ret = 0,rmi_value = 0;	
        u8  ch = 0,k,i;
        char *InputName[NR_SMCBUS]={"IDCS1","PulseCS1","PulseCS2"};
        
        for(k=0;k<3;k++){
                if(k == 1)//PULSE_CS1
                {
                 if(ctr_existed_state == DETECT_MODULE_OFF) {//摇信摇控模块没有插入
                        ch = 0xff;
                  }
                  else{
                        ret = bases[k]->read_byte(bases[k], &ch, IO_RDONLY);
                        //ch |= 0x0f;//摇信摇控低4路没有使用
                  }
                }
                else{
                        ret = bases[k]->read_byte(bases[k], &ch, IO_RDONLY);
                }
                    
                rmi_value |= ch << (k*8);
                for(i=0;i<8;i++){
                    len += sprintf(buf + len, "%d:%d,\t", k*8+i,(ch & (1 << i))?1:0);       
                }
                len += sprintf(buf + len, "\n"); 
                len += sprintf(buf + len, "%s = 0x%02x\n",InputName[k],ch);       
        }
        len += sprintf(buf + len, "\ninput0 = 0x%02x\n",rmi_value);       
        return len;
}

static void  smcbus_channels_unregister(struct hndl_rmi_device *input)
{
	rmi_smcbus_unregister(input, &rmi_smcbus_hndl1000x);
	iomem_object_put(bases[0]);
	iomem_object_put(bases[1]);
	iomem_object_put(bases[2]);
	return;
}

static int __devinit smcbus_channels_init(struct hndl_rmi_device *input)
{
	int reslt = 0;

	bases[0] = iomem_object_get(ID_CS1, 0);
	if (!bases[0]) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) ID_CS1);
		return -1;
	}
	bases[1] = iomem_object_get(PULSE_CS1, 0);
	if (!bases[1]) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) PULSE_CS1);
		reslt = -1;
		goto base0_request_failed;
	}
	bases[2] = iomem_object_get(PULSE_CS2, 0);
	if (!bases[2]) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) PULSE_CS2);
		reslt = -1;
		goto base1_request_failed;
	}	
	rmi_smcbus_hndl1000x.read = smcbus_read;
	rmi_smcbus_hndl1000x.proc_read = smcbus_proc_read;
	reslt = rmi_smcbus_register(input, &rmi_smcbus_hndl1000x, 
				INPUT_SMCBUS_OFFSET, INPUT_SMCBUS_SIZE);
        if (reslt < 0) {
		goto base2_request_failed;
	}
        return reslt;

base2_request_failed:
        iomem_object_put(bases[2]);
base1_request_failed:
	iomem_object_put(bases[1]);
base0_request_failed:
	iomem_object_put(bases[0]);

        return reslt;
}


static void  rmi_hndl1000x_remove(void)
{
	rmi_gpio_unregister(rmi_hndl1000x, &gpio_hntt1000x);
	smcbus_channels_unregister(rmi_hndl1000x);
	rmi_device_unregister(rmi_hndl1000x);
	rmi_device_free(rmi_hndl1000x);
	return;
}

static int __devinit rmi_hndl1000x_init(void)
{       int ret =0;

        ret = hndl1000x_id_mask_get();
        if(PRODUCT_HNDL1000X_CQ == ret){
                measure_device = MEASURE_DEVICE_7022;
        }        
        else if(PRODUCT_HNDL1000X_LN == ret){
                measure_device = MEASURE_DEVICE_6513;
        }        
                
	rmi_hndl1000x = rmi_device_alloc();
	if (!rmi_hndl1000x) {
		return -1;
	}
	gpio_channels_init(rmi_hndl1000x);
	smcbus_channels_init(rmi_hndl1000x);
	ret = rmi_device_register(rmi_hndl1000x);
	
        HNOS_DEBUG_INFO("rmi_hndl1000x registered.\n");
        return ret;
}
module_init(rmi_hndl1000x_init);
module_exit(rmi_hndl1000x_remove);
MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");
