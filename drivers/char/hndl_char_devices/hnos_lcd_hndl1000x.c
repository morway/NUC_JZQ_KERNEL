/*
 * drivers/char/hndl_char_devices/hnos_lcd_hndl1000x.c
 *
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */
#include "hnos_generic.h"
#include "hnos_lcd_cog.h"
#include "hnos_hndl1000x.h"
#include "hnos_iomem.h"   

struct iomem_object       *ioaddr_reset;
struct uc1698fb_callbacks *lcd1000x_cbs;

static void lcd1000x_bus_init(void)
{
    ioaddr_reset->write_bit(ioaddr_reset, BLCD_NREST, SIG_HIGH);
    return;
}

static void lcd1000x_backlight(unsigned char action)
{
    struct iomem_object *iomem = ioaddr_reset;
        
    if (UC1698FB_LCD_BACKLIGHT_ON == action) {
        iomem->write_bit(iomem, BLCD_BL_EN, SIG_HIGH);
    } else if (UC1698FB_LCD_BACKLIGHT_OFF == action) {
        iomem->write_bit(iomem, BLCD_BL_EN, SIG_LOW);
    }

    return;
}

void __exit lcd1000x_module_cleanup(void)
{
    uc1698fb_callbacks_unregister(lcd1000x_cbs);
    iomem_object_put(ioaddr_reset);
    kfree(lcd1000x_cbs);
    lcd1000x_cbs = NULL;

    return;
}

static int __init  lcd1000x_module_init(void)
{
    ioaddr_reset = iomem_object_get(GPRS_LCD_BEEP_CS, 0);
    if (!ioaddr_reset) {
        printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
                (unsigned int) GPRS_LCD_BEEP_CS);
        return -ENOMEM;
    }

    lcd1000x_cbs = kmalloc(sizeof(struct uc1698fb_callbacks), GFP_KERNEL);
    if (!lcd1000x_cbs) {
        printk(KERN_ERR "%s: can't malloc lcd1000x_cbs.\n", __FUNCTION__);
        return -ENOMEM;
    }

    memset(lcd1000x_cbs, 0, sizeof(struct uc1698fb_callbacks));

    lcd1000x_cbs->init = lcd1000x_bus_init;
    lcd1000x_cbs->reset = NULL;
    lcd1000x_cbs->backlight = lcd1000x_backlight;

    uc1698fb_callbacks_register(lcd1000x_cbs);
    return 0;
}

module_init(lcd1000x_module_init);
module_exit(lcd1000x_module_cleanup);

MODULE_LICENSE("Dual BSD/GPL");

