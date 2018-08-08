/*
 * drivers/char/hndl_char_devices/hnos_kdb_metergather.c
 *  drivers for the keyboard on Meter Gather such as HNDL-ND2000.
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

#define KEY_SCAN_TIMEOUT		5		/* default 50ms. */
#define KEYBOARD_PHY_BASE		0x30000040

enum KEY_INPUT
{
	KEY_INPUT_INVALID = 0xf,
	KEY_INPUT_CANCEL = 0xe,
	KEY_INPUT_UP = 0xd,
	KEY_INPUT_ENTER = 0xb,
	KEY_INPUT_DOWN = 0x7,
};

enum KEY_MAP
{
	KEY_UP = 8,  
	KEY_DOWN = 2,
	KEY_CANCEL = 4,  
	KEY_ENTER = 1,
	KEY_LEFT = 5,   
	KEY_RIGHT = 7,
	KEY_NULL = 0,
};

struct kbd_metergather_dev
{
	void __iomem *iomem;		  
	struct timer_list timer;
	unsigned long timeout;
	unsigned char key_i_last;
};

struct kbd_metergather_dev *kbd_metergather;

static unsigned char kbd_key_remap(unsigned char key_rd)
{
	unsigned char  key_map = KEY_NULL;
	switch (key_rd) {
		case KEY_INPUT_UP:
			key_map = KEY_UP;
			break;
		case KEY_INPUT_DOWN:
			key_map = KEY_DOWN;
			break;
		case KEY_INPUT_ENTER:
			key_map = KEY_ENTER;
			break;
		case KEY_INPUT_CANCEL:
			key_map = KEY_CANCEL;
			break;
		default:
			break;
	}

	return key_map;
}


static void kbd_metergather_scan(unsigned long data)
{	
	char key_v = KEY_NULL;
	unsigned char key_i = KEY_INPUT_INVALID;
	struct kbd_metergather_dev *dev = (struct kbd_metergather_dev *)data; 

	key_i = readb(dev->iomem);
	key_i = (key_i >> 1) & 0xf;

	if (key_i == dev->key_i_last) {
		goto out;
	} else if (key_i != KEY_INPUT_INVALID) {
		dprintk("key_i = %x\n", key_i);
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

void kbd_metergather_cleanup(void)
{
	if (kbd_metergather){
		del_timer_sync(&kbd_metergather->timer);

		if (kbd_metergather->iomem) {
			iounmap(kbd_metergather->iomem);
			kbd_metergather->iomem = NULL;
		}

		kfree(kbd_metergather);
		kbd_metergather = NULL;
	}

	return;
}

static int __init  kbd_metergather_init(void)
{
	int result=0;

	kbd_metergather = kmalloc(sizeof(struct kbd_metergather_dev), GFP_KERNEL);
	if (!kbd_metergather) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(kbd_metergather, 0, sizeof(struct kbd_metergather_dev));

	kbd_metergather->timeout = KEY_SCAN_TIMEOUT;
	kbd_metergather->key_i_last = KEY_INPUT_INVALID;

	kbd_metergather->iomem = ioremap(KEYBOARD_PHY_BASE, 1);
	if (!kbd_metergather->iomem) {
		printk(KERN_ERR "Can NOT remap address 0x%08x\n", KEYBOARD_PHY_BASE);
		result = -1;
		goto fail;
	}

	init_timer(&kbd_metergather->timer);
	kbd_metergather->timer.function = kbd_metergather_scan;
	kbd_metergather->timer.data = (unsigned long)kbd_metergather;
	kbd_metergather->timer.expires = jiffies + kbd_metergather->timeout;
	add_timer(&kbd_metergather->timer);

	HNOS_DEBUG_INFO("Keyboard for Meter Gather (such as HNDL-ND2000 and etc.) registered.\n");
	return 0;

fail:
	kbd_metergather_cleanup();
	return result;
}


module_init(kbd_metergather_init);
module_exit(kbd_metergather_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

