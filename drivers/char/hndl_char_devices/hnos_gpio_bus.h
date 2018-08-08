/*
 * drivers/char/hndl_char_devices/hnos_gpio_bus.h   
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
#ifndef		HNOS_GPIO_BUS_H
#define		HNOS_GPIO_BUS_H

#define  NR_GPIO_CS             3

#define  GPIO_CS_KEY            0
#define  GPIO_CS_KEY_PIN        AT91_PIN_PC7

#define  GPIO_CS_ID             1
#define  GPIO_CS_ID_PIN         AT91_PIN_PC9

#define  GPIO_CS_STATE          2
#define  GPIO_CS_STATE_PIN      AT91_PIN_PC21

#define  CS_DISABLE             1
#define  CS_ENABLE              0

#define  GBUS_DATA0             AT91_PIN_PC24
#define  GBUS_DATA1             AT91_PIN_PC25
#define  GBUS_DATA2             AT91_PIN_PC26
#define  GBUS_DATA3             AT91_PIN_PC27
#define  GBUS_DATA4             AT91_PIN_PC28
#define  GBUS_DATA5             AT91_PIN_PC29
#define  GBUS_DATA6             AT91_PIN_PC30
#define  GBUS_DATA7             AT91_PIN_PC31

unsigned char gbus_readb(unsigned int cs);

#endif
