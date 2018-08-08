/*
 * /drivers/char/hndl_char_devices/hnos_lcd_cog.h
 *
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
#ifndef __UC1698FB_H
#define __UC1698FB_H


struct at91_lcd_cdev 
{
    unsigned long is_open;	  
    struct class *myclass;
    struct cdev cdev;	       

    spinlock_t lock;
     unsigned int ioaddr_cmd;
     unsigned int ioaddr_data;

};

typedef void (*init_helper_t)(void);
typedef void (*reset_helper_t)(void);
typedef void (*blt_helper_t)(unsigned char action);

struct uc1698fb_callbacks {
    init_helper_t  init;
    reset_helper_t reset;
    blt_helper_t   backlight;
};

int uc1698fb_callbacks_register(struct uc1698fb_callbacks *cbs);
int uc1698fb_callbacks_unregister(struct uc1698fb_callbacks *cbs);

/* SMC(Static Memory Controller) parameters */
#define		UC1698FB_SMC_CHIP	2
#define		UC1698FB_CMD_ADDR	(0x30000000 + 0x20)
#define		UC1698FB_IOMEM_SIZE	0x3

/* UC1698U: Chip ID. */
#define		UC1698FB_CHIP_STATUS	0x92
#define		UC1698FB_PRODUCT_CODE	0x80

/* CMD Table */
 int my_writeb(unsigned char  dat,unsigned int flag);
/* CMD 4: Set Column Address */
#define		UC1698FB_SET_COLUMN_ADDR(cmd_port, addr) \
	do { \
		my_writeb(0x00|((addr)&0xf), cmd_port); \
		my_writeb(0x10|(((addr)&0x70)>>4), cmd_port); \
	}while(0)

/* CMD 5: Set Temp Compensation */
#define		UC1698FB_SET_TEMP_COMP(cmd_port, val) \
	do {\
		my_writeb(0x24|((val)&0x3), cmd_port); \
	}while(0)

/* CMD 6: Set Power Control */
#define		UC1698FB_SET_POWER_CNTRL(cmd_port, val) \
	do {\
		my_writeb(0x28|((val)&0x3), cmd_port); \
	}while(0)

/* CMD 7: Set Adv. Power Control */
/* NOTE: APC R = 1. */
#define		UC1698FB_SET_ADV_POWER(cmd_port, val) \
	do {\
		my_writeb(0x31, cmd_port); \
		my_writeb((val), cmd_port); \
	}while(0)

/* CMD 8: Set Scroll Line */
#define		UC1698FB_SET_SCROLL_LINE(cmd_port, line) \
	do {\
		my_writeb(0x40|((line)&0xf), cmd_port); \
		my_writeb(0x50|(((line)&0xf0)>>4), cmd_port); \
	}while(0)

/* CMD 9: Set Row Address */
#define		UC1698FB_SET_ROW_ADDRS(cmd_port, addr) \
	do {\
		my_writeb(0x60|((addr)&0xf), cmd_port); \
		my_writeb(0x70|(((addr)&0xf0)>>4), cmd_port); \
	}while(0)

/* CMD 10: Set Vbias Potentiometer */
#define		UC1698FB_SET_VBIAS_PM(cmd_port, val) \
	do {\
		my_writeb(0x81, cmd_port); \
		my_writeb((val), cmd_port); \
	}while(0)

/* CMD 11: Set Partial Display Control */
#define		UC1698FB_SET_PARTIAL_CNTRL(cmd_port, val) \
	do {\
		my_writeb(0x84|(((val)&0x1)), cmd_port); \
	}while(0)

/* CMD 12: Set RAM Address Control */
#define		UC1698FB_SET_RAM_ADDR_CNTRL(cmd_port, val) \
	do {\
		my_writeb(0x88|(((val)&0x7)), cmd_port); \
	}while(0)

/* CMD 13: Set Fixed Line */
#define		UC1698FB_SET_FIXED_LINE(cmd_port, line) \
	do {\
		my_writeb(0x90, cmd_port); \
		my_writeb((line), cmd_port); \
	}while(0)

/* CMD 14: Set Line Rate */
#define		UC1698FB_SET_LINE_RATE(cmd_port, rate) \
	do {\
		my_writeb(0xa0|((rate)&0x3), cmd_port); \
	}while(0)

/* CMD 15: Set All Pixel ON */
#define		UC1698FB_SET_PIXEL_ON(cmd_port, on) \
	do {\
		my_writeb(0xa4|((on)&0x1), cmd_port); \
	}while(0)

/* CMD 16: Set Inverse Display */
#define		UC1698FB_SET_INVERSE_DISP(cmd_port, val) \
	do {\
		my_writeb(0xa6|((val)&0x1), cmd_port); \
	}while(0)

/* CMD 17: Set Display Enable */
#define		UC1698FB_SET_DISPLAY_ENABLE(cmd_port, val) \
	do {\
		my_writeb(0xa8|((val)&0x7), cmd_port); \
	}while(0)

/* CMD 18: Set LCD Mapping Control */
#define		UC1698FB_SET_MAPPING_CNTRL(cmd_port, val) \
	do {\
		my_writeb(0xc0|((val)&0x7), cmd_port); \
	}while(0)

/* CMD 19: Set N-line Inversion */
#define		UC1698FB_SET_NLINE_INV(cmd_port, nline) \
	do {\
		my_writeb(0xc8, cmd_port); \
		my_writeb((nline)&0x1f, cmd_port); \
	}while(0)

/* CMD 20: Set Color Pattern */
#define		UC1698FB_SET_COLOR_PATTERN(cmd_port, rgb) \
	do {\
		my_writeb(0xd0|((rgb)&0x1), cmd_port); \
	}while(0)

/* CMD 21: Set Color Mode */
#define		UC1698FB_SET_COLOR_MODE(cmd_port, mode) \
	do {\
		my_writeb(0xd4|((mode)&0x3), cmd_port); \
	}while(0)

/* CMD 22: Set COM Scan Function */
#define		UC1698FB_SET_SCAN_FUNC(cmd_port, val) \
	do {\
		my_writeb(0xd8|((val)&0x7), cmd_port); \
	}while(0)

/* CMD 23: System Reset */
#define		UC1698FB_SYSTEM_RESET(cmd_port) \
	do {\
		my_writeb(0xe2, cmd_port); \
	}while(0)

/* CMD 24: NOP */
#define		UC1698FB_SYSTEM_NOP(cmd_port) \
	do {\
		my_writeb(0xe3, cmd_port); \
	}while(0)

/* CMD 26: Set LCD Bias Ratio */
#define		UC1698FB_SET_BIAS_RATIO(cmd_port, ratio) \
	do {\
		my_writeb(0xe8|((ratio)&0x3), cmd_port); \
	}while(0)

/* CMD 27: Set COM End */
#define		UC1698FB_SET_COM_END(cmd_port, line) \
	do {\
		my_writeb(0xf1, cmd_port); \
		my_writeb((line), cmd_port); \
	}while(0)

/* CMD 28: Set Partial Display Start */
#define		UC1698FB_SET_PARTIAL_START(cmd_port, line) \
	do {\
		my_writeb(0xf2, cmd_port); \
		my_writeb((line)&0x7f, cmd_port); \
	}while(0)

/* CMD 29: Set Partial Display End */
#define		UC1698FB_SET_PARTIAL_END(cmd_port, line) \
	do {\
		my_writeb(0xf3, cmd_port); \
		my_writeb((line)&0x7f, cmd_port); \
	}while(0)

/* CMD 30: Set Window Program Starting Column Address */
#define		UC1698FB_SET_WINDOW_COLUMN_START(cmd_port, val) \
	do {\
		my_writeb(0xf4, cmd_port); \
		my_writeb((val)&0x7f, cmd_port); \
	}while(0)

/* CMD 31: Set Window Program Starting Row Address */
#define		UC1698FB_SET_WINDOW_ROW_START(cmd_port, val) \
	do {\
		my_writeb(0xf5, cmd_port); \
		my_writeb((val), cmd_port); \
	}while(0)

/* CMD 32: Set Window Program Ending Column Address */
#define		UC1698FB_SET_WINDOW_COLUMN_END(cmd_port, val) \
	do {\
		my_writeb(0xf6, cmd_port); \
		my_writeb((val)&0x7f, cmd_port); \
	}while(0)

/* CMD 33: Set Window Program Ending Row Address */
#define		UC1698FB_SET_WINDOW_ROW_END(cmd_port, val) \
	do {\
		my_writeb(0xf7, cmd_port); \
		my_writeb((val), cmd_port); \
	}while(0)


/* CMD 34: Set Window Program Mode */
#define		UC1698FB_SET_WINDOW_MODE(cmd_port, mode) \
	do {\
		my_writeb(0xf8|((mode)&0x1), cmd_port); \
	}while(0)


/* LCD Size */
#define		UC1698FB_ROW_SIZE	160
#define		UC1698FB_COLUMN_SIZE	160
#define		UC1698FB_COLUMN_START	112
#define		UC1698FB_BIT_PER_PIXEL	1

#define		UC1698FB_GPIO_RESET_PIN		NUC970_PB0
//#define		UC1698FB_LCD_BACKLIGHT_PIN	NUC970_PB4

#define		UC1698FB_LCD_BACKLIGHT_ON	1
#define		UC1698FB_LCD_BACKLIGHT_OFF	0

#endif

