/*
 * drivers/char/hndl_char_devices/hnos_iomem.h   
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
#ifndef HNOS_IOMEM_H
#define HNOS_IOMEM_H

#define NR_MAX_IOMEM		16
#define IO_WRONLY		1
#define IO_RDONLY		2
#define IO_WRRD		        3

#define IOMEM_REMAP_SIZE	4

struct iomem_object 
{
	unsigned int phy_base;
	void *iomem_base;
	spinlock_t lock;
	unsigned long ref_cnt;
	u32 data;

	/* Byte (8 bits) read/write interface. */
	int (*write_byte) (struct iomem_object *iomem, u8 byte);
	int (*write_bit) (struct iomem_object *iomem, u8 bitmap, u8 is_set);
	int (*read_byte)(struct iomem_object *iomem, u8 *reslt, int flags);

	/* Word (16 bits) read/write interface. */
	int (*write_word) (struct iomem_object *iomem, u16 byte);
	int (*writew_bit) (struct iomem_object *iomem, u16 bitmap, u8 is_set);
	int (*read_word)(struct iomem_object *iomem, u16 *reslt, int flags);
};

struct iomem_object* iomem_object_get(unsigned long phy_base, u32 init_data);
int iomem_object_put(struct iomem_object *iomem);
#endif
