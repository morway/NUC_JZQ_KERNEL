/*
 * /drivers/char/hndl_char_devices/hnos_pio.h 
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

#ifndef __HNOS_PIO_H
#define __HNOS_PIO_H

#define PIO_DEVICE_NAME     "pio"

struct at91_pio_object
{
    unsigned pio_bit0_pin;
    unsigned pio_bit1_pin;
    unsigned pio_bit2_pin;
    unsigned pio_bit3_pin;
    unsigned pio_bit4_pin;
    unsigned pio_bit5_pin;
    unsigned pio_bit6_pin;
    unsigned pio_bit7_pin;
};

struct at91_pio_cdev 
{
    unsigned long is_open;      
    struct class *pio_class;
    struct cdev pio_cdev;           
    struct semaphore pio_lock;
    struct at91_pio_object *pio;
};

int pio_module_register(struct at91_pio_object *pio);
int pio_module_unregister(struct at91_pio_object *pio);

#endif
