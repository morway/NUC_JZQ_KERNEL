
/*
 * drivers/char/hndl_char_devices/hnos_lcd_ili9325.c
 *
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
#include "hnos_lcd_ili9325.h"
#include "hnos_proc.h"

struct ili9325_device 
{
	spinlock_t lock;
	unsigned long is_open;	  
	struct class *class;
	struct cdev cdev;	       
	void __iomem *membase;
	void __iomem *membase_index;
	u8 *fbdata;
};

struct ili9325_device *lcd_ili9325;
static  u16 ili9325_reg_cache[ILI9325_TIMING_CTRL_3 + 1];
static struct proc_dir_entry *hndl_proc_dir = NULL;

static int lcd_major =   0;
static int lcd_minor =   0;
module_param(lcd_major, int, S_IRUGO);
module_param(lcd_minor, int, S_IRUGO);

#undef  ILI9325_BPP
#define ILI9325_BPP  16


#define DEVICE_NAME		"lcd_ili9325"

static inline int ili9325_write_reg(struct ili9325_device *dev, u8 reg, u16 data)
{
	void __iomem *index = dev->membase_index;
	void __iomem *data_base = dev->membase;

	if (reg > ILI9325_TIMING_CTRL_3) {
		printk(KERN_ERR "%s: invalid register address %x.\n", __FUNCTION__, reg);
		return -EINVAL;
	}

	writew(reg, index);
	writew(data, data_base);
	return 0;
}

static inline int ili9325_read_reg(struct ili9325_device *dev, u8 reg)
{
	void __iomem *index = dev->membase_index;
	void __iomem *data_base = dev->membase;

	if (reg > ILI9325_DRV_CODE) {
		return ili9325_reg_cache[reg];
	} else {
		writew(reg, index);
		return readw(data_base);
	}
}


static void  ili9325_reset(void)
{
	at91_set_gpio_output(ILI9325_LCD_RST, 0);
	msleep(300);
	at91_set_gpio_output(ILI9325_LCD_RST, 1);
	msleep(100);

	return;
}

void ili9325_back_light(unsigned char cmd)
{
	int value = (LCD_BLT_ON == cmd) ? 1 : 0;
	at91_set_gpio_output(ILI9325_LCD_BLT, value);

	return;
}


static void ili9325_bus_init(void)
{
	at91_set_A_periph(AT91_PIN_PC11, 1); /* CS 2 */

	/* 
	 * WE SETUP: 6 MCK clocks. 
	 * CS SETUP: 4 MCK clocks.
	 * */
	at91_sys_write(AT91_SMC_SETUP(2), AT91_SMC_NWESETUP_(6) 
			| AT91_SMC_NCS_WRSETUP_(4)| AT91_SMC_NRDSETUP_(6) | AT91_SMC_NCS_RDSETUP_(4));

	/* 
	 * WE Pulse Width: 10 MCK clocks.
	 * CS Pulse Width: 20 MCK clocks.
	 * */
	at91_sys_write(AT91_SMC_PULSE(2), AT91_SMC_NWEPULSE_(10) 
			| AT91_SMC_NCS_WRPULSE_(20) | AT91_SMC_NRDPULSE_(10) | AT91_SMC_NCS_RDPULSE_(20));

	/* Cycle: 40 MCK Clocks */
	at91_sys_write(AT91_SMC_CYCLE(2), AT91_SMC_NWECYCLE_(40)
			| AT91_SMC_NRDCYCLE_(40));

	at91_sys_write(AT91_SMC_MODE(3), AT91_SMC_DBW_16 | AT91_SMC_READMODE 
			| AT91_SMC_WRITEMODE | AT91_SMC_EXNWMODE_DISABLE | AT91_SMC_TDF_(0));
	return ;
}

static int ili9325_chip_probe(struct ili9325_device *dev)
{
	u16 chip_id = 0, stat = 0;

#if 1
	//ili9325_reset();
	//msleep(150);
#endif

	/* Get chip ID. */
	chip_id = ili9325_read_reg(dev, ILI9325_DRV_CODE);
//	stat = ili9325_read_stat(dev);
	printk("%s: read addr %08x, chip_id = %04x, status = %04x\n",
			__FUNCTION__, dev->membase, chip_id, stat);

	if (ILI9325_CHIP_ID == (u16) chip_id) {
		HNOS_DEBUG_INFO("Chip ILI9325 found.\n");
		return chip_id;
	}

	//HNOS_DEBUG_INFO("No ILI9325 chip found.\n"); 
	return 0; /* Revisit  */
}

/* Power On sequence */
static void ili9325_pow_on(struct ili9325_device *dev)
{
	ili9325_write_reg(dev, ILI9325_POW_CTRL_1, 0x0000); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	ili9325_write_reg(dev, ILI9325_POW_CTRL_2, 0x0007); /* DC1[2:0], DC0[2:0], VC[2:0]  */
	ili9325_write_reg(dev, ILI9325_POW_CTRL_3, 0x0000); /* VREG1OUT voltage  */
	ili9325_write_reg(dev, ILI9325_POW_CTRL_4, 0x0000); /* VDV[4:0] for VCOM amplitude  */
	msleep(200); /* Dis-charge capacitor power voltage */

	ili9325_write_reg(dev, ILI9325_POW_CTRL_1, 0x1690); /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	ili9325_write_reg(dev, ILI9325_POW_CTRL_2, 0x0227); /* R11h=0x0221 at VCI=3.3V, DC1[2:0], DC0[2:0], VC[2:0]  */
	msleep(50);

	ili9325_write_reg(dev, ILI9325_POW_CTRL_3, 0x001D); /* External reference voltage= Vci  */
	msleep(50);

	ili9325_write_reg(dev, ILI9325_POW_CTRL_4, 0x0800); /* R13=1D00 when R12=009D;VDV[4:0] for VCOM amplitude  */
	ili9325_write_reg(dev, ILI9325_POW_CTRL_7, 0x0012); /* R29=0013 when R12=009D;VCM[5:0] for VCOMH  */
	ili9325_write_reg(dev, ILI9325_FRM_RATE_COLOR, 0x000B); /* Frame Rate = 70Hz  */
	msleep(50);

	return;
}

/* Adjust the Gamma Curve  */
static void ili9325_gamma_adjust(struct ili9325_device *dev)
{
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_1, 0x0007);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_2, 0x0707);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_3, 0x0006);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_4, 0x0704);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_5, 0x1F04);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_6, 0x0004);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_7, 0x0000);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_8, 0x0706);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_9, 0x0701);
	ili9325_write_reg(dev, ILI9325_GAMMA_CTRL_10, 0x000F);
	return;
}

/*  Partial Display Control */
static void ili9325_par_dis_ctrl(struct ili9325_device *dev)
{
	ili9325_write_reg(dev, ILI9325_PAR_IMG1_POS, 0);
	ili9325_write_reg(dev, ILI9325_PAR_IMG1_START, 0);
	ili9325_write_reg(dev, ILI9325_PAR_IMG1_END, 0);
	ili9325_write_reg(dev, ILI9325_PAR_IMG2_POS, 0);
	ili9325_write_reg(dev, ILI9325_PAR_IMG2_START, 0);
	ili9325_write_reg(dev, ILI9325_PAR_IMG2_END, 0);
	return;
}

/* Panel Control */
static void ili9325_pan_ctrl(struct ili9325_device *dev)
{
	ili9325_write_reg(dev, ILI9325_PAN_CTRL_1, 0x0010);
	ili9325_write_reg(dev, ILI9325_PAN_CTRL_2, 0x0000);
	ili9325_write_reg(dev, ILI9325_PAN_CTRL_3, 0x0003);
	ili9325_write_reg(dev, ILI9325_PAN_CTRL_4, 0x0110);
	ili9325_write_reg(dev, ILI9325_PAN_CTRL_5, 0x0000);
	ili9325_write_reg(dev, ILI9325_PAN_CTRL_6, 0x0000);
	ili9325_write_reg(dev, ILI9325_DIS_CTRL_1, 0x0133); /* 262K color and display ON */

	return;
}

static void ili9320_chip_init(struct ili9325_device *dev)
{
	printk("===---> %s.\n", __FUNCTION__);
	//ili9325_reset();

	ili9325_write_reg(dev,0x00E5,0x8000);// Set the Vcore voltage and this setting is must.        
	ili9325_write_reg(dev,0x0000,0x0001);// Start internal OSC.                                  
	//msleep(25); 

	ili9325_write_reg(dev,0x0001,0x0100);// set SS and SM bit                                    
	ili9325_write_reg(dev,0x0002,0x0700);// set 1 line inversion                                 
	ili9325_write_reg(dev,0x0003,0x9030);// set GRAM write direction and BGR=1.                  
	ili9325_write_reg(dev,0x0004,0x0000);// Resize register                                      
	ili9325_write_reg(dev,0x0008,0x0202);// set the back porch and front porch                   
	ili9325_write_reg(dev,0x0009,0x0000);// set non-display area refresh cycle ISC[3:0]          
	ili9325_write_reg(dev,0x000A,0x0000);// FMARK function                                       
	ili9325_write_reg(dev,0x000C,0x0000);// RGB interface setting                                
	ili9325_write_reg(dev,0x000D,0x0000);// Frame marker Position                                
	ili9325_write_reg(dev,0x000F,0x0000);// RGB interface polarity                               

	ili9325_write_reg(dev,0x0010,0x0000);// Power On sequence  //SAP, BT[3:0], AP, DSTB, SLP, STB
	//msleep(15);
	ili9325_write_reg(dev,0x0011,0x0007);// DC1[2:0], DC0[2:0], VC[2:0]
	ili9325_write_reg(dev,0x0012,0x0000);// VREG1OUT voltage
	ili9325_write_reg(dev,0x0013,0x0000);// VDV[4:0] for VCOM amplitude
	msleep(200);
	ili9325_write_reg(dev,0x0010,0x17B0);// SAP, BT[3:0], AP, DSTB, SLP, STB 
	//msleep(15); 
	ili9325_write_reg(dev,0x0011,0x0007);// R11h=0x0001 at VCI=3.3V DC1[2:0], DC0[2: 
	msleep(50); 
	ili9325_write_reg(dev,0x0012,0x013E);// R11h=0x0138 at VCI=3.3V VREG1OUT voltage
	msleep(50); 

	ili9325_write_reg(dev,0x0013,0x1F00);// R11h=0x1800 at VCI=2.8V VDV[4:0] for VCO  
	ili9325_write_reg(dev,0x0029,0x0013);// setting VCM for VCOMH  0018-0012          
	msleep(50); 

	ili9325_write_reg(dev,0x0020,0x0000);// GRAM horizontal Address          
	ili9325_write_reg(dev,0x0021,0x0000);// GRAM Vertical Address            

#if 0
	ili9325_write_reg(dev,0x0030,0x0000);// - Adjust the Gamma Curve -//     
	ili9325_write_reg(dev,0x0031,0x0707);                                    
	ili9325_write_reg(dev,0x0032,0x0707);                                    
	ili9325_write_reg(dev,0x0035,0x0000);                              
	ili9325_write_reg(dev,0x0036,0x001F);                                    
	ili9325_write_reg(dev,0x0037,0x0105);                                    
	ili9325_write_reg(dev,0x0038,0x0002);                                    
	ili9325_write_reg(dev,0x0039,0x0707);                                    
	ili9325_write_reg(dev,0x003C,0x0602);                                    
	ili9325_write_reg(dev,0x003D,0x1000);// - Adjust the Gamma Curve -//     
#endif
	ili9325_write_reg(dev,0x0030,0x0000);// - Adjust the Gamma Curve -//     
	ili9325_write_reg(dev,0x0031,0x0404);                                    
	ili9325_write_reg(dev,0x0032,0x0404);                                    
	ili9325_write_reg(dev,0x0035,0x0004);                              
	ili9325_write_reg(dev,0x0036,0x0404);                                    
	ili9325_write_reg(dev,0x0037,0x0404);                                    
	ili9325_write_reg(dev,0x0038,0x0404);                                    
	ili9325_write_reg(dev,0x0039,0x0707);                                    
	ili9325_write_reg(dev,0x003C,0x0500);                                    
	ili9325_write_reg(dev,0x003D,0x0607);// - Adjust the Gamma Curve -//     

	//msleep(15); 
	ili9325_write_reg(dev,0x0050,0x0000);// Horizontal GRAM Start Address    
	ili9325_write_reg(dev,0x0051,0x00EF);// Horizontal GRAM End Address      
	ili9325_write_reg(dev,0x0052,0x0000);// Vertical GRAM Start Address      
	ili9325_write_reg(dev,0x0053,0x013F);// Vertical GRAM Start Address      
	ili9325_write_reg(dev,0x0060,0x2700);// Gate Scan Line                   
	ili9325_write_reg(dev,0x0061,0x0001);// NDL,VLE, REV                     
	ili9325_write_reg(dev,0x006A,0x0000);// set scrolling line               

	ili9325_write_reg(dev,0x0080,0x0000);//- Partial Display Control -//     
	ili9325_write_reg(dev,0x0081,0x0000);
	ili9325_write_reg(dev,0x0082,0x0000);
	ili9325_write_reg(dev,0x0083,0x0000);
	ili9325_write_reg(dev,0x0084,0x0000);
	ili9325_write_reg(dev,0x0085,0x0000);

	ili9325_write_reg(dev,0x0090,0x0010);//- Panel Control -//            
	ili9325_write_reg(dev,0x0092,0x0000);                                 
	ili9325_write_reg(dev,0x0093,0x0003);                                 
	ili9325_write_reg(dev,0x0095,0x0110);                                 
	ili9325_write_reg(dev,0x0097,0x0000);                                 
	ili9325_write_reg(dev,0x0098,0x0000);//- Panel Control -//            
	ili9325_write_reg(dev,0x0007,0x0173);//Display Control and display ON 
	msleep(50); 

	//ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);
	return ;
}

static void ili9320_test1(struct ili9325_device *dev)
{
	u8 i;
	unsigned int k;
#if ILI9325_BPP		== 16
        u16 pixel = 0x1f << 11;
#else
	u32 pixel = 0x3f000;
#endif

	ili9325_write_reg(dev,0x0020,0x0000);// GRAM horizontal Address          
	ili9325_write_reg(dev,0x0021,0x0000);// GRAM Vertical Address            
	ILI9325_SET_INDEX(dev, 0x22);

	for (k=0; k<320; k++) {
		for (i=0; i<240; i++) {
#if ILI9325_BPP		== 16
			writew(pixel & 0xffff, dev->membase);
#else
			writew((pixel >> 2) & 0xffff, dev->membase);
			writew((pixel & 0x3) << 14 , dev->membase);
#endif
		}
		
	}

	return;
}

static void ili9320_test2(struct ili9325_device *dev)
{
	u8 i;
	unsigned int k;
#if ILI9325_BPP		== 16
	u16 pixel = 0x3f << 5;
#else
	u32 pixel = 0x3f << 6;
#endif

	ili9325_write_reg(dev,0x0020,0x0000);// GRAM horizontal Address          
	ili9325_write_reg(dev,0x0021,0x0000);// GRAM Vertical Address            
	ILI9325_SET_INDEX(dev, 0x22);

	for (k=0; k<320; k++) {
		for (i=0; i<240; i++) {
#if ILI9325_BPP		== 16
			writew(pixel & 0xffff, dev->membase);
#else
			writew((pixel >> 2) & 0xffff, dev->membase);
			writew((pixel & 0x3) << 14 , dev->membase);
#endif
		}
	}

	return;
}

static void ili9320_test3(struct ili9325_device *dev)
{
	u8 i;
	unsigned int k;
#if ILI9325_BPP		== 16
	u16 pixel = 0x1f;
#else
	u32 pixel = 0x3f;
#endif

	ili9325_write_reg(dev,0x0020,0x0000);// GRAM horizontal Address          
	ili9325_write_reg(dev,0x0021,0x0000);// GRAM Vertical Address            
	ILI9325_SET_INDEX(dev, 0x22);

	for (k=0; k<320; k++) {
		for (i=0; i<240; i++) {
#if ILI9325_BPP		== 16
			writew(pixel & 0xffff, dev->membase);
#else
			writew((pixel >> 2) & 0xffff, dev->membase);
			writew((pixel & 0x3) << 14 , dev->membase);
#endif
		}
	}

	return;
}


/* Start Initial Sequence */
static void ili9325_chip_init(struct ili9325_device *dev)
{
	ili9325_write_reg(dev, ILI9325_TIMING_CTRL_1, 0x3008); /* Set internal timing*/
	ili9325_write_reg(dev, ILI9325_TIMING_CTRL_2, 0x0012); /* Set internal timing*/
	ili9325_write_reg(dev, ILI9325_TIMING_CTRL_3, 0x1231); /* Set internal timing*/

	ili9325_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_1, 0x0100); /* set SS and SM bit */
	ili9325_write_reg(dev, ILI9325_LCD_DRV_CTRL, 0x0700); /* set 1 line inversion */

#if ILI9325_BPP		== 16
	printk("ili9325_chip init bpp16.\n");
	ili9325_write_reg(dev, ILI9325_ENTRY_MOD, 0x1030); /* 16bit mode, set GRAM write direction and BGR=1. */
#else
	ili9325_write_reg(dev, ILI9325_ENTRY_MOD, 0x9030); /* set GRAM write direction and BGR=1. */
#endif
	ili9325_write_reg(dev, ILI9325_RESIZE_CTRL, 0x0000); /* Resize register. */
	ili9325_write_reg(dev, ILI9325_DIS_CTRL_2, 0x0207); /* set the back porch and front porch. */
	ili9325_write_reg(dev, ILI9325_DIS_CTRL_3, 0x0000); /* set non-display area refresh cycle ISC[3:0]. */
	ili9325_write_reg(dev, ILI9325_DIS_CTRL_4, 0x0000); /* FMARK function. */
	ili9325_write_reg(dev, ILI9325_RGB_CTRL_1, 0x0000); /* RGB interface setting. */
	ili9325_write_reg(dev, ILI9325_FRAME_MARKER_POS, 0x0000); /* Frame marker Position. */
	ili9325_write_reg(dev, ILI9325_RGB_CTRL_2, 0x0000); /* RGB interface polarity. */

	ili9325_pow_on(dev);
	ili9325_gamma_adjust(dev);

	/* Set GRAM area */
	ili9325_write_reg(dev, ILI9325_HOR_ADDR_START, 0x0000); /* Horizontal GRAM Start Address. */
	ili9325_write_reg(dev, ILI9325_HOR_ADDR_END, 0x00EF); /* Horizontal GRAM End Address. */
	ili9325_write_reg(dev, ILI9325_VET_ADDR_START, 0x0000); /* Vertical GRAM Start Address. */
	ili9325_write_reg(dev, ILI9325_VET_ADDR_END, 0x013F); /* Vertical GRAM End Address. */
	ili9325_write_reg(dev, ILI9325_DRV_OUTPUT_CTRL_2, 0xA700); /* Gate Scan Line. */
	ili9325_write_reg(dev, ILI9325_BASE_IMG_CTRL, 0x0001); /* NDL,VLE, REV. */
	ili9325_write_reg(dev, ILI9325_VSCROLL_CTRL, 0x0000); /* set scrolling line. */

	ili9325_par_dis_ctrl(dev);
	ili9325_pan_ctrl(dev);

	ili9325_write_reg(dev, ILI9325_GRAM_HADDR, 0x0000); /* GRAM horizontal Address. */
	ili9325_write_reg(dev, ILI9325_GRAM_VADDR, 0x0000); /* GRAM Vertical Address. */

	ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);

	return;
}

static void ili9325_start_addr_set(struct ili9325_device *dev, u16 x, u16 y)
{
	ili9325_write_reg(dev, ILI9325_GRAM_HADDR, x); /* GRAM horizontal Address. */
	ili9325_write_reg(dev, ILI9325_GRAM_VADDR, y); /* GRAM Vertical Address. */

	ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);
	return;
}

static void ili9325_window_addr_set(struct ili9325_device *dev, 
		u16 startx, u16 endx,
		u16 starty, u16 endy )
{
	ili9325_write_reg(dev, ILI9325_HOR_ADDR_START, startx); /* Horizontal GRAM Start Address. */
	ili9325_write_reg(dev, ILI9325_HOR_ADDR_END, endx); /* Horizontal GRAM End Address. */
	ili9325_write_reg(dev, ILI9325_VET_ADDR_START, starty); /* Vertical GRAM Start Address. */
	ili9325_write_reg(dev, ILI9325_VET_ADDR_END, endy); /* Vertical GRAM End Address. */

	ILI9325_SET_INDEX(dev, ILI9325_GRAM_DATA);
	return;
}

static inline void ili9325_write_pixel(struct ili9325_device *dev, u32 pixel)
{
#if ILI9325_BPP		== 32
	u8 r, g, b; 
	u32 color = 0;

	/* 24/32-bit colors to 18-bit colors.*/
	b = (pixel & 0xff) >> 2;
	g = ((pixel >> 8 ) & 0xff) >> 2;
	r = ((pixel >> 16) & 0xff ) >> 2;
	color = (r << 12) | (g << 6) | (b << 0);

	writew((color >> 2) & 0xffff, dev->membase);
	writew((b & 0x3) << 14 , dev->membase);	

#elif ILI9325_BPP		== 16
	writew(pixel & 0xffff, dev->membase);				
#endif
	return;
}

static inline void ili9325_write_gram(struct ili9325_device *dev, u32 *buf, size_t size)
{
        size_t i = 0;
	
#if ILI9325_BPP		== 16
	u16 *pixel_buf = (u16 *) buf;
	u8 bpp = sizeof(u16);
#elif ILI9325_BPP	== 32
	u32 *pixel_buf = (u32 *) buf;
	u8 bpp = sizeof(u32);
#endif

	ili9325_write_reg(dev,0x0020,0x0000);// GRAM horizontal Address          
	ili9325_write_reg(dev,0x0021,0x0000);// GRAM Vertical Address            
	ILI9325_SET_INDEX(dev, 0x22);

	for (i=0; i<(size/bpp); i++) {
		ili9325_write_pixel(dev, pixel_buf[i]);
	}

	return;
	return;
}

static ssize_t ili9325_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{	
	struct ili9325_device *dev = filp->private_data; 
	u8 *fbbuf = (u8 *)dev->fbdata;

	if (!dev || !fbbuf) {
		return -ENODEV;
	}

	if (copy_from_user(fbbuf, buf, count)) {
		printk("%s: error occured while copy data from user.\n", __FUNCTION__);
		return -EFAULT;
	}

	ili9325_write_gram(dev, fbbuf, count);

	return count;
}

int ili9325_proc_read_id(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	struct ili9325_device *dev = data;
	int len = 0;

	int chip_id = ili9325_chip_probe(dev);
#if 0
	len += sprintf(buf, "ili9325 id :\t %04x.\n", chip_id);
#endif

	*eof = 1;
	return len;
}

static inline int ili9325_read_stat(struct ili9325_device *dev)
{
	void __iomem *data_base = dev->membase;

#if 0
	at91_set_gpio_output(AT91_PIN_PB24, 0); /* RS = 0 */
	udelay(50);
	writew(0, data_base);

	udelay(10);
	return readw(data_base);
#endif
	ili9320_test1(dev);
	msleep(1000);
	ili9320_test2(dev);
	msleep(1000);
	ili9320_test3(dev);
	return 0;
}

int ili9325_proc_read_stat(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	struct ili9325_device *dev = data;
	int len = 0;

	u16 stat = ili9325_read_stat(dev);
#if 1
	len += sprintf(buf, "ili9325 status :\t %04x.\n", stat);
#endif

	*eof = 1;
	return len;
}

static int ili9325_proc_write(struct file *file, const char __user * userbuf,
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

	return count;
}

static int ili9325_open(struct inode *inode, struct file *filp)
{
	struct ili9325_device *dev; 

	dev = container_of(inode->i_cdev, struct ili9325_device, cdev);
	filp->private_data = dev; /* for other methods */

	if (test_and_set_bit(0, &dev->is_open) != 0) {
		return -EBUSY;       
	}

	return 0; 
}

static int ili9325_release(struct inode *inode, struct file *filp)
{
	struct ili9325_device *dev = filp->private_data; 

	if (test_and_clear_bit(0, &dev->is_open) == 0) {/* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	return 0;
}


struct file_operations lcd_fops =
{
	.owner =    THIS_MODULE,
	.open =     ili9325_open,
	.release =  ili9325_release,
	.write = 	ili9325_write,
};

static int  ili9325_cdev_setup(struct ili9325_device *dev, dev_t devno)
{
	int err = 0;

	cdev_init(&dev->cdev, &lcd_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);

	if (err) { 
		printk(KERN_NOTICE "Error %d adding LCD(ili9325) device, major_%d", err, MAJOR(devno));
	}
	return err;
}

/*
 * The cleanup function is used to handle initialization failures as well.
 * Therefor, it must be careful to work correctly even if some of the items
 * have not been initialized.
 */
void ili9325_module_cleanup(void)
{
	dev_t devno = MKDEV(lcd_major, lcd_minor);	
	struct class *class;

	if (lcd_ili9325){
		/* Get rid of our char dev entries */	
		cdev_del(&lcd_ili9325->cdev);	

		class = lcd_ili9325->class;
		if (class){
			class_device_destroy(class, devno);
			class_destroy(class);
		}

		if (lcd_ili9325->membase) {
			iounmap(lcd_ili9325->membase);
			lcd_ili9325->membase = NULL;
			//release_mem_region(ILI9325_BASE_INDEX, ILI9325_IOMEM_SIZE);
		}

		if (lcd_ili9325->membase_index) {
                        iounmap(lcd_ili9325->membase_index);
                        lcd_ili9325->membase_index = NULL;
                }

		if (lcd_ili9325->fbdata) {
			kfree(lcd_ili9325->fbdata);
			lcd_ili9325->fbdata = NULL;
		}

		kfree(lcd_ili9325);
		lcd_ili9325 = NULL;
	}

	remove_proc_entry("lcd_chipid", hndl_proc_dir);
	remove_proc_entry("lcd_status", hndl_proc_dir);
	hnos_proc_rmdir();

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);
	return;
}


/*
 * Finally, the module stuff
 */
static int __init  ili9325_module_init(void)
{
	int result = 0;
	dev_t dev = 0;
	struct class *class = NULL;
	void __iomem *membase= NULL;

	struct proc_dir_entry *proc = NULL;

	/*
	 * Get a range of minor numbers to work with, asking for a dynamic
	 * major unless directed otherwise at load time.
	 */
	if (lcd_major) {
		dev = MKDEV(lcd_major, lcd_minor);
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	} else {
		result = alloc_chrdev_region(&dev, lcd_minor, 1, DEVICE_NAME);
		lcd_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "lcd ili9325: can't get major %d\n", lcd_major);
		return result;
	}	

	/* 
	 * allocate the devices -- we do not have them static.
	 */
	lcd_ili9325 = kmalloc(sizeof(struct ili9325_device), GFP_KERNEL);
	if (!lcd_ili9325) {
		result = -ENOMEM;
		goto alloc_failed;  /* Make this more graceful */
	}
	memset(lcd_ili9325, 0, sizeof(struct ili9325_device));	

	lcd_ili9325->fbdata = kmalloc(sizeof(u8) * ILI9325_FB_SIZE, GFP_KERNEL);
	if (!lcd_ili9325->fbdata) {
		printk(KERN_ERR "Can not malloc memory for lcd ili9325 frame buffer.\n");
		result = -ENOMEM;
		goto alloc_fb_failed;
	}

	spin_lock_init(&lcd_ili9325->lock);

	//ili9325_bus_init();
#if 0
	if (!request_mem_region(ILI9325_BASE_INDEX, ILI9325_IOMEM_SIZE, "ili9325")) {
		printk("%s: request mem region error.\n", __FUNCTION__);
		result = -1;
		goto region_failed;
	}
#endif

	membase = ioremap(ILI9325_BASE_DATA, ILI9325_IOMEM_SIZE);
	if (!membase) {
		printk(KERN_ERR "Can NOT remap address 0x%08x\n", ILI9325_BASE_INDEX);
		result = -1;
		goto remap_failed;
	}

	lcd_ili9325->membase = membase;
	printk("%s: phy addr %08x ==> virt %08x.\n", 
			__FUNCTION__, ILI9325_BASE_INDEX, 
			lcd_ili9325->membase);

 	lcd_ili9325->membase_index = ioremap(ILI9325_BASE_INDEX, ILI9325_IOMEM_SIZE);
	if (!lcd_ili9325->membase_index) {
		printk(KERN_ERR "Can NOT remap address 0x%08x\n", 
				(unsigned int)ILI9325_BASE_INDEX);
		result = -1;
		goto remap_failed;
	}

	/* Register a class_device in the sysfs. */
	class = class_create(THIS_MODULE, DEVICE_NAME);
	if (class == NULL) {
		result = -ENOMEM;
		goto class_failed;
	}
	lcd_ili9325->class = class;
	class_device_create(class, NULL, dev, NULL, DEVICE_NAME);

	if (ili9325_cdev_setup(lcd_ili9325, dev) < 0) {
		result = -1;
		goto cdev_failed;
	}

#if 1
	if (ili9325_chip_probe(lcd_ili9325) < 0) {
		result = -1;
		goto probe_failed;
	} 

#endif
	//ili9320_chip_init(lcd_ili9325);
	ili9325_chip_init(lcd_ili9325);

	if (!hndl_proc_dir) {
		hndl_proc_dir = hnos_proc_mkdir();
	}

	if (!hndl_proc_dir) {
		result = -ENODEV;
		goto probe_failed;
	} else {
		proc = create_proc_read_entry("lcd_chipid", S_IFREG | S_IRUGO | S_IWUSR, 
				hndl_proc_dir, ili9325_proc_read_id, lcd_ili9325);
		if (proc) {
			proc->write_proc = ili9325_proc_write;
		} else {
			result = -1;
			goto proc_entry_failed;
		}

		proc = create_proc_read_entry("lcd_status", S_IFREG | S_IRUGO | S_IWUSR, 
				hndl_proc_dir, ili9325_proc_read_stat, lcd_ili9325);
		if (proc) {
			proc->write_proc = ili9325_proc_write;
		} else {
			result = -1;
			goto proc_entry_failed;
		}

	}

	HNOS_DEBUG_INFO("Initialized device %s, major %d \n", DEVICE_NAME, lcd_major);

	return 0;

proc_entry_failed:
	hnos_proc_rmdir();

probe_failed:
	cdev_del(&lcd_ili9325->cdev);	

cdev_failed:
	if (class){
		class_device_destroy(class, dev);
		class_destroy(class);
	}

class_failed:
	iounmap(lcd_ili9325->membase);
	lcd_ili9325->membase = NULL;

remap_failed:
	iounmap(lcd_ili9325->membase_index);
	lcd_ili9325->membase_index = NULL;
	//release_mem_region(ILI9325_BASE_INDEX, ILI9325_IOMEM_SIZE);

region_failed:
	if (lcd_ili9325->fbdata) {
		kfree(lcd_ili9325->fbdata);
		lcd_ili9325->fbdata = NULL;
	}

alloc_fb_failed:
	kfree(lcd_ili9325);
	lcd_ili9325 = NULL;

alloc_failed:
	unregister_chrdev_region(dev, 1);

	return result;
}

EXPORT_SYMBOL(ili9325_back_light);

module_init(ili9325_module_init);
module_exit(ili9325_module_cleanup);

MODULE_LICENSE("Dual BSD/GPL");


