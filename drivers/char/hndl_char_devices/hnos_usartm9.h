/*
 *  drivers/char/hndl_char_devices/hnos_usartm9.h
 *  USART 9-bit mode support for at91sam9260.
 *
 *  Copyright (c) Changsha Waion Meters Group.
 *
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
 
#ifndef AT91_M9_H
#define AT91_M9_H

#define USARTM9_DEBUG		1

#if 0
#define M9_DEBUG(fmt...)	printk(fmt)
#else
//#undef USARTM9_DEBUG
#define M9_DEBUG(fmt...)
#endif

#define	USARTM9_PORT_BASE	AT91SAM9260_BASE_US3
#define	USARTM9_ID		AT91SAM9260_ID_US3
#define	USARTM9_CLK		"usart3_clk"

#define M9_BAUD_DEFAULT		2400

static inline void configure_usartm9_pins(void)
{
	at91_set_A_periph(AT91_PIN_PB10, 1);		/* TXD3 */
	at91_set_A_periph(AT91_PIN_PB11, 0);		/* RXD3 */
}

#define M9_MAJOR		0
#define M9_DEV_NAME		"usartm9"

#define BUFF_SIZE	512
#define WAKEUP_CHARS	(BUFF_SIZE >> 1)

#define M9_FLEN		10
#define M9_BAUD		12	

struct m9_icount {
	__u32	rx;
	__u32	tx;
	__u32	frame;
	__u32	overrun;
	__u32	parity;
	__u32	brk;
	__u32	buf_overrun;
};

struct usartm9_port
{
	unsigned long is_open;
	struct miscdevice dev;
	unsigned char* membase;
	struct clk *clk;
	unsigned int uartclk;
	wait_queue_head_t read_wait;
	spinlock_t rlock;
	struct kfifo *read_buf;
	wait_queue_head_t write_wait;
	spinlock_t wlock;
	struct kfifo *write_buf;
	struct m9_icount icount;
//	struct completion close_comp;
	
#ifdef USARTM9_DEBUG
	spinlock_t dbg_rlock;
	struct kfifo *dbg_rbuf;
	spinlock_t dbg_wlock;
	struct kfifo *dbg_wbuf;
#endif
};

#define UART_PUT_CR(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_CR)
#define UART_GET_MR(port)	__raw_readl((port)->membase + ATMEL_US_MR)
#define UART_PUT_MR(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_MR)
#define UART_PUT_IER(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_IER)
#define UART_PUT_IDR(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_IDR)
#define UART_GET_IMR(port)	__raw_readl((port)->membase + ATMEL_US_IMR)
#define UART_GET_CSR(port)	__raw_readl((port)->membase + ATMEL_US_CSR)
#define UART_GET_CHAR(port)	__raw_readl((port)->membase + ATMEL_US_RHR)
#define UART_PUT_CHAR(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_THR)
#define UART_GET_BRGR(port)	__raw_readl((port)->membase + ATMEL_US_BRGR)
#define UART_PUT_BRGR(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_BRGR)
#define UART_PUT_RTOR(port,v)	__raw_writel(v, (port)->membase + ATMEL_US_RTOR)
#define UART_GET_RTOR(port)	__raw_readl((port)->membase + ATMEL_US_RTOR)

static inline int usartm9_rxbuf_empty(struct usartm9_port *port) 
{
	return (0 == kfifo_len(port->read_buf));
}

static inline int usartm9_txbuf_empty(struct usartm9_port *port) 
{
	return (0 == kfifo_len(port->write_buf));
}

static inline int usartm9_txbuf_free(struct usartm9_port *port) 
{
	return (BUFF_SIZE - kfifo_len(port->write_buf));
}

static inline int usartm9_rxbuf_free(struct usartm9_port *port) 
{
	return (BUFF_SIZE - kfifo_len(port->read_buf));
}

#endif

