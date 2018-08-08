/*
 *  drivers/char/hndl_char_devices/hnos_gpio.c
 *
 *         zrm,eter_zrm@163.com
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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>

#include <linux/proc_fs.h>

#include <linux/moduleparam.h>
#include <linux/ioport.h>

#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <linux/ctype.h>

#include <asm/uaccess.h>
#include <mach/adc.h>

#include <mach/board.h>

#include <linux/hnos_product.h>
#include "hnos_busmem.h"

#if defined (CONFIG_HNDL_PRODUCT_HNDL1000X)
#include "hnos_hndl1000x.h"
//unsigned int phybase[BUSMEM_NUM] ={ID_CS1,LCD_CS,KEY_CS,PULSE_CS2,RELAY_CS1,RELAY_CS2,GPRS_LCD_BEEP_CS,PULSE_CS1};
struct busmem_object busmem_pr[BUSMEM_NUM]={
        {
         .phy_base = ID_CS1,
         .iomem_base = NULL,
         .data = 0,
         .name = "ID_CS1",
        },
        {
         .phy_base = LCD_CS,
         .iomem_base = NULL,
         .data = 0,
         .name = "LCD_CS",
        },
        {
         .phy_base = KEY_CS,
         .iomem_base = NULL,
         .data = 0,
         .name = "KEY_CS",
        },
        {
         .phy_base = PULSE_CS2,
         .iomem_base = NULL,
         .data = 0,
         .name = "PULSE_CS2",
        }, 
        {
         .phy_base = RELAY_CS1,
         .iomem_base = NULL,
         .data = 0,
         .name = "RELAY_CS1",
        },
        {
         .phy_base = RELAY_CS2,
         .iomem_base = NULL,
         .data = 0,
         .name = "RELAY_CS2",
        }, 
         {
         .phy_base = GPRS_LCD_BEEP_CS,
         .iomem_base = NULL,
         .data = 0,
         .name = "GPRS_LCD_BEEP_CS",
        },
         {
         .phy_base = PULSE_CS1,
         .iomem_base = NULL,
         .data = 0,
         .name = "PULSE_CS1",
        },        
};
#endif

int hnos_busmem_opt(int cmd, int cs_index,int data,u8 bit)
{       
        int ret = 0, value = 0;
        u8 opt_bit = bit;
        void __iomem *base;
        unsigned long flags;
        
        if (cs_index > BUSMEM_NUM ){
                return -1;
        }
        spin_lock_irqsave(&busmem_pr[1].lock,flags);
        
        if(busmem_pr[cs_index].iomem_base == NULL){
                spin_unlock_irqrestore(&busmem_pr[1].lock,flags);
                return( -ENOMEM);
        }
        
        base = busmem_pr[cs_index].iomem_base;
        
	switch(cmd) {
/******output bus********/	
		case BUSMEM_OUTPUT_GET:
                        ret = busmem_pr[cs_index].data;
			break;

		case BUSMEM_OUTPUT_SET:
                        ret = writeb(data,base);
                        busmem_pr[cs_index].data = data & 0xff;
			break;
			
                case BUSMEM_OUTPUT_SET_BIT:
                        opt_bit = bit;
                        value = busmem_pr[cs_index].data;                        
                        if (data == SIG_LOW){
                                opt_bit = ~opt_bit;
                                value =value & opt_bit;
                        }
                        else if (data == SIG_HIGH){
                                value |= opt_bit;
                        }
                        busmem_pr[cs_index].data = value;
                        ret = writeb(value,base);
                        break;
                        
               case BUSMEM_OUTPUT_GET_BIT:
                       opt_bit = bit;
                       ret = busmem_pr[cs_index].data & opt_bit;
                       if(ret)
                               ret = 1;                       

                        break;
                        
/********input bus*********/                        
		case BUSMEM_INPUT_GET:
		        ret = readb(base) & 0xff;
                        busmem_pr[cs_index].data =  ret & 0xff;
			break;

		case BUSMEM_INPUT_SET:
                        busmem_pr[cs_index].data = data & 0xff;
			break;
			
                case BUSMEM_INPUT_SET_BIT: 
                        opt_bit =  bit;
                        ret = busmem_pr[cs_index].data;
                        if (data == SIG_LOW){
                                opt_bit = ~opt_bit;
                                ret =ret & opt_bit;
                        }
                        else if (data == SIG_HIGH){
                                ret |= opt_bit;
                        }
                        busmem_pr[cs_index].data = ret;
                                                
                        break;
                        
               case BUSMEM_INPUT_GET_BIT:
                        opt_bit =  bit;
                        ret = writeb(data,base) & opt_bit;
                        busmem_pr[cs_index].data |= ret;
                        if(ret)
                                ret = 1;                       
                        break;
                       
              default:  
                        ret = -ENOTTY;
                        break;
		}
        spin_unlock_irqrestore(&busmem_pr[1].lock,flags);
        
	return ret;
	
}
EXPORT_SYMBOL(hnos_busmem_opt);

static void  busmem_exit(void)
{       
        int i;
        unsigned long flags;

        for(i = 0; i < BUSMEM_NUM; i++){
               if (!busmem_pr[1].iomem_base){
                        spin_lock_irqsave(&busmem_pr[1].lock,flags);
                        iounmap(busmem_pr[i].iomem_base);
//                        release_mem_region(busmem_pr[i].phy_base, 1);
                        spin_unlock_irqrestore(&busmem_pr[1].lock,flags);
                }
        }
}

static int __init busmem_init(void)
{       
        int ret = 0;
        int i;
        unsigned long flags;
        
        for(i = 0; i < BUSMEM_NUM; i++){
               spin_lock_init(&busmem_pr[i].lock);
               spin_lock_irqsave(&busmem_pr[i].lock,flags);
#if 0              
               if (!request_mem_region(busmem_pr[i].phy_base, 1, busmem_pr[i].name)) {
                       printk("%s: request mem region error.\n", __FUNCTION__);
                       ret = -1;
                       spin_lock_irqsave(&busmem_pr[i].lock,flags);
                       goto request_failed;
               }
#endif               
               busmem_pr[i].iomem_base = ioremap(busmem_pr[i].phy_base, 1);
               if (!busmem_pr[i].iomem_base) {
                        spin_unlock_irqrestore(&busmem_pr[i].lock,flags);
                        printk(KERN_ERR "busmem_init:Can NOT remap address 0x%08x\n", (unsigned int)busmem_pr[i].phy_base);
                        ret = -1;
                        goto map_failed;
                   }
               spin_unlock_irqrestore(&busmem_pr[i].lock,flags);
        }
        
        return 0;
        
map_failed:

//        release_mem_region(busmem_pr[i].phy_base, 1);
        
request_failed:        
        busmem_exit();
        
        return ret;
        
}



module_init(busmem_init);
module_exit(busmem_exit);
MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

