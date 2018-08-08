/*
 *  drivers/char/hndl_char_devices/hnos_gpio_intf.c
 *
 *  Gpio interface. 
 *
 *         zrm,eter_zrm@163.com
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
#include "hnos_proc.h" 
#include "hnos_gpio.h" 

int hnos_gpio_parse(char *buf, size_t length)
{
	int i = 0;
	unsigned char gpio_buf[5] = {0};
	unsigned int off = 0;
	char bank = 'A';
	unsigned int pin = AT91_PIN_PA0;

	char *s = strchr(buf, 'P');
	if (NULL == s) {
		return -1;
	}

	bank = *(s + 1);

	s = s + 2;
	while (!(isspace(*s)) && *s != '\0') {
		if (i >= sizeof(gpio_buf) || i >= length) {
			break;
		}
		gpio_buf[i] = *s;
		i++;
		s++;
	}
	off = simple_strtoull(gpio_buf, NULL, 0);
	if (off > 31) {
		off = 31;
	}

	switch (bank) {
		case 'A':
			pin = AT91_PIN_PA0 + off;
			break;
		case 'B':
			pin = AT91_PIN_PB0 + off;
			break;
		case 'C':
			pin = AT91_PIN_PC0 + off;
			break;
		default:
			break;
	}

    dprintk("%s: gpio_desc %s, bank %c, off %d, pin %d.\n",
            __FUNCTION__, buf, bank, off, pin);

	return pin;
} 
EXPORT_SYMBOL(hnos_gpio_parse);

void hnos_gpio_cfg(unsigned pin, u8 settings)
{
    if (!pin) {
        return;
    }

    if (settings & GPIO_OUTPUT_MASK) { /* GPIO: output */
        if (settings & GPIO_OUTPUT_HIGH) {
            at91_set_gpio_output(pin, 1);
        } else {
            at91_set_gpio_output(pin, 0);
        }
    } else {  /* GPIO: input */
        if (settings & GPIO_PULLUP) {
            at91_set_gpio_input(pin, 1);
        } else {
            at91_set_gpio_input(pin, 0);
        }
    }

    return ;
}
EXPORT_SYMBOL(hnos_gpio_cfg);

int hnos_proc_gpio_get(struct proc_item *item, char *page)
{
    int value = 0;
    int len = 0;

    unsigned pin = item->pin;
    value = at91_get_gpio_value(pin);
    len = sprintf(page, "%d\n", value);
    return len;
}
EXPORT_SYMBOL(hnos_proc_gpio_get);

int hnos_proc_gpio_get_reverse(struct proc_item *item, char *page)
{
    int value = 0;
    int len = 0;

    unsigned pin = item->pin;
    value = !(at91_get_gpio_value(pin));
    len = sprintf(page, "%d\n", value);
    return len;
}
EXPORT_SYMBOL(hnos_proc_gpio_get_reverse);

int hnos_proc_gpio_set_reverse(struct proc_item *item, const char __user * userbuf,
        unsigned long count) 
{
    unsigned int value = 0;
    char val[12] = {0};

    unsigned pin = item->pin;

    if (count >= 11){
        return -EINVAL;
    }

    if (copy_from_user(val, userbuf, count)){
        return -EFAULT;
    }

    value = (unsigned int)simple_strtoull(val, NULL, 0);

    dprintk(KERN_INFO "%s:val=%s,after strtoull,value=0x%08x\n",
            __FUNCTION__, val, value);

    if (value != 0){ 
        at91_set_gpio_value(pin, 0);
    } else {        
        at91_set_gpio_value(pin, 1);
    }

    return 0;
}
EXPORT_SYMBOL(hnos_proc_gpio_set_reverse);


int hnos_proc_gpio_set(struct proc_item *item, const char __user * userbuf,
        unsigned long count) 
{
    unsigned int value = 0;
    char val[12] = {0};

    unsigned pin = item->pin;

    if (count >= 11){
        return -EINVAL;
    }

    if (copy_from_user(val, userbuf, count)){
        return -EFAULT;
    }

    value = (unsigned int)simple_strtoull(val, NULL, 0);

    dprintk(KERN_INFO "%s:val=%s,after strtoull,value=0x%08x\n",
            __FUNCTION__, val, value);

    if (value != 0){ 
        at91_set_gpio_value(pin, 1);
    } else {        
        at91_set_gpio_value(pin, 0);
    }

    return 0;
}
EXPORT_SYMBOL(hnos_proc_gpio_set);


MODULE_LICENSE("Dual BSD/GPL");

