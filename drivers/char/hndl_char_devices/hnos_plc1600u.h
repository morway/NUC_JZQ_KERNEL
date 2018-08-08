/*
 * /drivers/char/hndl_char_devices/hnos_plc.h 
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

#ifndef __HNOS_PLC_H
#define __HNOS_PLC_H

#define PLC_DEVICE_NAME     "plc"

struct at91_plc_object
{
    unsigned reset_pin;
    unsigned power_pin;
    unsigned plc_a_pin;
    unsigned plc_b_pin;
    unsigned plc_c_pin;

    u8  power_on_state;
};

struct at91_plc_cdev 
{
    unsigned long is_open;      
    struct class *plc_class;
    struct cdev plc_cdev;           
    struct semaphore plc_lock;
    struct at91_plc_object *plc;
};

int plc_module_register(struct at91_plc_object *plc);
int plc_module_unregister(struct at91_plc_object *plc);

#endif
