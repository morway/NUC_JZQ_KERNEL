/*
 * drivers/char/hndl_char_devices/hnos_mon6513.c  
 *
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
#include "hnos_mon6513.h" 

#if 1
#undef dprintk
#define dprintk printk
#endif

static struct mon6513_data *mon6513;
static struct proc_dir_entry *hndl_proc_dir;

static void  mon6513_set_status(struct mon6513_data *mon, enum CHIP_STAUS stat)
{
	unsigned long flags;

	spin_lock_irqsave(&mon->lock, flags);
	mon->stat = stat;
	spin_unlock_irqrestore(&mon->lock, flags);

	return ;
}

static enum CHIP_STAUS  mon6513_get_status(struct mon6513_data *mon)
{
	enum CHIP_STAUS stat;
	unsigned long flags;

	spin_lock_irqsave(&mon->lock, flags);
	stat = mon->stat;
	spin_unlock_irqrestore(&mon->lock, flags);

	return stat;
}

static void mon6513_timer_fn(unsigned long data)
{
	struct mon6513_data *mon = (struct mon6513_data*)data;

	dprintk("%s: <DATA6513_INVALID> --> <DATA6513_VALID>.\n", __FUNCTION__);
	mon6513_set_status(mon, DATA6513_VALID);

	return;
}

static inline void mon6513_rst_cnt(struct mon6513_data *mon)
{
	unsigned long flags;

	spin_lock_irqsave(&mon->lock, flags);
	mon->rst_cnt ++;
	spin_unlock_irqrestore(&mon->lock, flags);

	return;
}

static void mon6513_work_fn(struct work_struct *work)
{
	struct mon6513_data *mon = mon6513;
	unsigned int pin_level = at91_get_gpio_value(PIN_6513_RESET);
	enum CHIP_STAUS stat = DATA6513_VALID;

	if (0 == pin_level) { 
		dprintk("%s:_PIN¡¡level 0.\n", __FUNCTION__);
		mon6513_rst_cnt(mon);

		stat = mon6513_get_status(mon);
		if ( DATA6513_INVALID == stat ) {
			mod_timer(&mon->timer, jiffies + mon->timeout);
		} else if ( DATA6513_VALID == stat ) {
			mon6513_set_status(mon, DATA6513_INVALID);

			init_timer(&mon->timer);
			mon->timer.expires = jiffies + mon->timeout;
			mon->timer.function = mon6513_timer_fn;
			add_timer(&mon->timer);

			dprintk("%s: <DATA6513_VALID> --> <DATA6513_INVALID>.\n", __FUNCTION__);
		}

	} else {
		dprintk("%s: PIN¡¡level 1.\n", __FUNCTION__);
	}

	return;
}

int mon6513_set_timeout(unsigned long sec)
{
	struct mon6513_data *mon = mon6513;
	unsigned long flags;
	unsigned long ticks = 0;

	if (sec == 0 || sec > 10){
		ticks = HZ;	/* default 1 seconds. */	
	} else {
		ticks = sec * HZ;    
	}

	spin_lock_irqsave(&mon->lock, flags);	
	mon->timeout = ticks;
	spin_unlock_irqrestore(&mon->lock, flags);	

	dprintk("%s: set timeout %ld seconds.\n", __FUNCTION__, sec);
	return 0;
}

int mon6513_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data) 
{
	int len = 0;
	enum CHIP_STAUS stat;
	struct mon6513_data *mon = (struct mon6513_data *)data;

	stat = mon6513_get_status(mon);

	len = sprintf(buf + len, "data %s, reset %ld time%s\n", 
			( (DATA6513_VALID == stat) ? "valid" : "invalid" ), 
			mon->rst_cnt,
			((mon->rst_cnt > 1) ? "s" : ""));

	return len;
}

static int mon6513_proc_write(struct file *file, const char __user * userbuf, unsigned long count, void *data)
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

	mon6513_set_timeout(timeout);

	return count;
}

static irqreturn_t mon6513_irq_fn(int irq, void *dev_id) 
{ 
	struct mon6513_data *mon = (struct mon6513_data *)dev_id;

	if (at91_get_gpio_value(PIN_6513_RESET) == 0) { 
		schedule_delayed_work(&mon->bh_work, 1);
	} 

	return IRQ_RETVAL(1);  
} 


static __init int mon6513_module_init(void)
{
	struct proc_dir_entry *proc = NULL;
	int result = 0;
	
	mon6513 = kmalloc(sizeof(struct mon6513_data), GFP_KERNEL);
	if (!mon6513) {
		printk(KERN_ERR "No enough memory.\n");
		return -ENOMEM;
	}
	memset(mon6513, 0, sizeof(struct mon6513_data));

	spin_lock_init(&mon6513->lock);
	mon6513->stat = DATA6513_VALID;

	INIT_DELAYED_WORK(&mon6513->bh_work, mon6513_work_fn);
	mon6513->timeout = MON6513_DEFAULT_TO;               

	at91_set_gpio_input(PIN_6513_RESET, 1);   
	at91_set_deglitch(PIN_6513_RESET, 1);    
	if (request_irq(PIN_6513_RESET, mon6513_irq_fn, 0, "mon6513", mon6513)) {
		result = -EBUSY; 
		goto irq_failed;
	}


	if (!hndl_proc_dir) {
		hndl_proc_dir = hnos_proc_mkdir();
	}

	if (!hndl_proc_dir) {
		result = -ENODEV;
		goto proc_failed;
	} else {
		proc = create_proc_read_entry("mon6513", S_IFREG | S_IRUGO | S_IWUSR, 
				hndl_proc_dir, mon6513_proc_read, mon6513);
		if (proc) {
			proc->write_proc = mon6513_proc_write;
		} else {
			result = -1;
			goto proc_entry_failed;
		}
	}

	HNOS_DEBUG_INFO("Monitor for chip TDK6513 registered.\n");
	return 0;

proc_entry_failed:
	hnos_proc_rmdir();
proc_failed:
	free_irq(PIN_6513_RESET, mon6513);
irq_failed:
	kfree(mon6513);
	return result;
}


static __exit void mon6513_module_exit(void)
{
	free_irq(PIN_6513_RESET, mon6513);
	remove_proc_entry("mon6513", hndl_proc_dir);
	hnos_proc_rmdir();
	del_timer_sync(&mon6513->timer);
	kfree(mon6513);
	return;
}

module_init(mon6513_module_init);
module_exit(mon6513_module_exit);

MODULE_LICENSE("Dual BSD/GPL");

