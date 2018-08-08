/*
 * drivers/char/hndl_char_devices/hnos_prog.c   
 * Netmeter Program Button control.
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
#include "hnos_prog.h" 

static struct at91_prog_button prog_btn;
static  struct proc_dir_entry *hndl_proc_dir = NULL;

static void prog_reset_btn_work(struct work_struct *work)
{
	static char *envp[] = { "HOME=/", "TERM=linux", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
	char *argv[] = { "/sbin/reboot", NULL };

	printk(KERN_CRIT "Reset the system now.\n");

	if (call_usermodehelper("/sbin/reboot", argv, envp, 0) < 0) {
		printk(KERN_CRIT "Reboot execution failed\n");
	}

	return;
}

static void prog_reset_btn_timer(unsigned long data)
{
	struct at91_prog_button *btn = &prog_btn;
	unsigned long flags;

	if (at91_get_gpio_value(btn->pin) == 1) { 
		dprintk("%s: reset button was released, scan_times = %d.\n",
				__FUNCTION__, btn->scan_times);
		spin_lock_irqsave(&btn->lock, flags);
		btn->btn_released = 1;  /* Reset button has been released.*/
		btn->scan_times = MAX_SCAN_TIMES;
		spin_unlock_irqrestore(&btn->lock, flags);

		return;
	}

	dprintk("%s: reset button was still pressed, scan_times = %d.\n",
			__FUNCTION__, btn->scan_times);

	spin_lock_irqsave(&btn->lock, flags);
	btn->scan_times--;
	spin_unlock_irqrestore(&btn->lock, flags);

	if (!btn->scan_times && !btn->btn_released) {	/* Reset button has been pressed for 3 seconds. */
		printk("\nWARNING: REBOOT the system since the reset button has been pressed for 3 seconds.\n");
		schedule_delayed_work(&btn->reset_work, PROG_WORK_DELAY);	/* REBOOT the system. */
	} else {	 /* Re-Add the timer. */
		btn->reset_timer.expires = jiffies + SCAN_INTERVAL_TICKS;
		btn->reset_timer.function = prog_reset_btn_timer;
		add_timer(&btn->reset_timer);
	}

	return;
}

enum PROGRAM_STATUS prog_get_status(void)
{
	struct at91_prog_button *btn = &prog_btn;
	return btn->stat;
}
EXPORT_SYMBOL(prog_get_status);

static inline void prog_set_status(enum PROGRAM_STATUS stat)
{
	struct at91_prog_button *btn = &prog_btn;
	unsigned long flags;

	spin_lock_irqsave(&btn->lock, flags);	
	btn->stat = stat;
	spin_unlock_irqrestore(&btn->lock, flags);	

	return;
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
	struct at91_prog_button *btn = &prog_btn;
	unsigned long flags;
	enum PROGRAM_STATUS stat ;

	if (at91_get_gpio_value(btn->pin) == 0) { 
		dprintk("%s: PROG_BUTTON_PIN¡¡level 0.\n", __FUNCTION__);

		spin_lock_irqsave(&btn->lock, flags);
		btn->btn_released = 0;
		btn->scan_times = MAX_SCAN_TIMES;
		spin_unlock_irqrestore(&btn->lock, flags);

#if !defined (CONFIG_HNDL_PRODUCT_HNDL1000X)
		init_timer(&btn->reset_timer);
		btn->reset_timer.expires = jiffies + SCAN_INTERVAL_TICKS;
		btn->reset_timer.function = prog_reset_btn_timer;
		add_timer(&btn->reset_timer);
#endif

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


/* Interrupt Handler for the PROG (RESET) Button */
static irqreturn_t prog_btn_irq(int irq, void *dev_id) 
{ 
	int  handled = 1;
	struct at91_prog_button *btn = &prog_btn;

	if (at91_get_gpio_value(btn->pin) == 0) { 
		dprintk("%s: PROG_BUTTON_PIN level 0 \n", __FUNCTION__);
		schedule_delayed_work(&btn->bh_work, PROG_WORK_DELAY);
	} else { 
		dprintk("%s: PROG_BUTTON_PIN level 1 \n", __FUNCTION__);
	} 

	return IRQ_RETVAL(handled);  
} 

int prog_set_timeout(unsigned long min)
{
	struct at91_prog_button *btn = &prog_btn;
	unsigned long flags;
	unsigned long ticks = 0;

	if (min == 0 || min > 30){
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
	struct at91_prog_button *btn = &prog_btn;
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

	/* In user space, prgram_enabled=1 means you can program the module. */
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

	return 0;
}


static __init inline void prog_gpio_cfg(struct at91_prog_button *btn)
{
#if defined (CONFIG_HNDL_PRODUCT_HNDL1000X)
		btn->pin = AT91_PIN_PA18;
#else
	u16 id = netmeter_id_mask_get();
	if (PRODUCT_HNDL800A_V100 == id) {
		btn->pin = AT91_PIN_PB29;
	} else {
		btn->pin = AT91_PIN_PB17;
	}
#endif
	return;
}

static __init int prog_btn_init(void)
{
	struct at91_prog_button *btn = &prog_btn;
	struct proc_dir_entry *proc = NULL;
	int result = 0;

	if (!btn) {
		printk(KERN_ERR "NULL at91_prog_button.\n");
		return -1;
	}

	spin_lock_init(&btn->lock);
	btn->stat = PROGRAM_DISABLED;

	INIT_DELAYED_WORK(&btn->bh_work, prog_btn_work);
	btn->work_delay = PROG_WORK_DELAY;
	btn->timeout = PROG_TIME_OUT;               

#if !defined (CONFIG_HNDL_PRODUCT_HNDL1000X)
	/* The program button may be used as the RESET button. */
	INIT_DELAYED_WORK(&btn->reset_work, prog_reset_btn_work);
	btn->btn_released = 0;
	btn->scan_times = MAX_SCAN_TIMES;
#endif

	prog_gpio_cfg(btn);
	at91_set_gpio_input(btn->pin, 1);      /* Set user button as input */
	at91_set_deglitch(btn->pin, 1);        /* Set glitch filter on button */

	/*Request IRQ's for the button */
	if (request_irq(btn->pin, prog_btn_irq, 0, DEVICE_NAME, NULL)) {
		return -EBUSY; 
	}

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

	HNOS_DEBUG_INFO("Program Enabled Button, (GPIO) IRQ %d.\n", btn->pin);
	return 0;

proc_entry_failed:
	hnos_proc_rmdir();
proc_failed:
	free_irq(btn->pin, NULL);
	return result;
}


static __exit void prog_btn_exit(void)
{
	struct at91_prog_button *btn = &prog_btn;
	if (!btn) {
		printk(KERN_ERR "NULL at91_prog_button.\n");
		return;
	}

	remove_proc_entry("program_enabled", &proc_root);
	hnos_proc_rmdir();

	del_timer_sync(&btn->expiring_timer);
	
#if !defined (CONFIG_HNDL_PRODUCT_HNDL1000X)
	del_timer_sync(&btn->reset_timer);
#endif

	btn->stat = PROGRAM_DISABLED;
	free_irq(btn->pin, NULL);
	return;
}

EXPORT_SYMBOL(prog_set_timeout);
EXPORT_SYMBOL(prog_set_work_delay);

module_init(prog_btn_init);
module_exit(prog_btn_exit);

MODULE_LICENSE("Dual BSD/GPL");

