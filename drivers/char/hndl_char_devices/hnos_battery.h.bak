/*
 * /drivers/char/hndl_char_devices/hnos_battery.h 
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

#ifndef __HNOS_BATTERY_H
#define __HNOS_BATTERY_H

#define BATTERY_DEVICE_NAME     "battery"

struct at91_battery_object
{
    unsigned charge_pin;
    unsigned supply_pin;
    unsigned power_type_pin;

    int (*get_battery_value)(u32* bat_val);

    u8  charge_state;
    u8  supply_state;
};

struct at91_battery_cdev 
{
    unsigned long is_open;      
    struct class *battery_class;
    struct cdev battery_cdev;           
    struct semaphore battery_lock;
    struct at91_battery_object *battery;
};

int battery_module_register(struct at91_battery_object *battery);
int battery_module_unregister(struct at91_battery_object *battery);

#endif
