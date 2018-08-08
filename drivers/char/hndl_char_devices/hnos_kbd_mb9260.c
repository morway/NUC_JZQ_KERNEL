/*
 * drivers/char/hndl_char_devices/hnos_kdb_mb9260.c
 *  drivers for the keyboard on mb9260.
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
#include "hnos_iomem.h"

#define KEY_SCAN_TIMEOUT		5				/* default 50ms. */
#define KEYBOARD_PHY_BASE0		0x30000040      /* key1 ~ key16 */
#define KEYBOARD_PHY_BASE1		0x30000060      /* key17 ~ key32 */
#define KBDSCAN_STABLE_COUNT 2

enum KEY_INPUT
{
	KEY_INPUT_INVALID = 0xffffffff,
};

struct kbd_mb9260_dev
{
	unsigned int keycode[256];
	struct iomem_object *iomem0;		  
	struct iomem_object *iomem1;		  
	unsigned long key_ever_pressed;
	unsigned long kbdscan_state;
	unsigned int kbdscan_count[32];  
	struct timer_list timer;
	unsigned long timeout;
	u32 key_last;
};

struct kbd_mb9260_dev *kbd_mb9260;

enum KEY_IDX {	
	IDX_POWER = 24 , /* 开关机 */
	IDX_DEL = 22 ,		
	IDX_F12 = 21 ,  /*  通信 */
	IDX_UP = 20,
	IDX_FUNC = 19,
	IDX_ESC = 18,   /* 退出 */
	IDX_KPENTER	= 17 ,
	IDX_LEFT = 16,
	IDX_ENTER = 15,
	IDX_RIGHT = 14,
	IDX_FN = 13,
	IDX_DOWN = 12,
	IDX_STAR = 11,      /* * */
	IDX_3DEF = 10,       /* 3/D/E/F */
	IDX_2ABC = 9,       /* 2/A/B/C */
	IDX_1 = 8,          /* 1 */
	IDX_0 = 7,          /* 0 */
	IDX_POUND = 6,      /* # */
	IDX_6MNO = 5,       /* 6/M/N/O */
	IDX_9WXYZ = 4,      /* 9/W/X/Y/Z */
	IDX_5JKL = 3,       /* 5/J/K/L */
	IDX_8TUV = 2,       /* 8/T/U/V */
	IDX_4GHI = 1,       /* 8/G/H/I */
	IDX_7PQRS = 0,      /* 7/P/Q/R/S */

};


static inline u32 kbd_mb9260_getkey(struct kbd_mb9260_dev *dev)
{
	u16 key0 = 0xffff, key1 = 0xffff;
	struct iomem_object *iomem0 = dev->iomem0;		  
	struct iomem_object *iomem1 = dev->iomem1;	

	iomem0->read_word(iomem0, &key0, IO_RDONLY);
	iomem1->read_word(iomem1, &key1, IO_RDONLY);
	key1 |= 0xfe00; 

	return (key0 | (key1 << 16));
}

static inline void kbd_mb9260_build_event(struct input_event *ev, 
				unsigned int keycode, int pressed)
{
	struct timeval curtime;
	
	do_gettimeofday(&curtime);
	ev->time = curtime;
	ev->type = EV_KEY;
	ev->code = keycode;
	ev->value= pressed;
	return;
}

static void kbd_mb9260_report_key(struct kbd_mb9260_dev *mb9260kbd, 
	unsigned long key_stat, unsigned int idx)
{
	unsigned int pressed;
	struct input_event ev;
	
	if (unlikely(idx >= 256)) {
		return;
	}

	pressed = !test_bit(idx, &key_stat);
	printk("%s: key_stat %8x, key idx %d, keycode %d, %s.\n",
			__FUNCTION__, key_stat, idx, mb9260kbd->keycode[idx],
			(!pressed)?"released":"pressed.");
	
	kbd_mb9260_build_event(&ev, mb9260kbd->keycode[idx], pressed);
	kb_custom_key_insert((char *)&ev, sizeof(ev));

	return;
}

static void  __devinit kbd_mb9260_keycode_init(unsigned int *keycodes)
{
	keycodes[IDX_FUNC] = KEY_MENU;
	keycodes[IDX_F12] = KEY_F12;
	keycodes[IDX_UP] = KEY_UP;
	keycodes[IDX_FN] = KEY_LEFTSHIFT;
	keycodes[IDX_ENTER] = KEY_ENTER;
	keycodes[IDX_DEL] = KEY_BACKSPACE;
	keycodes[IDX_LEFT] = KEY_LEFT;

	/* FIX ME! */
	keycodes[IDX_KPENTER] = KEY_KPENTER;

	keycodes[IDX_RIGHT] = KEY_RIGHT;
	keycodes[IDX_ESC] = KEY_ESC;
	keycodes[IDX_DOWN] = KEY_DOWN;

	/* comobo key */
	keycodes[IDX_STAR] = KEY_KP8;

	keycodes[IDX_3DEF] = KEY_3;
	keycodes[IDX_2ABC] = KEY_2;
	keycodes[IDX_1] = KEY_1;
	
	keycodes[IDX_0] = KEY_0;

	/* comobo key */
	keycodes[IDX_POUND] = KEY_KP3;

	keycodes[IDX_6MNO] = KEY_6;
	keycodes[IDX_9WXYZ] = KEY_9;
	keycodes[IDX_5JKL] = KEY_5;
	keycodes[IDX_8TUV] = KEY_8;
	keycodes[IDX_4GHI] = KEY_4;
	keycodes[IDX_7PQRS] = KEY_7;
	keycodes[IDX_POWER] = KEY_POWER;
	return ;
}


static void kbd_mb9260_scan(unsigned long data)
{	
	struct kbd_mb9260_dev *mb9260kbd = (struct kbd_mb9260_dev *)data;
	unsigned long key_stat = 0xffffffff;
	unsigned long idx = 0;

	key_stat = kbd_mb9260_getkey(mb9260kbd);
	dprintk("%s: get key %08x.\n", __FUNCTION__, key_stat);
	if ((mb9260kbd->key_ever_pressed == 0) 
		&& (key_stat == 0xffffffff)
		&& (mb9260kbd->kbdscan_state == key_stat)) {
			dprintk("%s: no key pressed.\n", __FUNCTION__);
			goto out;
	}

	for (idx = 0; idx <= 31; idx++) {
		if (test_bit(idx, &key_stat) 
				^ test_bit(idx, &mb9260kbd->kbdscan_state)) {

			dprintk("%s: key %d pressed(released?), scan_code %08x, lastscan_code %08x\n", __FUNCTION__, idx, 
						 key_stat, mb9260kbd->kbdscan_state);
			set_bit(idx, &mb9260kbd->key_ever_pressed);
			mb9260kbd->kbdscan_count[idx] = 0;
			mb9260kbd->kbdscan_state = key_stat;
		} else if (++mb9260kbd->kbdscan_count[idx] >= KBDSCAN_STABLE_COUNT) {
			dprintk("%s: key %d pressed (released) stable cnt reached, scan_code %08x, lastscan_code %08x\n",
						__FUNCTION__, idx, 
						 key_stat, mb9260kbd->kbdscan_state);
			if (test_and_clear_bit(idx, &mb9260kbd->key_ever_pressed)) {
				dprintk("%s: key %d pressed (released) stable, scan_code %08x, lastscan_code %08x\n",
						__FUNCTION__, idx, 
						 key_stat, mb9260kbd->kbdscan_state);
				kbd_mb9260_report_key(mb9260kbd, key_stat, idx);
			}

			mb9260kbd->kbdscan_count[idx] = 0;
		}
	}

out:
	/* resubmit the timer. */
	mb9260kbd ->timer.expires = jiffies + mb9260kbd->timeout;
	add_timer(&mb9260kbd ->timer);
	
	return;
}

void kbd_mb9260_cleanup(void)
{
	if (kbd_mb9260){
		del_timer_sync(&kbd_mb9260->timer);

		if (kbd_mb9260->iomem0) {
			iomem_object_put(kbd_mb9260->iomem0);
			kbd_mb9260->iomem0 = NULL;
		}

		if (kbd_mb9260->iomem1) {
			iomem_object_put(kbd_mb9260->iomem1);
			kbd_mb9260->iomem1 = NULL;
		}

		kfree(kbd_mb9260);
		kbd_mb9260 = NULL;
	}

	return;
}

static int __init  kbd_mb9260_init(void)
{
	int result = 0;

	kbd_mb9260 = kmalloc(sizeof(struct kbd_mb9260_dev), GFP_KERNEL);
	if (!kbd_mb9260) {
		result = -ENOMEM;
		printk(KERN_ERR "Can NOT malloc memory for kbd_mb9260.\n");
		goto fail;  /* Make this more graceful */
	}
	memset(kbd_mb9260, 0, sizeof(struct kbd_mb9260_dev));

	kbd_mb9260->timeout = KEY_SCAN_TIMEOUT;
	kbd_mb9260->kbdscan_state = 0xffffffff;

	kbd_mb9260->iomem0 = iomem_object_get(KEYBOARD_PHY_BASE0, 0xffffffff);
	if (!kbd_mb9260->iomem0) {
		printk(KERN_ERR "Can NOT remap address 0x%08x\n", KEYBOARD_PHY_BASE0);
		result = -1;
		goto fail;
	}

	kbd_mb9260->iomem1 = iomem_object_get(KEYBOARD_PHY_BASE1, 0xffffffff);
	if (!kbd_mb9260->iomem1) {
		printk(KERN_ERR "Can NOT remap address 0x%08x\n", KEYBOARD_PHY_BASE1);
		result = -1;
		goto fail;
	}
	kbd_mb9260_keycode_init(kbd_mb9260->keycode);

	init_timer(&kbd_mb9260->timer);
	kbd_mb9260->timer.function = kbd_mb9260_scan;
	kbd_mb9260->timer.data = (unsigned long)kbd_mb9260;
	kbd_mb9260->timer.expires = jiffies + kbd_mb9260->timeout;
	add_timer(&kbd_mb9260->timer);

	HNOS_DEBUG_INFO("Keyboard for Mobile9260 registered.\n");
	return 0;

fail:
	kbd_mb9260_cleanup();
	return result;
}


module_init(kbd_mb9260_init);
module_exit(kbd_mb9260_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

