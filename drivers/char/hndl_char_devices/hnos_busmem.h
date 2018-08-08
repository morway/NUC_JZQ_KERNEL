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
#ifndef		HNOS_IOMAMORY_H
#define		HNOS_IOMAMORY_H


#define BUSMEM_OUTPUT_GET             0xa1
#define BUSMEM_OUTPUT_SET             0xa2
#define BUSMEM_OUTPUT_SET_BIT         0xa3
#define BUSMEM_OUTPUT_GET_BIT         0xa4

#define BUSMEM_INPUT_GET              0xa5
#define BUSMEM_INPUT_SET              0xa6
#define BUSMEM_INPUT_SET_BIT          0xa7
#define BUSMEM_INPUT_GET_BIT          0xa8

struct busmem_object 
{
	u8 data;
	spinlock_t lock;	
	unsigned int phy_base;
	void __iomem *iomem_base;
        char *name;
};

int hnos_busmem_opt(int cmd, int cs_index,int data,u8 bit);

#endif
