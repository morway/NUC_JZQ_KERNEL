/*
 *  drivers/char/hndl_char_devices/hnos_usartm9.c
 *  USART 9-bit mode support for at91sam9260.
 *
 *  Copyright (c) Changsha Waion Meters Group.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "hnos_generic.h"
#include <asm/mach/serial_at91.h>
#include "hnos_proc.h"
#include "hnos_usartm9.h"
#include "atmel_serial.h"

static int m9_major;
static struct usartm9_port  m9_port =
{
	.dev = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = "usartm9",
	},
	.read_wait = __WAIT_QUEUE_HEAD_INITIALIZER(m9_port.read_wait),
	.write_wait = __WAIT_QUEUE_HEAD_INITIALIZER(m9_port.write_wait),
	.rlock = SPIN_LOCK_UNLOCKED,
	.wlock = SPIN_LOCK_UNLOCKED,
#ifdef USARTM9_DEBUG
	.dbg_rlock = SPIN_LOCK_UNLOCKED,
	.dbg_wlock = SPIN_LOCK_UNLOCKED,
#endif
};

static  struct proc_dir_entry	*hndl_proc_dir = NULL;

#ifdef USARTM9_DEBUG

static size_t m9_dump_regs(struct usartm9_port *port, char *buf)
{
	size_t len = 0;

	len += sprintf(buf + len, "US_MR   : %x\n", UART_GET_MR(port));
	len += sprintf(buf + len, "US_IMR  : %x\n", UART_GET_IMR(port));
	len += sprintf(buf + len, "US_CSR  : %x\n", UART_GET_CSR(port));
	len += sprintf(buf + len, "US_BRGR : %x\n", UART_GET_BRGR(port));
	len += sprintf(buf + len, "US_RTOR : %x\n", UART_GET_RTOR(port));
	return len;
}

static size_t m9_dump_fifos(struct kfifo *fifo, char *buf)
{
	unsigned char data = 0;
	int i = 0;
	size_t cnt = 0;
	size_t len = kfifo_len(fifo);

	for (i=0; i<len; i++) {
		if (i % 16 == 0) {
			cnt += sprintf(buf + cnt, "%4d: ", i);
		}

		kfifo_get(fifo, &data, sizeof(unsigned char));
		cnt += sprintf(buf + cnt, "%2x ", data);

		if ( ( ( ( i + 1 ) % 16 ) == 0 ) ) {
			cnt += sprintf(buf + cnt, "\n");
		}
	}
	cnt += sprintf(buf + cnt, "\n");
	return cnt;
}

int m9_dbg_read(struct proc_item *item, char *page)
{
	int len = 0;
	struct usartm9_port *port = &m9_port;
	struct kfifo *fifo = NULL;

	if (strcmp(item->name, "usartm9_regs") == 0) {
		len = m9_dump_regs(port, page);
	} else if (strcmp(item->name, "usartm9_wrfifo") == 0) {
		fifo = port->dbg_wbuf;
		len = m9_dump_fifos(fifo, page);
	} else if (strcmp(item->name, "usartm9_rdfifo") == 0) {
		fifo = port->dbg_rbuf;
		len = m9_dump_fifos(fifo, page);
	}

	return len;
}

u16 fill_buf[BUFF_SIZE];

static int m9_fill_rdfifo(struct proc_item *item, const char *buffer, unsigned long cnt)
{
	unsigned char local_buf[40] = {'\0'};
	int len = ( cnt > 40 ) ? 40 : cnt;
	struct usartm9_port *port = &m9_port;

	int i = 0;
	int size = 0;

	if (copy_from_user(local_buf, buffer, len)) {
		return -EFAULT;
	}
	
	if (sscanf(local_buf, "gen %d", &size) == 1) {
		M9_DEBUG("Fill rdfifo with %d words.\n", size);

		if (size > BUFF_SIZE) {
			size = BUFF_SIZE;
		}

		for (i=0; i<size; i++) {
			fill_buf[i] = 256 + i;
		}

		kfifo_put(port->read_buf, (char *) fill_buf, size * sizeof(u16));
	}
	return 0;
}

#endif

int m9_line_info(struct proc_item *item, char *buf)
{
	int ret = 0;
	struct usartm9_port *port = &m9_port;

	ret = sprintf(buf, "usartm9: uart: atmel usart mmio:0x%08lX irq:%d",
				(unsigned long) port->membase, USARTM9_ID);

	ret += sprintf(buf + ret, " tx:%d rx:%d",
			port->icount.tx, port->icount.rx);
	if (port->icount.frame) {
		ret += sprintf(buf + ret, " fe:%d", port->icount.frame);
	}
	if (port->icount.parity) {
		ret += sprintf(buf + ret, " pe:%d", port->icount.parity);
	}
	if (port->icount.brk) {
		ret += sprintf(buf + ret, " brk:%d", port->icount.brk);
	} 
	if (port->icount.overrun) {
		ret += sprintf(buf + ret, " oe:%d", port->icount.overrun);
	}

	ret += sprintf(buf + ret, "\n");
	return ret;
}


static struct proc_item m9_items[] = 
{
	{
		.name = "usartm9_info", 
		.read_func = m9_line_info,
	},
#ifdef USARTM9_DEBUG
	{
		.name = "usartm9_regs", 
		.read_func = m9_dbg_read,
	},
	{
		.name = "usartm9_wrfifo", 
		.read_func = m9_dbg_read,
	},
	{
		.name = "usartm9_rdfifo", 
		.read_func = m9_dbg_read,
	},
	{
		.name = "usartm9_data_gen", 
		.write_func = m9_fill_rdfifo,
	},
#endif
	{NULL},

};


/*
 * Deal with parity, framing and overrun errors.
 */
static void m9_rxerr(struct usartm9_port *port, unsigned int status)
{
	/* clear error */
	UART_PUT_CR(port, ATMEL_US_RSTSTA);

	if (status & ATMEL_US_RXBRK) {
		status &= ~(ATMEL_US_PARE | ATMEL_US_FRAME);	/* ignore side-effect */
		port->icount.brk++;
	}
	if (status & ATMEL_US_PARE) {
		port->icount.parity++;
	}
	if (status & ATMEL_US_FRAME) {
		port->icount.frame++;
	}
	if (status & ATMEL_US_OVRE) {
		port->icount.overrun++;
	}

	return;
}

static void m9_rx_chars(struct usartm9_port *port)
{
	size_t free = 0;
 	u16 recv = 0;
 	unsigned int status = 0;

	status = UART_GET_CSR(port);
	while (status & ATMEL_US_RXRDY) {
		free = usartm9_rxbuf_free(port);
		if (unlikely(free == 0)) {
			break;
		}

		recv = UART_GET_CHAR(port) & 0x1FF;
		port->icount.rx++;
		kfifo_put(port->read_buf, (char*)&recv, sizeof(u16));

#ifdef USARTM9_DEBUG
		kfifo_put(port->dbg_rbuf, (char*)&recv, sizeof(u16));
#endif

		if (unlikely(status & (ATMEL_US_PARE | ATMEL_US_FRAME | ATMEL_US_OVRE | ATMEL_US_RXBRK))) {
			m9_rxerr(port, status);
		}
		status = UART_GET_CSR(port);
	}

	if (!usartm9_rxbuf_empty(port)) {
		wake_up_interruptible(&port->read_wait);
	}

	return;
}

/*
 * Start transmitting.
 */
static inline void m9_start_tx(struct usartm9_port *port)
{
	UART_PUT_IER(port, ATMEL_US_TXRDY);
	return;
}

/*
 * Stop transmitting.
 */
static inline void m9_stop_tx(struct usartm9_port *port)
{
	UART_PUT_IDR(port, ATMEL_US_TXRDY);
//	complete(&port->close_comp);
	return;
}

/*
 * Stop receiving - port is in process of being closed.
 */
static inline void m9_stop_rx(struct usartm9_port *port)
{
	UART_PUT_IDR(port, ATMEL_US_RXRDY);
	return;
}

static void m9_tx_chars(struct usartm9_port *port)
{
 	u16 data = 0;
	
	if (usartm9_txbuf_empty(port)) {
		m9_stop_tx(port);
	}

	while (UART_GET_CSR(port) & ATMEL_US_TXRDY) {
		kfifo_get(port->write_buf, (unsigned char *)&data, sizeof(u16));
		UART_PUT_CHAR(port, data);

#ifdef USARTM9_DEBUG
		//M9_DEBUG("tx %4x.\n", data);
		kfifo_put(port->dbg_wbuf, (unsigned char *)&data, sizeof(u16));
#endif
		port->icount.tx++;

		if (usartm9_txbuf_empty(port)) {
			break;
		}
	}

	if (kfifo_len(port->write_buf) < WAKEUP_CHARS) {
		wake_up_interruptible(&port->write_wait);
	}

	if (usartm9_txbuf_empty(port)) {
		m9_stop_tx(port);
	}

	return;
}


static irqreturn_t m9_interrupt(int irq, void *dev_id)
{
 	unsigned int status = 0;
 	unsigned int pending = 0;
	struct usartm9_port *port = (struct usartm9_port *) dev_id;
	
	status = UART_GET_CSR(port);
	pending = status & (UART_GET_IMR(port));
	if (pending & ATMEL_US_RXRDY) {
		m9_rx_chars(port);		
	} else if (pending & ATMEL_US_TXRDY) {
		m9_tx_chars(port);		
	}

	return IRQ_HANDLED;
}

static int m9_open (struct inode *inode, struct file *file)
{
	struct usartm9_port *port = &m9_port;

	if (test_and_set_bit(0, &port->is_open) != 0) {
		return -EBUSY;       
	}

	file->private_data = port; /* for other methods */

#if 1
	if (request_irq(USARTM9_ID, m9_interrupt, 0, "usartm9", port)) {
		return -EBUSY;
	}

	UART_PUT_CR(port, (ATMEL_US_RSTSTA | ATMEL_US_RSTRX));
	UART_PUT_IER(port, ATMEL_US_RXRDY);

	kfifo_reset(port->read_buf);
	kfifo_reset(port->write_buf);

#ifdef USARTM9_DEBUG
	kfifo_reset(port->dbg_rbuf);
	kfifo_reset(port->dbg_wbuf);
#endif

	UART_PUT_CR(port, (ATMEL_US_RXEN | ATMEL_US_TXEN));
#endif

	M9_DEBUG("%s: open usartm9.\n", __FUNCTION__);
	return 0;
}

static int m9_close (struct inode *inode, struct file *file)
{
	struct usartm9_port *port = file->private_data;

	if (test_and_clear_bit(0, &port->is_open) == 0) { /* release lock, and check... */
		return -EINVAL; /* already released: error */
	}

	if (!usartm9_txbuf_empty(port)) {
		if (wait_event_interruptible(port->write_wait, usartm9_txbuf_empty(port))) {
				return -ERESTARTSYS;
		}
	}
#if 1
	UART_PUT_CR(port, (ATMEL_US_RXDIS | ATMEL_US_TXDIS));
	UART_PUT_IDR(port, (ATMEL_US_RXRDY | ATMEL_US_TXRDY));
	free_irq(USARTM9_ID, port);
#endif
	
	M9_DEBUG("%s: release usartm9.\n", __FUNCTION__);
	return 0;
}

static int m9_set_baud (struct usartm9_port *port,  unsigned int baud)
{
	unsigned int quot;
	
	if (baud != 0) {
		quot = ((port->uartclk + (8 * baud)) / (16 * baud)) / 8;
	} else {
		return -EFAULT;
	}

	UART_PUT_BRGR(port, quot);
	return 0;
}

static int m9_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct usartm9_port *port = file->private_data;

	switch (cmd) {
		case M9_FLEN:
			ret = __kfifo_len(port->read_buf);
			break;
		case M9_BAUD:
			ret = m9_set_baud(port, (unsigned int) arg);
			break;
		default:
			ret = -ENOTTY;
			break;
	}
	return ret;
}

static ssize_t m9_read (struct file *file, char __user *buffer, size_t count, loff_t *offset)
{
	size_t cnt = 0;
	size_t length = ( count < BUFF_SIZE ? count : BUFF_SIZE );
	u8 *buf = NULL;
	struct usartm9_port *port = file->private_data; 

	if (count % sizeof(u16) != 0 ) { /* we read only words in 9Bit-Mode */
		return -EIO;
	}

	buf = kmalloc(length, GFP_KERNEL);
	if (!buf) {
		return -ENOMEM;
	}

	cnt = kfifo_get(port->read_buf, buf, length);
	while (cnt == 0) {		
		if (file->f_flags & O_NONBLOCK) {
			kfree(buf);
			return -EAGAIN;
		} else {
			if (wait_event_interruptible((port->read_wait), (kfifo_len(port->read_buf) != 0))) {
				kfree(buf);
				return -ERESTARTSYS;
			}
			cnt = kfifo_get(port->read_buf, buf, length);
		}
	}
	
	if (cnt) {
		if( copy_to_user (buffer, buf, cnt)) {
			kfree(buf);
			return -EFAULT;
		}
	}
	
	M9_DEBUG("%s: read %d data.\n", __FUNCTION__, cnt);
	kfree(buf);
	return cnt;
}

static ssize_t m9_write (struct file *file, const char __user *buffer, size_t count, loff_t *offset)
{
	u8* wbuff = NULL;
	u8* pbuf = NULL;
	size_t wrote = 0;
	size_t free = 0;
	size_t length = count;
	struct usartm9_port *port = file->private_data; 

	if (!count || count%2) {
		M9_DEBUG("%s: invalid count %d.\n", __FUNCTION__, count);
		return -EIO;
	}

	wbuff = kmalloc(length, GFP_KERNEL);
	if (wbuff == NULL) {
		return -ENOMEM;
	}
	pbuf = wbuff;

	if (copy_from_user ((char*)wbuff, buffer, count)) {
		kfree(wbuff);
		return -EFAULT;
	}

	M9_DEBUG("%s: %d data to write.", __FUNCTION__ ,length);

	while (length > 0) {
		if (usartm9_txbuf_free(port) <= WAKEUP_CHARS) {
			if (wait_event_interruptible((port->write_wait), (usartm9_txbuf_free(port) > WAKEUP_CHARS))) {
				kfree(wbuff);
				return -ERESTARTSYS;
			}
		}

		free = usartm9_txbuf_free(port);
		if (free > length) {
			free = length;
		}
		wrote += kfifo_put(port->write_buf, pbuf, free);

		length -= free;
		pbuf += free;

		M9_DEBUG("%s: start TX, %d to write.\n", __FUNCTION__, wrote);
		m9_start_tx(port); /* Start TX. */
	}

	
	M9_DEBUG("%s: write %d data.\n", __FUNCTION__, wrote);
	kfree(wbuff);
	return wrote;
}

unsigned int m9_poll(struct file *file, struct poll_table_struct *wait)
{
	struct usartm9_port *port = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &port->read_wait,  wait);
	poll_wait(file, &port->write_wait, wait);
	if (kfifo_len(port->read_buf) > 0) {
		mask |= POLLIN | POLLRDNORM;	/* readable */
	}
#if 0 
	else if (kfifo_len(port->read_buf) == 0) {
		mask |= POLLHUP | POLLERR;	/* no data available */
	}
#endif
	if (usartm9_txbuf_free(port) > WAKEUP_CHARS) {
		mask |= POLLOUT | POLLWRNORM;	/* writable */
	}
	return mask;
}

static struct file_operations m9_fops =
{
	.owner = THIS_MODULE,
	.open = m9_open,
	.release = m9_close,
	.ioctl = m9_ioctl,
	.read  = m9_read,
	.write = m9_write,
	.poll = m9_poll,
};


static int m9_probe(struct usartm9_port *port)
{
	struct clk *clk = NULL;
	unsigned long uartclk = 0;

	M9_DEBUG("%s.\n", __FUNCTION__);

	configure_usartm9_pins();

	UART_PUT_CR(port, (ATMEL_US_RXDIS | ATMEL_US_TXDIS));

	clk = clk_get(0, USARTM9_CLK);
	if (!clk) {
		printk(KERN_ERR "%s: can't get clock for usartm9.\n", __FUNCTION__);
		return -1;
	}
	clk_enable(clk);
	uartclk = clk_get_rate(clk);

	UART_PUT_MR(port, ((ATMEL_US_USCLKS_MCK_DIV8 | ATMEL_US_PAR_NONE | ATMEL_US_NBSTOP_1 | ATMEL_US_MODE9)));
	//UART_PUT_MR(port, ((ATMEL_US_USCLKS_MCK_DIV8 | ATMEL_US_PAR_NONE | ATMEL_US_NBSTOP_1 | ATMEL_US_CHRL_8)));

	port->clk = clk;
	port->uartclk = uartclk;
	
	M9_DEBUG("%s: usartclk %lx.\n", __FUNCTION__, uartclk);
	m9_set_baud(port, M9_BAUD_DEFAULT);

#if 0
	if (request_irq(USARTM9_ID, m9_interrupt, 0, "usartm9", port)) {
		return -EBUSY;
	}

	UART_PUT_CR(port, (ATMEL_US_RSTSTA | ATMEL_US_RSTRX));
	UART_PUT_IER(port, ATMEL_US_RXRDY);

	kfifo_reset(port->read_buf);
	kfifo_reset(port->write_buf);

#ifdef USARTM9_DEBUG
	kfifo_reset(port->dbg_rbuf);
	kfifo_reset(port->dbg_wbuf);
#endif

	UART_PUT_CR(port, (ATMEL_US_RXEN | ATMEL_US_TXEN));
#endif

	return 0;
}

static int __init  m9_proc_create(void)
{
	struct proc_item *item;
	int ret = 0;

	for (item = m9_items; item->name; ++item) {
		ret += hnos_proc_entry_create(item);
	}

	return ret;
}

static int m9_proc_remove(void)
{
	struct proc_item *item;

	for (item = m9_items; item->name; ++item) {
		remove_proc_entry(item->name, hndl_proc_dir);
	}

	return 0;
}



static int __init m9_init(void)
{
	struct usartm9_port *port = &m9_port;
	void *membase = NULL;
	int ret = 0;

	port->dev.fops = &m9_fops;

	if ((m9_major = misc_register(&port->dev)) < 0) {
		printk(KERN_ERR M9_DEV_NAME ": Unable to register device at91-m9");
		return -EIO;
	}
	
	membase = ioremap(USARTM9_PORT_BASE, 1024);
 	if (membase == NULL) {
		ret = -ENOMEM;
 		printk(KERN_ERR M9_DEV_NAME ": Unable to allocate memmory %d\n", M9_MAJOR);
		goto fail5;
	}	
	M9_DEBUG("%s: USARTM9_PORT_BASE %x, membase %x.\n", __FUNCTION__,
			(unsigned int) USARTM9_PORT_BASE, (unsigned int)membase);

	port->read_buf = kfifo_alloc(BUFF_SIZE, GFP_KERNEL, &port->rlock);
	if (!port->read_buf) {
		ret = -ENOMEM;
		goto fail4;
	}

	port->write_buf = kfifo_alloc(BUFF_SIZE, GFP_KERNEL, &port->wlock);
	if (!port->write_buf) {
		ret = -ENOMEM;
		goto fail3;
	}

#ifdef USARTM9_DEBUG
	port->dbg_rbuf = kfifo_alloc(BUFF_SIZE, GFP_KERNEL, &port->dbg_rlock);
	if (!port->dbg_rbuf) {
		ret = -ENOMEM;
		goto fail2;
	}

	port->dbg_wbuf = kfifo_alloc(BUFF_SIZE, GFP_KERNEL, &port->dbg_wlock);
	if (!port->dbg_wbuf) {
		ret = -ENOMEM;
		goto fail1;
	}

#endif

	port->membase = membase;
//	init_completion(&port->close_comp);

	ret = m9_probe(port);
	if (ret) {
		goto fail0;
	}

	hndl_proc_dir = hnos_proc_mkdir();
	if (!hndl_proc_dir) {
		ret = -ENODEV;
		goto fail0;

	} else {
		ret = m9_proc_create();
		if (ret) {
			hnos_proc_rmdir();
			goto fail0;
		}
	}

	printk(KERN_INFO M9_DEV_NAME ": driver loaded.\n");
	return 0;	

fail0:	
#ifdef USARTM9_DEBUG
	kfifo_free(port->dbg_wbuf);
#endif
fail1:	
#ifdef USARTM9_DEBUG
	kfifo_free(port->dbg_rbuf);
#endif

fail2:	
	kfifo_free(port->write_buf);
fail3:	
	kfifo_free(port->read_buf);
fail4:	
	iounmap(membase);
fail5:	
	misc_deregister(&port->dev);
	
	return ret;	
}

static void __exit m9_exit(void)
{
	struct usartm9_port *port = &m9_port;

	m9_stop_tx(port);
	m9_stop_rx(port);

#if 0
	UART_PUT_CR(port, (ATMEL_US_RXDIS | ATMEL_US_TXDIS));
	UART_PUT_IDR(port, (ATMEL_US_RXRDY | ATMEL_US_TXRDY));
	free_irq(USARTM9_ID, port);
#endif

	m9_proc_remove();
	hnos_proc_rmdir();

	misc_deregister(&port->dev);

	kfifo_free(port->read_buf);
	kfifo_free(port->write_buf);

#ifdef USARTM9_DEBUG
	kfifo_free(port->dbg_rbuf);
	kfifo_free(port->dbg_wbuf);
#endif
	iounmap(port->membase);

	clk_disable(port->clk);
	clk_put(port->clk);	
	return;
}

module_init(m9_init);
module_exit(m9_exit);

MODULE_DESCRIPTION("ATMEL AT91SAM9260 USART Mode 9Bit.");
MODULE_LICENSE("GPL");


