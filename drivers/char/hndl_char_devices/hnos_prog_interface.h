/*
 * drivers/char/hndl_char_devices/hnos_prog_interface.h   
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
#ifndef __HNDL_HNOS_PROG_H
#define __HNDL_HNOS_PROG_H

#define DEVICE_NAME    "prog_button"

#define PROG_TIME_OUT		(10 * 60 * HZ) /* 10 minute. */
#define PROG_WORK_DELAY          (1)

#define RESET_HOLD_SECS		(3)            /* 3 seconds */
#define SCAN_INTERVAL_TICKS	(HZ/100)       /* 1 tick, 10ms  */
#define MAX_SCAN_TIMES		300            /* 300 times. */

enum PROGRAM_STATUS
{
	PROGRAM_DISABLED = 0,
	PROGRAM_ENABLED = 1,
};

struct prog_button 
{
	spinlock_t lock;
	enum PROGRAM_STATUS stat; 
	struct delayed_work bh_work;       /* Delayed work used as the Bottom Half Work .*/
	unsigned long work_delay;          /* delay for dithering. */
	struct timer_list expiring_timer;  /* Timer for program enabled button. */
	unsigned long timeout;		   /* How many minutes will program_enabled status last for. */
};

struct prog_btn_callback 
{
	int (*btn_pressed)(void);
};

int prog_btn_event(void);
int prog_callback_register(struct prog_btn_callback *cb);

#endif
