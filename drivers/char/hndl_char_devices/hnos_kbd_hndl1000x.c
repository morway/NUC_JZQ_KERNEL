/*
 * drivers/char/hndl_char_devices/hnos_kdb_hndl1000x.c
 *
 * Copyright (C) 2008 .
 * Author ZhangRM, peter_zrm@163.com
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
#include "hnos_iomem.h"                /* iomem object  */
#include "hnos_hndl1000x.h"

#define KEY_SCAN_TIMEOUT        5        /* default 50ms. */
#define	UC1611S_SMC_CHIP	2

enum KEY_INPUT
{
    KEY_INPUT_INVALID = 0x3f,
    KEY_INPUT_ESC = 0x3e,
    KEY_INPUT_ENTER = 0x3d,
    KEY_INPUT_UP = 0x3b,
    KEY_INPUT_DOWN = 0x37,
    KEY_INPUT_LEFT = 0x2f,
    KEY_INPUT_RIGHT = 0x1f,    
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

struct kbd_hndl1000x_dev
{
    struct iomem_object *iomem;
    unsigned char key_i_last;
};
struct kbd_hndl1000x_dev *kbd_hndl1000x =NULL;

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
        case KEY_INPUT_LEFT:
            key_map = KEY_LEFT;
            break;
        case KEY_INPUT_RIGHT:
            key_map = KEY_RIGHT;
            break;            
        case KEY_INPUT_ENTER:
            key_map = KEY_ENTER;
            break;
        case KEY_INPUT_ESC:
            key_map = KEY_CANCEL;
            break;
        default:
            break;
    }

    return key_map;
}


void kbd_hndl1000x_scan(void)
{    
    char key_v = KEY_NULL;
    unsigned char key_i = KEY_INPUT_INVALID;
    int ret =0;
    struct kbd_hndl1000x_dev *dev = kbd_hndl1000x; 

    if(!dev)
        return;
        
    ret = dev->iomem->read_byte(dev->iomem, &key_i, IO_RDONLY);
    
    key_i &= 0x3f;

    if (key_i == dev->key_i_last) {
        goto out;
    } else {
        key_v = kbd_key_remap(key_i);
        kb_key_insert(key_v);
        dev->key_i_last = key_i;
    }

out:
    
    return;
}
EXPORT_SYMBOL(kbd_hndl1000x_scan);

void kbd_hndl1000x_cleanup(void)
{
    if (kbd_hndl1000x){
        if (kbd_hndl1000x->iomem) {
            iomem_object_put(kbd_hndl1000x->iomem);
        }
        kfree(kbd_hndl1000x);
        kbd_hndl1000x = NULL;
    }

    return;
}

static int __init  kbd_hndl1000x_init(void)
{
    int result=0;

    kbd_hndl1000x = kmalloc(sizeof(struct kbd_hndl1000x_dev), GFP_KERNEL);
    if (!kbd_hndl1000x) {
        result = -ENOMEM;
        goto fail;  /* Make this more graceful */
    }
    memset(kbd_hndl1000x, 0, sizeof(struct kbd_hndl1000x_dev));

    kbd_hndl1000x->key_i_last = KEY_INPUT_INVALID;

    kbd_hndl1000x->iomem = iomem_object_get(KEY_CS, 0);
    if (!kbd_hndl1000x->iomem) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) KEY_CS);
                result = -1;
		goto iomem_fail;
		
	}
	
    printk("<HNOS> Keyboard for Wfet1000x registered.\n");
    return 0;
    
    iomem_object_put(kbd_hndl1000x->iomem);
iomem_fail:
    kfree(kbd_hndl1000x);
fail:    
    return result;
}


module_init(kbd_hndl1000x_init);
module_exit(kbd_hndl1000x_cleanup);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

