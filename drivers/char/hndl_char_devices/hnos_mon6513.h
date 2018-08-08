/*
 * drivers/char/hndl_char_devices/hnos_mon6513.h
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

#ifndef		HNOS_MON6513_H
#define		HNOS_MON6513_H

#define		PIN_6513_RESET		AT91_PIN_PC1
#define		MON6513_DEFAULT_TO	HZ

enum	CHIP_STAUS
{
	DATA6513_VALID = 0,
	DATA6513_INVALID = 1,
};

struct mon6513_data
{
	spinlock_t lock;
	enum CHIP_STAUS stat; 
	unsigned long rst_cnt;
	struct delayed_work bh_work;       
	struct timer_list timer; 
	unsigned long timeout;		 
};

#endif



