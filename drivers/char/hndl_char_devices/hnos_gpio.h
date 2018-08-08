/*
 * drivers/char/hndl_char_devices/hnos_gpio.h   
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
#ifndef		HNOS_GPIO_H
#define		HNOS_GPIO_H

#define GPIO_OUTPUT_MASK				(1 << 0)
#define GPIO_PULLUP					    (1 << 1)
#define GPIO_OUTPUT_HIGH				(1 << 2)
#define GPIO_OUTPUT_LOW				    (0 << 2)

int hnos_gpio_parse(char *buf, size_t length);
void hnos_gpio_cfg(unsigned pin, u8 settings);
int  hnos_proc_gpio_set(struct proc_item *item, const char __user * userbuf, unsigned long count);
int  hnos_proc_gpio_get(struct proc_item *item, char *page);
int  hnos_proc_gpio_get_reverse(struct proc_item *item, char *page);
int  hnos_proc_gpio_set_reverse(struct proc_item *item, const char __user * userbuf, unsigned long count); 

#endif
