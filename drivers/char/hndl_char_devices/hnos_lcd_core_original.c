/*
 * drivers/char/hndl_char_devices/hnos_lcd_core.c
 *
 * Based on uc1698fb.c from Linux 2.4 kernel, writen by licikui.
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */

#include "hnos_generic.h"
#include "hnos_lcd_cog.h"
#include "hnos_proc.h"
#include "hnos_kbd.h"
#include "hnos_debug.h"
#include <mach/map.h>
#include <mach/regs-gpio.h>


#define DEVICE_NAME	"fb0"

int nuc970_gpio_core_direction_out1(unsigned gpio_num, int val);
int nuc970_gpio_core_set1(unsigned gpio_num,int val);
int nuc970_gpio_core_to_request1(unsigned offset);



struct at91_lcd_cdev *lcd_COG_device;
static struct proc_dir_entry *hndl_proc_dir = NULL;

static unsigned char uc1698fb_data[UC1698FB_ROW_SIZE*UC1698FB_COLUMN_SIZE/8] = {0};

static unsigned int uc1698fb_temp_comp = 0x2;  /* Temp Compensation, range from 0x0 to 0x3.*/
module_param(uc1698fb_temp_comp, int, S_IRUGO);

static unsigned int uc1698fb_bias_ratio = 0x2; /* LCD bias ratio, range from 0x0 to 0x3.*/
module_param(uc1698fb_bias_ratio, int, S_IRUGO);

static unsigned int uc1698fb_bias_data = 0xa1; /* Vbias Potentiometer, range from 0x0 to 0xff.*/ 
module_param(uc1698fb_bias_data, int, S_IRUGO);

static int lcd_major =   0;
static int lcd_minor =   0;
module_param(lcd_major, int, S_IRUGO);
module_param(lcd_minor, int, S_IRUGO);

struct uc1698fb_callbacks uc1698fb_cbs;

//---------------增加用于串行液晶，uc1698u-------------------------------------
//核心板增加两管脚（97、98脚），做液晶I2C总线，97脚LCD
//SDA'接核心板中单片机PC19(J1)
//98脚LCD
//SCK'接核心板中单片机的PC18（H1）

//#define RS_HIGH   writeb(0, UC1698FB_CMD_ADDR+1)  //写数据
//#define RS_LOW    writeb(0, UC1698FB_CMD_ADDR)    //写指令

//#define RS_HIGH		nuc970_gpio_core_set(NUC970_PB5, 1)
//#define RS_LOW		nuc970_gpio_core_set(NUC970_PB5, 0)
#define RS_HIGH {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t|0x8000);} //pb15
#define RS_LOW {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t&~(0x8000));} //pb15


//#define nCS_HIGH	nuc970_gpio_core_set(NUC970_PB2, 1)
//#define nCS_LOW		nuc970_gpio_core_set(NUC970_PB2, 0)
#define nCS_HIGH {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t|0x1000);} //pb12
#define nCS_LOW {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t&~(0x1000));} //pb12


//#define SDA_HIGH	nuc970_gpio_core_set(NUC970_PB1, 1)
//#define SDA_LOW		nuc970_gpio_core_set(NUC970_PB1, 0) 
#define SDA_HIGH {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t|0x4000);} //pb14
#define SDA_LOW {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t&~(0x4000));} //pb14


//#define SCLK_HIGH	nuc970_gpio_core_set(NUC970_PB3, 1) 
//#define SCLK_LOW	nuc970_gpio_core_set(NUC970_PB3, 0)
#define SCLK_HIGH {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t|0x2000);} //pb13
#define SCLK_LOW {register unsigned int t= (*(volatile unsigned int *)REG_GPIOB_DATAOUT);((*(volatile u32 *) (REG_GPIOB_DATAOUT))=t&~(0x2000));} //pb13


#define B_HIGH {register unsigned int t= (*(volatile unsigned int *)REG_GPIOG_DATAOUT);((*(volatile u32 *) (REG_GPIOG_DATAOUT))=t|0x20);} //pg5
#define B_LOW {register unsigned int t= (*(volatile unsigned int *)REG_GPIOG_DATAOUT);((*(volatile u32 *) (REG_GPIOG_DATAOUT))=t&~(0x20));} //pg5


unsigned int my_ioaddr_cmd;
void write_cmd(unsigned char  cmd)
 {
  unsigned char   Scnt;
  RS_LOW;
 // udelay(1);
  nCS_LOW;
  Scnt=8;
  while(Scnt--)
  {
	  SCLK_LOW;
	  //udelay(1);
	  SDA_LOW;
	  //udelay(1);
	  if (cmd&0x80)
	  {
	    SDA_HIGH;
	   // udelay(1);
	  }
	  SCLK_HIGH;
	  //udelay(1);
	  cmd<<=1;
  }
  nCS_HIGH;
 // udelay(1);
 }
 
 
 void write_data(unsigned char   dat)
 {
 
  unsigned char   Scnt;
  RS_HIGH;
 // udelay(1);
  nCS_LOW;
  Scnt=8;
  while(Scnt--)
  {
	  SCLK_LOW;
	 // udelay(1);
	  SDA_LOW;
	 // udelay(1);
	  if (dat&0x80)
	  {
	    SDA_HIGH;
	 //   udelay(1);
	  }
	  SCLK_HIGH;
	 // udelay(1);
	  dat<<=1;
  }
  nCS_HIGH;
  //udelay(1);
 }
 
int  my_writeb(unsigned char   dat,unsigned int flag)
 {
 #if 1
	 	if (flag==my_ioaddr_cmd+1)//data
	 	{
	 	
	 		write_data(dat);
	 		
	 	}else//cmd
	 	{
	 	
	 		write_cmd(dat);
	 	}
#else
    writeb(dat,flag);
#endif
	
 	return 1;
 }



int uc1698u_pin_init(void)
{
	//RESET
	nuc970_gpio_core_to_request1(196);
	nuc970_gpio_core_direction_out1(196,0);
	
	//SDA
	nuc970_gpio_core_to_request1(46);
	nuc970_gpio_core_direction_out1(46,0);

	//CS	
	nuc970_gpio_core_to_request1(44);
	nuc970_gpio_core_direction_out1(44,0);
	
	//CLK
	nuc970_gpio_core_to_request1(45);
	nuc970_gpio_core_direction_out1(45,0);
	
	//RS
	nuc970_gpio_core_to_request1(47);
	nuc970_gpio_core_direction_out1(47,0);
	
    //BLACK LIGHT
    nuc970_gpio_core_to_request1(197);
	nuc970_gpio_core_direction_out1(197,0);

	
//	B_LOW;
//	mdelay(1000);
	//B_HIGH;
	
	//mdelay(1000);
	//B_LOW;
	//mdelay(1000);
	//B_HIGH;


}





//----------uc1698c end -------

//------------------------------------------------------

int uc1698fb_callbacks_register(struct uc1698fb_callbacks *cbs)
{
    if (unlikely(!cbs)) {
        printk(KERN_ERR "%s: invalid param.\n", __FUNCTION__);
        return -EINVAL;
    }

    if (cbs->init) {
        uc1698fb_cbs.init = cbs->init;
        uc1698fb_cbs.init();
    }

    if (cbs->reset) {
        uc1698fb_cbs.reset = cbs->reset;
        cbs->reset();
    }

    if (cbs->backlight) {
        uc1698fb_cbs.backlight = cbs->backlight;
    }

    return 0;
}

int uc1698fb_callbacks_unregister(struct uc1698fb_callbacks *cbs)
{
    if (unlikely(!cbs)) {
        printk(KERN_ERR "%s: invalid param.\n", __FUNCTION__);
        return -EINVAL;
    }

    if (cbs->init) {
        uc1698fb_cbs.init = NULL;
    }

    if (cbs->reset) {
        uc1698fb_cbs.reset = NULL;
    }

    if (cbs->backlight) {
        uc1698fb_cbs.backlight = NULL;
    }

    return 0;
}

void uc1698fb_bias_ratio_set(unsigned char data, struct at91_lcd_cdev *dev) 
{
    unsigned int ioaddr_cmd = dev->ioaddr_cmd;
    uc1698fb_bias_ratio = data & 0x3;

    UC1698FB_SET_BIAS_RATIO(ioaddr_cmd, uc1698fb_bias_ratio);
    return;
}

unsigned char uc1698fb_bias_ratio_get(void)
{
    return uc1698fb_bias_ratio;
}

void uc1698fb_bias_data_set(unsigned char data)
{
     unsigned int ioaddr_cmd = lcd_COG_device->ioaddr_cmd;
    uc1698fb_bias_data = data;
    UC1698FB_SET_VBIAS_PM(ioaddr_cmd, uc1698fb_bias_data);
    return;
}

unsigned char uc1698fb_bias_data_get(void)
{
    return uc1698fb_bias_data;
}

void uc1698fb_temp_comp_set(unsigned char data)
{
     unsigned int ioaddr_cmd = lcd_COG_device->ioaddr_cmd;
    uc1698fb_temp_comp = data & 0x3;
    UC1698FB_SET_TEMP_COMP(ioaddr_cmd, uc1698fb_temp_comp);
    return;
}

unsigned char uc1698fb_temp_comp_get(void)
{
    return uc1698fb_temp_comp;
}

void  uc1698fb_reset(void)
{
     unsigned int ioaddr_cmd = lcd_COG_device->ioaddr_cmd;
 
 #if 0
	nuc970_gpio_core_set1(196, 1); 	
	mdelay(20);	
	nuc970_gpio_core_set1(196, 0); 	
	mdelay(20);	
	nuc970_gpio_core_set1(196, 1); 	
#endif


    if (uc1698fb_cbs.reset) {
        uc1698fb_cbs.reset();
    }

    UC1698FB_SYSTEM_RESET(ioaddr_cmd);
    return;
}

void uc1698fb_blt_generic(unsigned char action)
{
    if (UC1698FB_LCD_BACKLIGHT_ON == action) {
		//nuc970_gpio_core_direction_out1(197, 1);
		B_HIGH;
        //at91_set_gpio_output(UC1698FB_LCD_BACKLIGHT_PIN, 1);
    } else if (UC1698FB_LCD_BACKLIGHT_OFF == action) {
       // at91_set_gpio_output(UC1698FB_LCD_BACKLIGHT_PIN, 0);
       B_LOW;
		//nuc970_gpio_core_direction_out(UC1698FB_LCD_BACKLIGHT_PIN, 0);
    }

    return;
}

int uc1698fb_back_light(unsigned int action)
{
    if (uc1698fb_cbs.backlight) {
        uc1698fb_cbs.backlight(action);
    } else {
        uc1698fb_blt_generic(action);
    }

    return 0;
}

static int uc1698fb_chip_probe(void)
{

#if 0
    void __iomem *ioaddr_cmd = lcd_COG_device->ioaddr_cmd;
    unsigned char status = 0;
    unsigned char product_code = 0;

    /* 
     * FIXME:
     * The LCD had been initialized in U-Boot, so we may not nead this.
     * */

    uc1698fb_reset();
    mdelay(5);

    /* Get chip status and product ID. */
    status = readb(ioaddr_cmd);
    product_code = readb(ioaddr_cmd);
    product_code = readb(ioaddr_cmd);
    product_code &= 0xf0;

    dprintk("%s: status=0x%2x, product_code=0x%2x\n",__FUNCTION__, status, product_code);
    if (UC1698FB_CHIP_STATUS == status 
            && ((UC1698FB_PRODUCT_CODE == product_code )||(0x90 == product_code))) {
        HNOS_DEBUG_INFO("Chip UC1698U found, status %2x, product_code %2x\n", status, product_code);
        return 0;
    }
#endif
	
	uc1698fb_reset();

    HNOS_DEBUG_INFO("No UC1698U chip found,status 0, product_code 0\n"); 
    return 0; 
}

static void uc1698fb_chip_init(struct at91_lcd_cdev *dev)
{	

    unsigned int ioaddr_cmd = dev->ioaddr_cmd;

    UC1698FB_SET_SCROLL_LINE(ioaddr_cmd, 0);	/* no scroll */
    UC1698FB_SET_PARTIAL_CNTRL(ioaddr_cmd, 0);	/* partial display disabled */
    UC1698FB_SET_FIXED_LINE(ioaddr_cmd, 0);         /* fixed line 0*/

    UC1698FB_SET_PIXEL_ON(ioaddr_cmd, 0);
    UC1698FB_SET_INVERSE_DISP(ioaddr_cmd, 0);

    UC1698FB_SET_ADV_POWER(ioaddr_cmd, 0x08);
    UC1698FB_SET_POWER_CNTRL(ioaddr_cmd, 0x3);

    UC1698FB_SET_TEMP_COMP(ioaddr_cmd, uc1698fb_temp_comp);

    UC1698FB_SET_LINE_RATE(ioaddr_cmd, 0);
    UC1698FB_SET_BIAS_RATIO(ioaddr_cmd, uc1698fb_bias_ratio);

    UC1698FB_SET_COM_END(ioaddr_cmd, UC1698FB_ROW_SIZE - 1);

    UC1698FB_SET_VBIAS_PM(ioaddr_cmd, uc1698fb_bias_data);

    UC1698FB_SET_MAPPING_CNTRL(ioaddr_cmd, 4); 
    UC1698FB_SET_RAM_ADDR_CNTRL(ioaddr_cmd, 1);  /* no automatic wrap, CA auto-increment first, +1 */

    UC1698FB_SET_COLOR_PATTERN(ioaddr_cmd, 1);  /* RGB */
    UC1698FB_SET_COLOR_MODE(ioaddr_cmd, 1);     /* 4K color mode */

    UC1698FB_SET_NLINE_INV(ioaddr_cmd, 0x18);
    UC1698FB_SET_DISPLAY_ENABLE(ioaddr_cmd, 0x5); /* display enabled, green enhancing mode disabled */

    /* Window program init */
    UC1698FB_SET_WINDOW_COLUMN_START(ioaddr_cmd, UC1698FB_COLUMN_START/3);	
    UC1698FB_SET_WINDOW_COLUMN_END(ioaddr_cmd, (UC1698FB_COLUMN_START+UC1698FB_COLUMN_SIZE)/3);

    UC1698FB_SET_WINDOW_ROW_START(ioaddr_cmd, 0);
    UC1698FB_SET_WINDOW_ROW_END(ioaddr_cmd, UC1698FB_ROW_SIZE-1);

    UC1698FB_SET_WINDOW_MODE(ioaddr_cmd, 0);  /* inside mode */

    /* SRAM Position init */
    UC1698FB_SET_ROW_ADDRS(ioaddr_cmd, 0);   /* SRAM row address start 0 */
    UC1698FB_SET_COLUMN_ADDR(ioaddr_cmd, UC1698FB_COLUMN_START/3);/* SRAM column address start 0 */

    return;
}

/* 
 * Turn 1 byte B/W data to 4k-color data (RRRR-GGGG-BBBB)
 * NOTE: out_picture array size should >= 4 
 * */
static int  uc1698fb_BW2color(unsigned char in_picture, unsigned char *out_picture)
{
    int ret = 0;  
    unsigned char temp, temp1, temp2, temp3, temp4, temp5, temp6, temp7, temp8; 
    unsigned char h11, h12, h13, h14, h15, h16, h17, h18; 

    if (!out_picture){
        return -1;
    }

    temp = in_picture;   
    temp1 = temp & 0x80; 
    temp2 = (temp & 0x40) >> 3; 
    temp3 = (temp & 0x20) << 2; 
    temp4 = (temp & 0x10) >> 1; 
    temp5 = (temp & 0x08) << 4; 
    temp6 = (temp & 0x04) << 1; 
    temp7 = (temp & 0x02) << 6; 
    temp8 = (temp & 0x01) << 3; 

    h11 = temp1 | temp1 >> 1 | temp1 >> 2 | temp1 >> 3; 
    h12 = temp2 | temp2 >> 1 | temp2 >> 2 | temp2 >> 3; 
    h13 = temp3 | temp3 >> 1 | temp3 >> 2 | temp3 >> 3; 
    h14 = temp4 | temp4 >> 1 | temp4 >> 2 | temp4 >> 3; 
    h15 = temp5 | temp5 >> 1 | temp5 >> 2 | temp5 >> 3; 
    h16 = temp6 | temp6 >> 1 | temp6 >> 2 | temp6 >> 3; 
    h17 = temp7 | temp7 >> 1 | temp7 >> 2 | temp7 >> 3; 
    h18 = temp8 | temp8 >> 1 | temp8 >> 2 | temp8 >> 3; 

    *out_picture = h11 | h12; 
    *(out_picture + 1) = h13 | h14; 
    *(out_picture + 2) = h15 | h16; 
    *(out_picture + 3) = h17 | h18; 

    return ret;
}


static inline void uc1698fb_write_data(unsigned char *buf, unsigned char size)
{
	int i = 0; 
	unsigned int ioaddr_data = lcd_COG_device->ioaddr_data;

	for (i=0; i<size; i++) {
		my_writeb(buf[i], ioaddr_data);
	}

	return;
}

static ssize_t uc1698fb_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{	
	struct at91_lcd_cdev *dev = filp->private_data; 
	int i = 0, j = 0;
	unsigned char color_data_buf[4] = {0};
	unsigned int nr_columns = UC1698FB_COLUMN_SIZE / 8;
	unsigned int nr_rows = UC1698FB_ROW_SIZE;

	if (!dev) {
		return -ENODEV;
	}

    if (count != UC1698FB_ROW_SIZE * UC1698FB_COLUMN_SIZE / 8) {
        printk("%s: count should be %d \n", __FUNCTION__, 
                UC1698FB_ROW_SIZE * UC1698FB_COLUMN_SIZE / 8);
        return -1;
    } 

    if (copy_from_user(uc1698fb_data, buf, count)) {
        printk("%s: error occured while copy data from user.\n", __FUNCTION__);
        return -1;
    }
#if 0
	else{
    
         printk("%s:count=%d\n", __FUNCTION__,count);

    int i;
		 for(i=0;i<20;i++)
		   printk("%02x ",buf[i]);

		 
		 printk("\n");
    	}
#endif
    /*
     * FIXME:
     * It seems that before you write data to uc1698, you should init it first.
     * */
    uc1698fb_chip_init(dev);

    for(i=0; i<nr_rows; i++) {
        for(j=0; j<nr_columns; j++){
            uc1698fb_BW2color(*(uc1698fb_data + i * nr_columns + j), color_data_buf);
            uc1698fb_write_data(color_data_buf, sizeof(color_data_buf));
        }   	

        my_writeb(0x00, dev->ioaddr_data); // ?
    }

    return count;
}

int uc1698fb_proc_read_CtlReg(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	int len = 0;
//    int k;
//	unsigned int ioaddr_cmd = lcd_COG_device->ioaddr_cmd;
//	unsigned char read_buff[3] = {0};
#if 0    
    for (k=0;k<3;k++)    
	    read_buff[k] = readb(ioaddr_cmd);
	
	len += sprintf(buf+len, "status_reg0  :\t0x%2x\n", read_buff[0]);
	len += sprintf(buf+len, "status_reg1  :\t0x%2x\n", read_buff[1]);
	len += sprintf(buf+len, "status_reg2  :\t0x%2x\n", read_buff[2]);

	*eof = 1;
#endif	
	return len;
}


static int uc1698fb_proc_read(char *buf, char **start, off_t offset,
        int count, int *eof, void *data)
{
    int len = 0;
    len += sprintf(buf+len, "lcd_bias   :\t0x%2x\n", uc1698fb_bias_data_get());
    len += sprintf(buf+len, "temp_comp  :\t0x%2x\n", uc1698fb_temp_comp_get());
    len += sprintf(buf+len, "bias_ratio :\t0x%2x\n", uc1698fb_bias_ratio_get());

    *eof = 1;
    return len;
}

static int uc1698fb_proc_write(struct file *file, const char __user * userbuf,
        unsigned long count, void *data)
{
    unsigned int value = 0;
    char val[14] = {0};

    if (count >= 14){
        return -EINVAL;
    }

    if (copy_from_user(val, userbuf, count)){
        return -EFAULT;
    }

    value = (unsigned int)simple_strtoull(val, NULL, 0);

    dprintk(KERN_INFO "\n%s:val=%s,after strtoull,value=0x%08x\n",
            __FUNCTION__, val, value);

    uc1698fb_bias_data_set(value&0xff);
    return count;
}

static int uc1698fb_open(struct inode *inode, struct file *filp)
{
    struct at91_lcd_cdev *dev; 

    dev = container_of(inode->i_cdev, struct at91_lcd_cdev, cdev);
    filp->private_data = dev; /* for other methods */

    if (test_and_set_bit(0, &dev->is_open) != 0) {
        return -EBUSY;       
    }

    return 0; 
}

static int uc1698fb_release(struct inode *inode, struct file *filp)
{
    struct at91_lcd_cdev *dev = filp->private_data; 

    if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
        return -EINVAL; /* already released: error */
    }

    return 0;
}




static ssize_t demo_read(struct file *fp, char __user *user_buf, size_t count, loff_t *ppos)
 {
     char kbuf[10];
     int ret, wrinten;

 //    printk(KERN_INFO "user_buf: %p, count: %d, ppos: %lld\n",
//         user_buf, count, *ppos);
 
   //  wrinten = snprintf(kbuf, 10, "%s", "Hello");
 
     ret = copy_to_user(user_buf, kbuf, wrinten+1);
     if (ret != 0) {
         printk(KERN_ERR "read error");
         return -EIO;
     }else{
        // printk( "%s:read success!\n",__FUNCTION__);
     	}
 
 
     *ppos += wrinten;
 
     return wrinten;
 }
 
 static ssize_t demo_write (struct file *fp, const char __user *user_buf, size_t count, loff_t *ppos)
 {
     char kbuf[10] = {0};
     int ret;
 
  //   printk(KERN_INFO "user_buf: %p, count: %d, ppos: %lld\n",
   //         user_buf, count, *ppos);
 
     ret = copy_from_user(kbuf, user_buf, count);
     if (ret) {
         pr_err("%s: write error\n", __FUNCTION__);
         return -EIO;
     }else{
       //  printk( "%s:write success!\n",__FUNCTION__);
     	}
 
     *ppos += count;
 
     return 0;
 }
 
 static struct file_operations file_ops = {
	//.read = demo_read,
	//.write = demo_write,
	.read = uc1698fb_proc_read,
	.write = uc1698fb_proc_write,

 };
#if 0
static ssize_t module_output(struct file *filp,
            char *buffer,
            size_t length,
            loff_t * offset)
{

    static int finished = 0;
    int i;
    char message[MESSAGE_LENGTH + 30];
    if(finished) {
        finished = 0;
        return 0;
    }
    sprintf(message,"Last input: %s",Message);
    for(i = 0;i < length && message[i];i++)
        put_user(message[i],buffer+i);

    finished = 1;
        return i;
}

static ssize_t module_input(struct file *filp,const char* buff,size_t len,loff_t *off)
{
//写数据到内核空间，通过get_user将字符一个一个存入到内核空间的Message的字符数组中
    int i;
    for(i = 0;i < MESSAGE_LENGTH-1 && i < len;i++)
        get_user(Message[i],buff + i);

    Message[i] = '\0';
    return i;
}

#endif
struct file_operations lcd_fops =
{
    .owner =    THIS_MODULE,
    .open =     uc1698fb_open,
    .release =  uc1698fb_release,
    .write = uc1698fb_write,
};

static int  uc1698fb_cdev_setup(struct at91_lcd_cdev *dev, dev_t devno)
{
    int err = 0;

    cdev_init(&dev->cdev, &lcd_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);

    if (err) { 
        printk(KERN_NOTICE "Error %d adding LCD(COG) device, major_%d", err, MAJOR(devno));
    }
    return err;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
void uc1698fb_module_cleanup(void)
{
#if 0

	cdev_del(&lcd_COG_device->cdev);
	kfree(lcd_COG_device);
	unregister_chrdev_region(MKDEV(lcd_major, lcd_minor), 1);


#else

    dev_t devno = MKDEV(lcd_major, lcd_minor);	
    struct class *myclass;

    if (lcd_COG_device){
        lcd_blt_unregister(uc1698fb_back_light);

        /* Get rid of our char dev entries */	
        cdev_del(&lcd_COG_device->cdev);	

        myclass = lcd_COG_device->myclass;
        if (myclass){
            device_destroy(myclass, devno);
            class_destroy(myclass);
        }

        if (lcd_COG_device->ioaddr_cmd) {
         //   iounmap(lcd_COG_device->ioaddr_cmd);
            lcd_COG_device->ioaddr_cmd = 0;
            lcd_COG_device->ioaddr_data = 0;
            release_mem_region(UC1698FB_CMD_ADDR, UC1698FB_IOMEM_SIZE);
        }

        kfree(lcd_COG_device);
        lcd_COG_device = NULL;
    }

    remove_proc_entry("lcd_status", hndl_proc_dir);
    remove_proc_entry("lcd_reg_status", hndl_proc_dir);
    hnos_proc_rmdir();

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, 1);

    HNOS_DEBUG_INFO("Cleanup device %s, major %d \n", DEVICE_NAME, lcd_major);
#endif
	return;

}

/*
 * Finally, the module stuff
 */
static int __init  uc1698fb_module_init(void)
{
    int result = 0;
    dev_t dev = 0;
    struct class *myclass = NULL;
    unsigned int ioaddr_cmd = 0;
    
	struct proc_dir_entry *proc = NULL;

#if 0  //serial液晶
	uc1698u_pin_init();
#endif

    /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */


    //lcd_major=252;
	//lcd_minor=0;
#if 1	
    if (lcd_major) {
        dev = MKDEV(lcd_major, lcd_minor);
        result = register_chrdev_region(dev, 1, DEVICE_NAME);
    } else {
        result = alloc_chrdev_region(&dev, lcd_minor, 1, DEVICE_NAME);
        lcd_major = MAJOR(dev);
    }
    if (result < 0) {
        printk(KERN_WARNING "hndl_lcd: can't get major %d\n", lcd_major);
        return result;
    }else{
    	printk(KERN_WARNING "hndl_lcd:  get major %d\n", lcd_major);
    }

    /* 
     * allocate the devices -- we do not have them static.
     */
    lcd_COG_device = kmalloc(sizeof(struct at91_lcd_cdev), GFP_KERNEL);
    if (!lcd_COG_device) {
        result = -ENOMEM;
        goto alloc_failed;  /* Make this more graceful */
    }
	else{
		printk("%s: alloc success.\n", __FUNCTION__);
	}
	
    memset(lcd_COG_device, 0, sizeof(struct at91_lcd_cdev));

    if (uc1698fb_cdev_setup(lcd_COG_device, dev) < 0) {
        result = -1;
        printk("%s: uc1698fb_cdev_setup fail.\n", __FUNCTION__);
        goto cdev_failed;
    }else{
    	printk("%s: uc1698fb_cdev_setup success.\n", __FUNCTION__);
    }

	
    spin_lock_init(&lcd_COG_device->lock);

   // if (!request_mem_region(UC1698FB_CMD_ADDR, UC1698FB_IOMEM_SIZE, "uc1698fb")) {
   //     printk("%s: request mem region error.\n", __FUNCTION__);
   //     result = -1;
   //     goto region_failed;
   // }
#else

	result = register_chrdev(lcd_major, DEVICE_NAME, &lcd_fops);				
	if (result)  
		goto out;	


#endif



 //   if (!ioaddr_cmd) {
 //       printk(KERN_ERR "Can NOT remap address 0x%08x\n", UC1698FB_CMD_ADDR);
 //       result = -1;
 //       goto remap_failed;
 //   }else{
  //      printk(KERN_ERR "address 0x%08x,remap 0x%08x \n", UC1698FB_CMD_ADDR,ioaddr_cmd);
 //   	}
 	ioaddr_cmd =UC1698FB_CMD_ADDR;
    my_ioaddr_cmd=ioaddr_cmd;
    lcd_COG_device->ioaddr_cmd = ioaddr_cmd;
    lcd_COG_device->ioaddr_data = ioaddr_cmd + 1;


    /* Register a class_device in the sysfs. */
    myclass = class_create(THIS_MODULE, DEVICE_NAME);
    if (myclass == NULL) {
        result = -ENOMEM;
        printk("%s: class_create fail.\n", __FUNCTION__);
        goto class_failed;
    }else{
    	printk("%s: class_create success.\n", __FUNCTION__);
    	}
    lcd_COG_device->myclass = myclass;

    device_create(myclass, NULL, dev, NULL, DEVICE_NAME);

    
    if (uc1698fb_chip_probe() < 0) {
        result = -1;
        printk("%s: uc1698fb_chip_probe fail.\n", __FUNCTION__);
        goto probe_failed;
    }else{
    	 printk("%s: uc1698fb_chip_probe success.\n", __FUNCTION__);
    	}
	
	
    uc1698fb_chip_init(lcd_COG_device);




#if 1

    if (!hndl_proc_dir) {
       hndl_proc_dir = hnos_proc_mkdir();
    }

    if (!hndl_proc_dir) {
        result = -ENODEV;
        printk("%s: creat proc dir fail.\n", __FUNCTION__);
        goto probe_failed;
        
    } else {
       // proc = create_proc_read_entry("lcd_status", S_IFREG | S_IRUGO | S_IWUSR, 
       //         hndl_proc_dir, uc1698fb_proc_read, NULL);
        proc = proc_create("lcd_status", S_IFREG | S_IRUGO | S_IWUSR,  hndl_proc_dir, &file_ops);
     	if (proc) {
        //    proc->write_proc = uc1698fb_proc_write;
			printk("%s: creat proc success.\n", __FUNCTION__);
        } else {
            result = -1;
            printk("%s: creat proc fail.\n", __FUNCTION__);
            goto proc_entry_failed;
        }

	//	proc = create_proc_read_entry("lcd_reg_status", S_IFREG | S_IRUGO | S_IWUSR, 
	//			hndl_proc_dir, uc1698fb_proc_read_CtlReg, NULL);
    //    proc = proc_create("lcd_status", S_IFREG | S_IRUGO | S_IWUSR, 
    //            hndl_proc_dir, uc1698fb_proc_read, NULL);		
	//	if (!proc) {
	//		result = -1;
	//		goto proc_entry_failed1;
	//	}
        
    }

    result = lcd_blt_register(uc1698fb_back_light);
    if (result) {
        goto lcd_blt_err;
    }

    HNOS_DEBUG_INFO("Initialized device %s, major %d \n", DEVICE_NAME, lcd_major);
    return 0;

lcd_blt_err:
    remove_proc_entry("lcd_reg_status", hndl_proc_dir);
    
proc_entry_failed1:
    remove_proc_entry("lcd_status", hndl_proc_dir);

proc_entry_failed:
    hnos_proc_rmdir();

#endif


probe_failed:
    cdev_del(&lcd_COG_device->cdev);	

cdev_failed:
    if (myclass){
        device_destroy(myclass, dev);
        class_destroy(myclass);
    }

class_failed:
//	iounmap(lcd_COG_device->ioaddr_cmd);
    lcd_COG_device->ioaddr_cmd = 0;
    lcd_COG_device->ioaddr_data = 0;




remap_failed:
	release_mem_region(UC1698FB_CMD_ADDR, UC1698FB_IOMEM_SIZE);

region_failed:
    kfree(lcd_COG_device);
    lcd_COG_device = NULL;

alloc_failed:
    unregister_chrdev_region(dev, 1);

out:  
	printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);  


    return result;
}

module_init(uc1698fb_module_init);
module_exit(uc1698fb_module_cleanup);

EXPORT_SYMBOL(uc1698fb_back_light);
EXPORT_SYMBOL(uc1698fb_callbacks_register);
EXPORT_SYMBOL(uc1698fb_callbacks_unregister);

MODULE_LICENSE("Dual BSD/GPL");

