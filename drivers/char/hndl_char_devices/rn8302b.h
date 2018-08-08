#ifndef _RN8302B_H
#define _RN8302B_H
#include "atmel_spi.h"
#define RN8302B_BUF_SIZE	2048//1024

void RN8302_SPI_Init(void);
void RN8302_Delay( unsigned long n );
unsigned char  RN8302_Read( unsigned char *RAM_Addr, unsigned int ADE78xx_Addr, unsigned char Lenth );
unsigned char  RN8302_Write( unsigned int ADE78xx_Addr, unsigned char *RAM_Addr, unsigned char Lenth );

		
struct  rn8302b_spi_device
{
	struct cdev cdev;
	struct class *class;
	struct spi_device *spi;
    u8     tx_buf[RN8302B_BUF_SIZE];
	u8     rx_buf[RN8302B_BUF_SIZE];
	unsigned long is_open;
};

#endif
