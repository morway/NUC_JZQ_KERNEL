/*
 *  drivers/char/hndl_char_devices/hnos_gpio_bus.c
 *
 *  Gpio Bus Interface. 
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "hnos_generic.h"
#include "hnos_gpio_bus.h" 
#include "hnos_proc.h"

#define     GBUS_DEBUG   1

static spinlock_t  gbus_lock;
unsigned long gbus_slots[NR_GPIO_CS] = 
{
    GPIO_CS_KEY_PIN,
    GPIO_CS_ID_PIN,
    GPIO_CS_STATE_PIN,
};

#if defined (GBUS_DEBUG)

static struct proc_item gbus_items[] = 
{
	[0] = {
		.name = "gpio_cs_key", 
	},

	[1] = {
		.name = "gpio_cs_id", 
	},

	[2] = {
		.name = "gpio_cs_state", 
	},
};

static int gbus_paranoid_check(void)
{
    int i = 0;
    int collision = 0;

    for (i=0; i<NR_GPIO_CS; i++) {
        collision = !at91_get_gpio_value(gbus_slots[i]);
        if (collision) {
            printk(KERN_ERR "ERROR: GBUS collision detected, cs %d was enable.\n", i);
        }
    }

    return 0;
}

static int gbus_proc_read(struct proc_item *item, char *page)
{
	unsigned int len = 0;
    unsigned int tmp = 0;

    if (!item || !page || !item->name) {
        printk(KERN_ERR "%s: invalid params.\n", __FUNCTION__);
    }

    if (strcmp(item->name, "gpio_cs_key") == 0) {
        tmp = gbus_readb(GPIO_CS_KEY);
        len = sprintf(page, "gpio_cs_key :\t%02x\n", tmp);
        goto out;

    } else if (strcmp(item->name, "gpio_cs_id") == 0) {
        tmp = gbus_readb(GPIO_CS_ID);
        len = sprintf(page, "gpio_cs_id:\t%02x\n", tmp);
        goto out;

    } else if (strcmp(item->name, "gpio_cs_state") == 0) {
        tmp = gbus_readb(GPIO_CS_STATE);
        len = sprintf(page, "gpio_cs_state:\t%02x\n", tmp);
        goto out;
    } 
out:
	return len;
}


#else
static inline int gbus_paranoid_check(void) { return 0;}
#endif


/* caller must hold the lock. */
static inline int gbus_enable(unsigned int cs)
{
    if (unlikely(cs >= NR_GPIO_CS)) {
        printk(KERN_ERR "%s: invalid params, cs %d.\n", __FUNCTION__, cs);
        return -EINVAL;
    }

    gbus_paranoid_check();
    at91_set_gpio_value(gbus_slots[cs], CS_ENABLE);
    return 0;
}

/* caller must hold the lock. */
static inline int gbus_disable(unsigned int cs)
{
    if (unlikely(cs >= NR_GPIO_CS)) {
        printk(KERN_ERR "%s: invalid params, cs %d.\n", __FUNCTION__, cs);
        return -EINVAL;
    }

    at91_set_gpio_value(gbus_slots[cs], CS_DISABLE);
    return 0;
}

u8 gbus_readb(unsigned int cs)
{
    u32 pdsr = 0;
    unsigned long flags;
    void __iomem *pio = (void __iomem *) AT91_VA_BASE_SYS + AT91_PIOC;

    spin_lock_irqsave(&gbus_lock, flags);

    gbus_enable(cs);
    pdsr = __raw_readl(pio + PIO_PDSR);
    gbus_disable(cs);

    spin_unlock_irqrestore(&gbus_lock, flags);

    return ((pdsr >> 24) & 0xff);
}

static int __init gbus_module_init(void)
{
    unsigned int slot = 0;

    spin_lock_init(&gbus_lock);

    for (slot=0; slot<NR_GPIO_CS; slot++) {
        at91_set_gpio_output(gbus_slots[slot], CS_DISABLE);
    }

    for (slot=GBUS_DATA0; slot<=GBUS_DATA7; slot++) {
        at91_set_gpio_input(slot, 1);
    }

#if defined (GBUS_DEBUG)
    for (slot=0; slot<ARRAY_SIZE(gbus_items); slot++) {
        gbus_items[slot].read_func = gbus_proc_read;
    }
    hnos_proc_items_create(gbus_items, ARRAY_SIZE(gbus_items));
    printk("add a gpio bus proc interface.\n");
#endif

    HNOS_DEBUG_INFO("Register a gpio bus interface.\n");
    return 0;
}

static void __exit gbus_module_exit(void)
{
#if defined (GBUS_DEBUG)
    hnos_proc_items_remove(gbus_items, ARRAY_SIZE(gbus_items));
#endif

    HNOS_DEBUG_INFO("Remove the gpio bus interface.\n");
    return;
}

module_init(gbus_module_init);
module_exit(gbus_module_exit);

EXPORT_SYMBOL(gbus_readb);

MODULE_LICENSE("Dual BSD/GPL");

