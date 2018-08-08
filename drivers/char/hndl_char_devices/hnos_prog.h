/*
 * /drivers/char/hndl_char_devices/at91_prog_button.h
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

#ifndef _AT91_PROG_BUTTON_H
#define _AT91_PROG_BUTTON_H

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

struct at91_prog_button 
{
	spinlock_t lock;
	unsigned pin;

	enum PROGRAM_STATUS stat; 

	struct delayed_work bh_work;       /* Delayed work used as the Bottom Half Work for the GPIO level change interrupt.*/
	unsigned long work_delay;          /* delay for dithering. */

	struct timer_list expiring_timer;  /* Timer for program enabled button. */
	unsigned long timeout;		   /* How many minutes will program_enabled status last for. */

	struct delayed_work reset_work;    /* Delayed work, used to call user mode helper "/sbin/reboot". */
	struct timer_list reset_timer;     /* Timer used to check if the RESET button was pressed for 3 seconds. */

	int scan_times;
	int btn_released;
};


enum PROGRAM_STATUS prog_get_status(void);
int prog_set_timeout(unsigned long min);
int prog_set_work_delay(unsigned long ms);

#endif


