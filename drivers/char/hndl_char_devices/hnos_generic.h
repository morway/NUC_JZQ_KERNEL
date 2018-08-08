/*
 * drivers/char/hndl_char_devices/hnos_generic.h   
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
#ifndef		HNOS_GENERIC_H
#define		HNOS_GENERIC_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/nmi.h>
#include <linux/mutex.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/ctype.h>
#include <asm/uaccess.h>

#include <mach/gpio.h>
#include <linux/slab.h>
#include <linux/gpio-pxa.h>

//#include <asm/arch/adc.h>
//#include <asm/arch/board.h>
//#include <asm/arch/gpio.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>               /*request_irq()...*/
#include <linux/circ_buf.h>                /*circle buffer...*/
#include <linux/proc_fs.h>                 /*proc fs*/
#include <linux/times.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
//#include <linux/hnos_product.h>
//#include <linux/hnos_debug.h>
//#include <asm/arch/at91_tc.h>
//#include <asm/arch/at91_rstc.h>
//#include <asm/arch/at91sam926x_mc.h>
//#include <asm/arch/at91sam9260_matrix.h>
//#include <asm/arch/at91sam9260.h>
//#include <asm/arch/at91_pio.h>
#include <linux/kfifo.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/poll.h>

#endif
