/*
 * drivers/char/hndl_char_devices/hnos_esam_intf.h   
 *
 *        wlh, Liehua76@sohu.com
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
#ifndef		HNOS_ESAM_INTF_H
#define		HNOS_ESAM_INTF_H

#define ESAM_BUF_SIZE	2048//1024
		
struct  esam_spi_device
{
	struct cdev cdev;
	struct class *class;
	struct spi_device *spi;
    u8     tx_buf[ESAM_BUF_SIZE];
	u8     rx_buf[ESAM_BUF_SIZE];
	unsigned long is_open;
};

#endif
