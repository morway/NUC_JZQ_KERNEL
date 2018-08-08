/*
 * drivers/char/hndl_char_devices/hnos_debug.h   
 *
 * Author zhu_xhong
 * Copyright@2010.1.29 (C) Zhejiang Huineng
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
#ifndef	HNOS_DEBUG_H
#define	HNOS_DEBUG_H

#if 0
#define dprintk(fmt...)	 printk(fmt)
#else
#define dprintk(fmt...)	 do { ; }while(0)
#endif

#define HNOS_PREFIX	"[HNOS] "
#define HNOS_DEBUG_INFO(fmt...) \
do {\
	printk(HNOS_PREFIX);\
	printk(fmt);\
}while(0)

#endif
