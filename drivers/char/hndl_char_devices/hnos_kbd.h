/*
 * drivers/char/hndl_char_devices/hnos_kbd.h   
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
#ifndef	HNOS_KBD_H
#define	HNOS_KBD_H

#include "hnos_lcd_cog.h"
typedef int (*blt_func_t)(unsigned int blt);

void kb_key_insert(unsigned char key);
void kb_custom_key_insert(void *key, size_t size);
void kb_lastkey_time_update(void);

#define LCD_BLT_ON				1
#define LCD_BLT_OFF				0

int lcd_blt_register(blt_func_t blt_routine);
int lcd_blt_unregister(blt_func_t blt_routine);

#endif
