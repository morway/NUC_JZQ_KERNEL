/*
 * drivers/char/hndl_char_devices/hnos_dc_intf.h   
 *
 *        zrm,eter_zrm@163.com
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
#ifndef		HNOS_DC_INTF_H
#define		HNOS_DC_INTF_H

#define     NR_DC_READ          3
#define     NR_DC_CHAN          2

struct adc_ops
{
    int (*read) (u8 ch, u32 *reslt);
};

struct  dc_device
{
    unsigned long is_open;
    struct class *class;
    struct cdev cdev;
    struct semaphore lock;
    struct adc_ops *adc;
};

int dc_adc_register(struct adc_ops *adc);
int dc_adc_unregister(struct adc_ops *adc);

#endif
