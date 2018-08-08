

#include <linux/fs.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/clk.h> 
#include <linux/list.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/clkdev.h>
#include <linux/device.h>
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>

#include <mach/map.h>
#include <mach/regs-gcr.h>
#include <mach/regs-clock.h>
#include <mach/regs-timer.h>
#include <mach/regs-gpio.h>
#include <mach/nuc970-etimer.h>
#include <mach/irqs.h>
#include <mach/mfp.h>
#include <mach/hardware.h>
#include <mach/regs-aic.h>

#include <asm/uaccess.h>
#include <asm/io.h> 
#include <asm/mach-types.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>

#include "hnos_generic.h"
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_gpio.h" 
#include "hnos_output.h"
#include "hnos_iomem.h"
#include "hnos_debug.h"



#define RESETINT	0x1f
#define PERIOD		(0x01 << 27)
#define ONESHOT		(0x00 << 27)
#define COUNTEN		(0x01 << 30)
#define INTEN		(0x01 << 29)

#define TMR2HZ		1000
#define PRESCALE	0x31 /* Divider = prescale + 1 */
#define	TDR_SHIFT	24
#define	TDR_MASK	((1 << TDR_SHIFT) - 1)



#define IR_HIGH {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t|0x01);} //pI0
#define IR_LOW {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t&~(0x01));} //pI0





struct device_pulse_input 
{
    struct cdev my_cdev;
};


#define DEBUG 0
#define MAXCH 2
#define DEV_NAME "pulse_input"
#define ENABLE_IRQ    1
static int minor_num = 0;
static int major_num = 0;
static  int  pulse_count[MAXCH]={0};

static struct class *my_class;
static struct proc_dir_entry *hndl_proc_dir = NULL;
static struct device_pulse_input *g_my_device;

static int my_open(struct inode *inode, struct file *filp);
static int my_release(struct inode *inode, struct file *filp);
static int pulse_proc_read(char *page);
static ssize_t hnos_proc_dispatch_read(struct file *file, char __user * page, size_t count,
		       loff_t * ppos);

int nuc970_gpio_core_get1( unsigned gpio_num);
int nuc970_gpio_core_direction_out1(unsigned gpio_num, int val);

static unsigned int timer2_load;
unsigned int  irq;
int pulse_is_open=0;
int pulse_is_first_read=0;





//-----------------------处理-----------------------------------------------------------



static irqreturn_t  timer1_interrupt(int irq, void *dev_id)
{ 
	
	__raw_writel(0x04, REG_TMR_TISR); /* clear TIF2 */

	static int flag=0;
//	at91_set_gpio_value(AT91_PIN_PB18, flag);

	if(flag == 0x01){
		IR_HIGH;
	}else{	
		IR_LOW;
		}

	flag=flag ^ 0x01;
	//printk("flag=%d\n",flag);

	return  IRQ_HANDLED;
}



static void  pulse_start(void) 
{   

	unsigned int val;
	unsigned int rate;
	struct clk *clk = clk_get(NULL, "timer2");

	BUG_ON(IS_ERR(clk));

	clk_prepare(clk);
	clk_enable(clk);

	__raw_writel(0x00, REG_TMR_TCSR2);				//CE 0

	rate = clk_get_rate(clk) / (PRESCALE + 1);
	printk("pulse_start...rate=%d\n",rate);

	timer2_load = (rate / (TMR2HZ*90));
	
	val = __raw_readl(REG_TMR_TCSR2);
	val &= ~(0x01 << 27);
	__raw_writel(timer2_load, REG_TMR_TICR2);		//TCMP
	val |= (PERIOD | COUNTEN | INTEN | PRESCALE);
	__raw_writel(val, REG_TMR_TCSR2);				//MODE;CE;IE; PRESCALE     


	//__raw_writel(RESETINT, REG_TMR_TISR);


}  			
			

//-----------------------处理结束------------------------------------------------------------
static int hnos_proc_dispatch_close(struct inode *inode, struct file *file)
{

	pulse_is_open = 0;

	__raw_writel(0x00, REG_TMR_TCSR2);

	return 0;
}


static int hnos_proc_dispatch_open(struct inode *inode, struct file *file)
{

	if (pulse_is_open)
		goto out_busy;

	pulse_is_open = 1;
	pulse_is_first_read=1;

	 pulse_start();

	return 0;

out_busy:
	return -EBUSY;
}


static ssize_t pulse_proc_read(char *page)
{
    int cnt = 0;

    cnt += sprintf(page + cnt, "%s\t%d\n", "pulse0:", pulse_count[0]);  
	cnt += sprintf(page + cnt, "%s\t%d\n", "pulse1:", pulse_count[1]);  
  //  printk( "%s\t%d\n", "pulse0:", pulse_count[0]);  
  //  printk( "%s\t%d\n", "pulse1:", pulse_count[1]);  
    return cnt;
}


static int my_open(struct inode *inode, struct file *filp)
{    
    struct device_pulse_input *my_device;
    my_device = container_of(inode->i_cdev, struct device_pulse_input, my_cdev);
    filp->private_data = my_device;

	return 0;
}

static int my_release(struct inode *inode, struct file *filp)
{
    return 0;
}


static ssize_t hnos_proc_dispatch_read(struct file *file, char __user * page, size_t count,
		       loff_t * ppos)
{
	static int len=0;

	if (pulse_is_first_read)
  	{
		pulse_is_first_read=0;

		len = pulse_proc_read(page);
		if (len < 0)
			return len;

		if (len < count) {
			count = len;
		}

		*ppos += count;
		
		return count;
  	}else
  	{
		return 0;
	}
	
}




static struct file_operations proc_file_ops = {

	.read = hnos_proc_dispatch_read,
  // .read = pulse_proc_read,   	
	//.write =hnos_proc_dispatch_write,
   //.write =pulse_proc_write,   
	.open= hnos_proc_dispatch_open,
	.release= hnos_proc_dispatch_close,

};

static struct file_operations my_fops = 
{
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
};


void __exit my_device_exit(void)
{
    dev_t dev_num = MKDEV(major_num, minor_num);   
//    pulse_stop();
   	if(g_my_device)
   	{
       	cdev_del(&(g_my_device->my_cdev));
       	kfree(g_my_device);
       	g_my_device = NULL;
    }
	
	//------------------------
    if(my_class)
	{
        //class_device_destroy(my_class, dev_num);
		device_destroy(my_class, dev_num);
        class_destroy(my_class);
    }
	
    remove_proc_entry(DEV_NAME, hndl_proc_dir);
    //  hnos_proc_rmdir();  
    // hnos_proc_items_remove(items_pulse, ARRAY_SIZE(items_pulse));


    unregister_chrdev_region(dev_num, 1); 
	printk("class_device_destroy...3\n");
	   
   	free_irq(IRQ_TMR2, NULL);
   
    HNOS_DEBUG_INFO("Pulse_device_exit: exit sucess!\n");
 return;
}

static int __init my_device_init(void)
{    
	int result = 0;
    dev_t dev_num = 0;
    int res;
    int err;
	struct proc_dir_entry *proc = NULL;
		 
     printk("my_device_init, alloc_chrdev_region...\n");
    res = alloc_chrdev_region(&dev_num, minor_num, 1, DEV_NAME);   
    major_num = MAJOR(dev_num);   
    if(res < 0)
    {
        printk("alloc_chrdev_region error!\n");
        return -1;
    }
  
    my_class = class_create(THIS_MODULE, DEV_NAME);
      
    if(IS_ERR(my_class)) 
	{
        printk("Err: failed in creating class.\n");
        return -1;
    }
   
    //class_device_create(my_class, NULL, dev_num, NULL, DEV_NAME);
    device_create(my_class, NULL, dev_num, NULL, DEV_NAME);
        
    g_my_device= kmalloc(sizeof(struct device_pulse_input), GFP_KERNEL);
    memset(g_my_device, 0, sizeof(struct device_pulse_input));
    cdev_init(&(g_my_device->my_cdev), &my_fops);
    g_my_device->my_cdev.owner = THIS_MODULE;
    g_my_device->my_cdev.ops = &my_fops;
    
    err = cdev_add(&(g_my_device->my_cdev), dev_num, 1);
     
    if(err != 0)
    {
    	printk("cdev_add error!\n");
        goto release_g_my_device;
    }
	

    printk("request_irq...\n");	
	if (request_irq(IRQ_TMR2, timer1_interrupt,IRQF_TIMER|IRQF_IRQPOLL|IRQF_DISABLED, DEV_NAME, NULL)) {
        pr_debug("register irq failed \n");
        result = -EAGAIN;		
        goto request_irq_err ;
    }

   
    printk("hnos_proc_items_create...\n");
 //   hnos_proc_items_create(items_pulse, ARRAY_SIZE(items_pulse));
	if (!hndl_proc_dir) {
       hndl_proc_dir = hnos_proc_mkdir();
    }

    if (!hndl_proc_dir) {
		result = -ENODEV;
		printk("%s: creat proc dir fail.\n", __FUNCTION__); 
		goto proc_entry_failed1;
    } else {
        proc = proc_create(DEV_NAME, S_IFREG | S_IRUGO | S_IWUSR,  hndl_proc_dir, &proc_file_ops);
     	if (proc) {
			printk("%s: creat proc success.\n", __FUNCTION__);
        } else {
            result = -1;
            printk("%s: creat proc fail.\n", __FUNCTION__);
			goto proc_entry_failed;
        }
	}

	nuc970_gpio_core_direction_out1(256,0);

   


    HNOS_DEBUG_INFO("Initialized device %s, major %d.\n", DEV_NAME, major_num);
    return 0; 

proc_entry_failed1:
    remove_proc_entry(DEV_NAME, hndl_proc_dir);

proc_entry_failed:
    hnos_proc_rmdir();

request_irq_err:
   	cdev_del(&(g_my_device->my_cdev));
	
release_g_my_device: 	           
   	kfree(g_my_device);
   	g_my_device = NULL;
    
    return result;
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("Dual BSD/GPL");

