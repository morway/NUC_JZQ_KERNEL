/*
 * drivers/char/hndl_char_devices/hnos_kdb_hntt1800x.c
 *  drivers for the keyboard on hntt1800x.
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
#include "hnos_kbd.h"
#include "hnos_prog_interface.h"
#include "hnos_debug.h"

#define KEY_SCAN_TIMEOUT		5		/* default 50ms. */
//#define KEY_SCAN_TIMEOUT		100		/* default 1s. */
#define KEYBOARD_PHY_BASE		0x30000040

int nuc970_gpio_core_get1( unsigned gpio_num);
int nuc970_gpio_core_direction_in1(unsigned gpio_num);


struct prog_btn_callback cb_hntt1800x;

enum KEY_INPUT
{
	KEY_INPUT_INVALID = 0x7f,
	//KEY_INPUT_ENTER = ((~(1 << 6)) & 0x7f), //0011 1111 =0x3f
	KEY_INPUT_ENTER = ((~(1 << 4)) & 0x7f), //0011 1111 =0x6f
	KEY_INPUT_UP = ((~(1 << 5)) & 0x7f),    //0101 1111 =0x5f
	//KEY_INPUT_ESC = ((~(1 << 4)) & 0x7f),   //0110 1111 =0x6f
	KEY_INPUT_ESC = ((~(1 << 6)) & 0x7f),   //0110 1111 =0x3f
	KEY_INPUT_DOWN = ((~(1 << 3)) & 0x7f),  //0111 0111 =0x77
	KEY_INPUT_LEFT = ((~(1 << 2)) & 0x7f),  //0111 1011 = 0x7b
	KEY_INPUT_RIGHT = ((~(1 << 1)) & 0x7f), //0111 1101 = 0x7d
	KEY_INPUT_SET = ((~(1 << 0)) & 0x7f),   //0111 1110 = 0x7e
};

enum KEY_MAP
{
        eKEY_UP = 8,  
	eKEY_DOWN = 2,
	eKEY_CANCEL = 1,  
	eKEY_ENTER = 4,
	eKEY_LEFT = 5,   
	eKEY_RIGHT = 7,
	eKEY_NULL = 0,
};

struct kbd_hntt1800x_dev
{
//	void __iomem *iomem;
	unsigned int iomem;
	struct timer_list timer;
	unsigned long timeout;
	unsigned char key_i_last;
};

struct kbd_hntt1800x_dev *kbd_hntt1800x;

int kbd_prog_btn_pressed(void) 
{
	unsigned char key_i = KEY_INPUT_INVALID;

//	key_i = ((readb(kbd_hntt1800x->iomem) & 0x7f)|0x01);
	return ( ( KEY_INPUT_SET == key_i ) ? 1 : 0 );
}

static unsigned char kbd_key_remap(unsigned char key_rd)
{
	unsigned char  key_map = eKEY_NULL;
	switch (key_rd) {
		case KEY_INPUT_UP:
			key_map = eKEY_UP;
			break;

		case KEY_INPUT_DOWN:
			key_map = eKEY_DOWN;
			break;

		case KEY_INPUT_ENTER:
			key_map = eKEY_ENTER;
			break;

		case KEY_INPUT_LEFT:
			key_map = eKEY_LEFT;
			break;
			
		case KEY_INPUT_RIGHT:
			key_map = eKEY_RIGHT;
			break;

		case KEY_INPUT_ESC:
			key_map = eKEY_CANCEL;
			break;

		default:
			break;
	}

	return key_map;
}

static int pin_shift(unsigned int pin_number)
{
	int shift;
	
	if(pin_number==228)
		shift=6;
	if(pin_number==229)
		shift=5;
	if(pin_number==230)
		shift=4;
	if(pin_number==231)
		shift=3;
	if(pin_number==232)
		shift=2;
	if(pin_number==233)
		shift=1;

	return shift;
}

static void init_key_pin(void)
{
	nuc970_gpio_core_direction_in1(228);
	nuc970_gpio_core_direction_in1(229);
	nuc970_gpio_core_direction_in1(230);
	nuc970_gpio_core_direction_in1(231);
	nuc970_gpio_core_direction_in1(232);
	nuc970_gpio_core_direction_in1(233);
	
}


static void kbd_hntt1800x_test(unsigned long data)
{	
	unsigned int i;
	char key_v = eKEY_NULL;
	unsigned char key_i = KEY_INPUT_INVALID;
	struct kbd_hntt1800x_dev *dev = (struct kbd_hntt1800x_dev *)data; 
    unsigned int t;
	key_i=0;
	
	for(i=228;i<=233;i++){
		//	key_i |= ((~(nuc970_gpio_core_get1(i)<<pin_shift(i)))&0x7f);
		t=0;
		t=nuc970_gpio_core_get1(i)<<pin_shift(i);
		key_i |= ((t & 0x7f)|0x01);

	}

	if (key_i == dev->key_i_last) {
	//	printk("fail:0x%08x\n",key_i);
		goto out;
		
	} else if(key_i != KEY_INPUT_INVALID) {
		dprintk("%s: key_i = %2x\n", __FUNCTION__, key_i);
	//	printk("key:0x%08x\n",key_i);
	//	if (key_i == KEY_INPUT_SET) {
	//		prog_btn_event();
	//	}

		key_v = kbd_key_remap(key_i);
	//	printk("key_v:%d\n",key_v);
		kb_key_insert(key_v);
	}

	dev->key_i_last = key_i;

out:
	/* resubmit the timer. */
	dev->timer.expires = jiffies + dev->timeout;
	add_timer(&dev->timer);
	
	return;




}




static void kbd_hntt1800x_scan(unsigned long data)
{	
	char key_v = eKEY_NULL;
	unsigned char key_i = KEY_INPUT_INVALID;
	struct kbd_hntt1800x_dev *dev = (struct kbd_hntt1800x_dev *)data; 

	key_i = ((readb(dev->iomem) & 0x7f)|0x01);
	if (key_i == dev->key_i_last) {
		goto out;
	} else if(key_i != KEY_INPUT_INVALID) {
		dprintk("%s: key_i = %2x\n", __FUNCTION__, key_i);
		if (key_i == KEY_INPUT_SET) {
			prog_btn_event();
		}

		key_v = kbd_key_remap(key_i);
		kb_key_insert(key_v);
	}

	dev->key_i_last = key_i;
out:
	/* resubmit the timer. */
	dev->timer.expires = jiffies + dev->timeout;
	add_timer(&dev->timer);
	
	return;
}

void kbd_hntt1800x_cleanup(void)
{
	if (kbd_hntt1800x){
		del_timer_sync(&kbd_hntt1800x->timer);

		if (kbd_hntt1800x->iomem) {
			//iounmap(kbd_hntt1800x->iomem);
			//kbd_hntt1800x->iomem = NULL;
			kbd_hntt1800x->iomem = 0;
		}

		kfree(kbd_hntt1800x);
		kbd_hntt1800x = NULL;
	}

	HNOS_DEBUG_INFO("Keyboard for HNTT1800X unregistered.\n");
	return;
}

static int __init  kbd_hntt1800x_init(void)
{
	int result=0;

	kbd_hntt1800x = kmalloc(sizeof(struct kbd_hntt1800x_dev), GFP_KERNEL);
	if (!kbd_hntt1800x) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(kbd_hntt1800x, 0, sizeof(struct kbd_hntt1800x_dev));

	kbd_hntt1800x->timeout = KEY_SCAN_TIMEOUT;
	kbd_hntt1800x->key_i_last = KEY_INPUT_INVALID;

	//kbd_hntt1800x->iomem = ioremap(KEYBOARD_PHY_BASE, 1);
	kbd_hntt1800x->iomem =1;
	if (!kbd_hntt1800x->iomem) {
		printk(KERN_ERR "Can NOT remap address 0x%08x\n", KEYBOARD_PHY_BASE);
		result = -1;
		goto fail;
	}

	//cb_hntt1800x.btn_pressed = kbd_prog_btn_pressed;
	//prog_callback_register(&cb_hntt1800x);

	init_key_pin();

	init_timer(&kbd_hntt1800x->timer);
	//kbd_hntt1800x->timer.function = kbd_hntt1800x_scan;
	kbd_hntt1800x->timer.function = kbd_hntt1800x_test;
	kbd_hntt1800x->timer.data = (unsigned long)kbd_hntt1800x;
	kbd_hntt1800x->timer.expires = jiffies + kbd_hntt1800x->timeout;
	add_timer(&kbd_hntt1800x->timer);

	HNOS_DEBUG_INFO("Keyboard for HNTT1800X registered.\n");
	return 0;

fail:
	kbd_hntt1800x_cleanup();
	return result;
}


module_init(kbd_hntt1800x_init);
module_exit(kbd_hntt1800x_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

