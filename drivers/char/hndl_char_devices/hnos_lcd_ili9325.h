/*
 * drivers/char/hndl_char_devices/hnos_lcd_ili9325.h
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
#ifndef __HNOS_LCD_ILI9325_H
#define __HNOS_LCD_ILI9325_H


/* ILI9325 address base. */
#define ILI9325_BASE_INDEX			UL(0x30000020)
#define ILI9325_BASE_DATA			UL(0x300000a0)

#define ILI9325_IOMEM_SIZE			0x8

#define ILI9325_X_RES				240
#define ILI9325_Y_RES				320
#define ILI9325_BPP					32

//#define ILI9325_FB_SIZE				(ILI9325_X_RES * ILI9325_Y_RES * ILI9325_BPP / 8)
#define ILI9325_FB_SIZE				(ILI9325_X_RES * ILI9325_Y_RES * ILI9325_BPP / 8)


/* ILI9325 ID */
#define ILI9325_CHIP_ID				0x9325

/* ILI9325 control registers */

#define ILI9325_DRV_CODE			0x00  /* Driver code read, RO.*/
#define ILI9325_DRV_OUTPUT_CTRL_1		0x01  /* Driver output control 1, W.*/
#define ILI9325_LCD_DRV_CTRL			0x02  /* LCD driving control, W.*/
#define ILI9325_ENTRY_MOD			0x03  /* Entry mode, W.*/
#define ILI9325_RESIZE_CTRL			0x04  /* Resize Control, W.*/

#define ILI9325_DIS_CTRL_1			0x07  /* Display Control 1, W.*/
#define ILI9325_DIS_CTRL_2			0x08  /* Display Control 2, W.*/
#define ILI9325_DIS_CTRL_3			0x09  /* Display Control 3, W.*/
#define ILI9325_DIS_CTRL_4			0x0A  /* Display Control 4, W.*/

#define ILI9325_RGB_CTRL_1			0x0C  /* RGB display interface control 1, W.*/
#define ILI9325_FRAME_MARKER_POS		0x0D  /* Frame Marker Position, W.*/
#define ILI9325_RGB_CTRL_2			0x0F  /* RGB display interface control 2, W.*/

#define ILI9325_POW_CTRL_1			0x10  /* Power control 1, W.*/
#define ILI9325_POW_CTRL_2			0x11  /* Power control 2, W.*/
#define ILI9325_POW_CTRL_3			0x12  /* Power control 3, W.*/
#define ILI9325_POW_CTRL_4			0x13  /* Power control 4, W.*/

#define ILI9325_GRAM_HADDR			0x20  /* Horizontal GRAM address set, W.*/
#define ILI9325_GRAM_VADDR			0x21  /* Vertical GRAM address set, W.*/
#define ILI9325_GRAM_DATA			0x22  /* Write Data to GRAM, W.*/

#define ILI9325_POW_CTRL_7			0x29  /* Power control 7, W.*/

#define ILI9325_FRM_RATE_COLOR			0x2B  /* Frame Rate and Color Control, W. */

#define ILI9325_GAMMA_CTRL_1			0x30  /* Gamma Control 1, W. */
#define ILI9325_GAMMA_CTRL_2			0x31  /* Gamma Control 2, W. */
#define ILI9325_GAMMA_CTRL_3			0x32  /* Gamma Control 3, W. */
#define ILI9325_GAMMA_CTRL_4			0x35  /* Gamma Control 4, W. */
#define ILI9325_GAMMA_CTRL_5			0x36  /* Gamma Control 5, W. */
#define ILI9325_GAMMA_CTRL_6			0x37  /* Gamma Control 6, W. */
#define ILI9325_GAMMA_CTRL_7			0x38  /* Gamma Control 7, W. */
#define ILI9325_GAMMA_CTRL_8			0x39  /* Gamma Control 8, W. */
#define ILI9325_GAMMA_CTRL_9			0x3C  /* Gamma Control 9, W. */
#define ILI9325_GAMMA_CTRL_10			0x3D  /* Gamma Control l0, W. */

#define ILI9325_HOR_ADDR_START			0x50  /* Horizontal Address Start, W. */
#define ILI9325_HOR_ADDR_END			0x51  /* Horizontal Address End Position, W. */
#define ILI9325_VET_ADDR_START			0x52  /* Vertical Address Start, W. */
#define ILI9325_VET_ADDR_END			0x53  /* Vertical Address Start, W. */

#define ILI9325_DRV_OUTPUT_CTRL_2		0x60  /* Driver output control 2, W.*/

#define ILI9325_BASE_IMG_CTRL			0x61  /* Base Image Display Control, W.*/
#define ILI9325_VSCROLL_CTRL			0x6A  /* Vertical Scroll Control, W.*/

#define ILI9325_PAR_IMG1_POS			0x80  /* Partial Image 1 Display Position, W.*/
#define ILI9325_PAR_IMG1_START			0x81  /* Partial Image 1 Area (Start Line), W.*/
#define ILI9325_PAR_IMG1_END			0x82  /* Partial Image 1 Area (End Line), W.*/
#define ILI9325_PAR_IMG2_POS			0x83  /* Partial Image 2 Display Position, W.*/
#define ILI9325_PAR_IMG2_START			0x84  /* Partial Image 2 Area (Start Line), W.*/
#define ILI9325_PAR_IMG2_END			0x85  /* Partial Image 2 Area (End Line), W.*/

#define ILI9325_PAN_CTRL_1			0x90  /* Panel Interface Control 1, W.*/
#define ILI9325_PAN_CTRL_2			0x92  /* Panel Interface Control 2, W.*/
#define ILI9325_PAN_CTRL_3			0x93  /* Panel Interface Control 3, W.*/
#define ILI9325_PAN_CTRL_4			0x95  /* Panel Interface Control 4, W.*/


#define ILI9325_OTP_PROG_CTRL			0xA1  /* OTP VCM Programming Control, W.*/
#define ILI9325_OTP_STATUS			0xA2  /* OTP VCM Status and Enable, W.*/
#define ILI9325_OTP_ID_KEY			0xA5  /* OTP Programming ID Key, W.*/

/* Registers not depicted on the datasheet :-( */
#define ILI9325_TIMING_CTRL_1			0xE3  /* Timing control. */
#define ILI9325_TIMING_CTRL_2			0xE7  /* Timing control. */
#define ILI9325_TIMING_CTRL_3			0xEF  /* Timing control. */

#define ILI9325_PAN_CTRL_5			0x97  /* Panel Interface Control 5, W.*/
#define ILI9325_PAN_CTRL_6			0x98  /* Panel Interface Control 6, W.*/

#define ILI9325_SET_INDEX(dev, reg) \
	do {\
		writew(reg, dev->membase_index);\
	}while(0)

#define ILI9325_LCD_RST			AT91_PIN_PB31
#define ILI9325_LCD_BLT			AT91_PIN_PB30

#define LCD_BLT_ON				1
#define LCD_BLT_OFF				0

void ili9325_back_light(unsigned char cmd);

#endif
