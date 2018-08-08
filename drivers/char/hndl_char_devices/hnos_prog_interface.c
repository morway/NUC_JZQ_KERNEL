/*
 * drivers/char/hndl_char_devices/hnos_prog_interface.c   
 * Program Button Control Interface.
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

#include "hnos_generic.h"
#include "hnos_proc.h" 
#include "hnos_prog_interface.h" 

#if 0
#undef dprintk
#define dprintk printk
#endif

static struct prog_button prog_btn;
static struct prog_btn_callback *callback = NULL;
static  struct proc_dir_entry *hndl_proc_dir = NULL;

static enum PROGRAM_STATUS prog_get_status(void)
{
	struct prog_button *btn = &prog_btn;
	return btn->stat;
}

static inline void prog_set_status(enum PROGRAM_STATUS stat)
{
	struct prog_button *btn = &prog_btn;
	unsigned long flags;

	spin_lock_irqsave(&btn->lock, flags);	
	btn->stat = stat;
	spin_unlock_irqrestore(&btn->lock, flags);	

	return;
}

int prog_callback_register(struct prog_btn_callback *cb)
{
	if (cb) {
		dprintk("%s\n", __FUNCTION__);
		callback = cb;
	}
	return 0;
}

static void prog_btn_timer(unsigned long data)
{
	enum PROGRAM_STATUS stat ;

	stat = prog_get_status();
	dprintk("%s:current PROG status:%s\n", __FUNCTION__, (stat ? "ENABLED" : "DISABELED"));

	if (PROGRAM_DISABLED == stat) {
		dprintk("%s ERROR, PROGRAM_ENABLED expected but PROGRAM_DISABLED found.\n",
				__FUNCTION__);
	}

	prog_set_status(PROGRAM_DISABLED);
	dprintk("%s: <PROGRAM_ENABLED> --> <PROGRAM_DISABLED>.\n", __FUNCTION__);

	return;
}

static void prog_btn_work(struct work_struct *work)
{
	struct prog_button *btn = &prog_btn;
	enum PROGRAM_STATUS stat ;

	if (callback && callback->btn_pressed()) { 
		dprintk("%s: PROG_BUTTON_PIN¡¡level 0.\n", __FUNCTION__);

		stat  = prog_get_status();
		dprintk("%s: PROG_BUTTON current status %s.\n", __FUNCTION__, 
				(stat ? "ENABLED" : "DISABLED"));

		if (stat == PROGRAM_ENABLED) {
			del_timer_sync(&btn->expiring_timer);
			prog_set_status(PROGRAM_DISABLED);

			dprintk("%s: <PROGRAM_ENABLED> --> <PROGRAM_DISABLED>.\n", __FUNCTION__);
		} else if (stat == PROGRAM_DISABLED) {
			prog_set_status(PROGRAM_ENABLED);

			init_timer(&btn->expiring_timer);
			btn->expiring_timer.expires = jiffies + btn->timeout;
			btn->expiring_timer.function = prog_btn_timer;
			add_timer(&btn->expiring_timer);

			dprintk("%s: <PROGRAM_DISABLED> --> <PROGRAM_ENABLED>.\n", __FUNCTION__);
		}

	} else {
		dprintk("%s: PROG_BUTTON_PIN¡¡level 1.\n", __FUNCTION__);
	}

	return;
}

/* Trigger a program button event. */
int prog_btn_event(void) 
{ 
	struct prog_button *btn = &prog_btn;

	if (callback && callback->btn_pressed()) { 
		dprintk("%s: PROG_BUTTON_PIN level 0 \n", __FUNCTION__);
		schedule_delayed_work(&btn->bh_work, PROG_WORK_DELAY);
	} else { 
		dprintk("%s: PROG_BUTTON_PIN level 1 \n", __FUNCTION__);
	} 

	return 0;
} 

int prog_set_timeout(unsigned long min)
{
	struct prog_button *btn = &prog_btn;
	unsigned long flags;
	unsigned long ticks = 0;

	if (min == 0){
		ticks = 10 * 60 * HZ;	/* default 10 minutes. */	
	} else {
		ticks = min * 60 * HZ;    
	}

	spin_lock_irqsave(&btn->lock, flags);	
	btn->timeout = ticks;
	spin_unlock_irqrestore(&btn->lock, flags);	

	dprintk("%s: set timeout %ld minutes.\n", __FUNCTION__, min);
	return 0;
}

int prog_set_work_delay(unsigned long ms)
{
	struct prog_button *btn = &prog_btn;
	unsigned long flags;
	unsigned long ticks = 0;

	if (ms < 10 || ms > 1000){
		ticks = 1 ;	/* default 10 ms. */	
	} else {
		ticks = (HZ * ms)/1000;    
	}

	spin_lock_irqsave(&btn->lock, flags);	
	btn->work_delay = ticks;
	spin_unlock_irqrestore(&btn->lock, flags);	

	dprintk("%s: set timeout %ld ticks.\n", __FUNCTION__, ticks);
	return 0;
}

int prog_enabled_status(char *buf, char **start, off_t offset,
				int count, int *eof, void *data) 
{
	enum PROGRAM_STATUS stat;
	int len = 0;

	stat = prog_get_status();

	/* In user space, prgram_enabled = 1 means you can program the module. */
	len = sprintf(buf + len, "%d\n", stat);

	return len;
}

static int prog_btn_ctrl(struct file *file, const char __user * userbuf,
						unsigned long count, void *data)
{
	int timeout = 0;
	char val[50] = {0};

	if (count >= 50){
		return -EINVAL;
	}

	if (copy_from_user(val, userbuf, count)) {
		return -EFAULT;
	}

	if (sscanf(val, "timeout %d", &timeout) != 1){
		return -EINVAL;
	}

	dprintk(KERN_INFO "%s: buf %s, timeout = %d\n",
			__FUNCTION__, val, timeout);

	prog_set_timeout(timeout);

	return count;
}

static __init int prog_btn_init(void)
{
	struct prog_button *btn = &prog_btn;
	struct proc_dir_entry *proc = NULL;
	int result = 0;

	if (!btn) {
		printk(KERN_ERR "NULL prog_button.\n");
		return -1;
	}

	spin_lock_init(&btn->lock);
	btn->stat = PROGRAM_DISABLED;

	INIT_DELAYED_WORK(&btn->bh_work, prog_btn_work);
	btn->work_delay = PROG_WORK_DELAY;
	btn->timeout = PROG_TIME_OUT;               

	if (!hndl_proc_dir) {
		hndl_proc_dir = hnos_proc_mkdir();
	}

	if (!hndl_proc_dir) {
		result = -ENODEV;
		goto proc_failed;
	} else {
		proc = create_proc_read_entry("program_enabled", S_IFREG | S_IRUGO | S_IWUSR, 
				hndl_proc_dir, prog_enabled_status, NULL);
		if (proc) {
			proc->write_proc = prog_btn_ctrl;
		} else {
			result = -1;
			goto proc_entry_failed;
		}
	}

	HNOS_DEBUG_INFO("Program Enabled Button registered.\n");
	return 0;

proc_entry_failed:
	hnos_proc_rmdir();
proc_failed:
	return result;
}


static __exit void prog_btn_exit(void)
{
	struct prog_button *btn = &prog_btn;
	if (!btn) {
		printk(KERN_ERR "NULL prog_button.\n");
		return;
	}

	cancel_delayed_work(&btn->bh_work);

	remove_proc_entry("program_enabled", hndl_proc_dir);
	hnos_proc_rmdir();

	del_timer_sync(&btn->expiring_timer);
	btn->stat = PROGRAM_DISABLED;

	HNOS_DEBUG_INFO("Program Enabled Button unregistered.\n");
	return;
}

module_init(prog_btn_init);
module_exit(prog_btn_exit);

EXPORT_SYMBOL(prog_callback_register);
EXPORT_SYMBOL(prog_btn_event);

MODULE_LICENSE("Dual BSD/GPL");

