
/*
 * drivers/char/hndl_char_devices/hnos_ap89170.h   
 *
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */
 
#ifndef _HNOS_AP89170_H_
#define _HNOS_AP89170_H_

#define AP89170_DEV_NAME    "AP89170"   	//此即为/dev目录下AP89170的设备节点
#define TX_BUFSIZE	 4
#define RX_BUFSIZE	 4
#define POWER_TIMEOUT       10
#define WAITPLAY_TIMEOUT    150
#define PLAYEND_TIMEOUT     100
#define PLAYSTART_TIMEOUT   2
#define WAITEMPTY_TIMEOUT   150
#define FILLBUF_TIMEOUT     150

/*由于AT9260的SPI是先发送高位再发低位，而AP89170读数据是低位在前，所以要把发送的命令字换过来*/
#define AP89170_CMD_PUP1            0xA3   	//0xC5 
#define AP89170_CMD_PUP2            0xB1    //0x8D
#define AP89170_CMD_PDN1            0x87  	//0xE1
#define AP89170_CMD_PDN2            0x95    //0xA9
#define AP89170_CMD_PLAY            0xAA    //0x55
#define AP89170_CMD_STATUS          0xC7 	//0xE3
#define AP89170_CMD_PAUSE           0x9C  	//0x39
#define AP89170_CMD_RESUME          0xB8 	//0x1D
#define AP89170_CMD_PREFETCH        0x8E 	//0x71

#ifdef CONFIG_HNDL_PRODUCT_HNDL1000X
#define AP89170_SIGNAL_ADR      UL(0x30000060)
#define AP89170_BUSY                    0x40
#define AP89170_FULL                    0x80
#define AP89170_BUSY_FULL               0xC0
#define GPIO_VOICE_CTRL         AT91_PIN_PA14
#elif defined CONFIG_HNDL_PRODUCT_HNTT1800SSC
#define AP89170_SIGNAL_ADR      UL(0x300000C0)
#define AP89170_BUSY                    0x10
#define AP89170_FULL                    0x20
#define AP89170_BUSY_FULL               0x30
#define GPIO_VOICE_CTRL         0
#else
#define AP89170_SIGNAL_ADR      UL(0x00000000)
#define AP89170_BUSY                    0x0
#define AP89170_FULL                    0x0
#define AP89170_BUSY_FULL               0x0
#define GPIO_VOICE_CTRL         0
#endif 

enum  EPower
{
	ePowerON = 1,
	ePowerOFF = 0
};

#endif 
