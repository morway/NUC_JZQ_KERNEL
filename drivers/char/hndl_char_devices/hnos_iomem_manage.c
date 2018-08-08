/*
 *  drivers/char/hndl_char_devices/hnos_smcbus_manage.c
 *
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
#include "hnos_ioctl.h" 
#include "hnos_proc.h" 
#include "hnos_iomem.h"

static DECLARE_MUTEX(iomem_lock);
static struct iomem_object *iomem_pool[NR_MAX_IOMEM] = {NULL};
static int nr_iomem = 0;

int iomem_write_byte(struct iomem_object *iomem, u8 byte)
{
	unsigned long flags;
	
        if(!iomem) {
                return -1;
	}

	dprintk("%s: iomem phy %08x, byte %02x.\n",
			__FUNCTION__, (unsigned int)iomem->phy_base, byte);

	spin_lock_irqsave(&iomem->lock, flags);
	iomem->data = byte;
	writeb(byte, iomem->iomem_base);
	spin_unlock_irqrestore(&iomem->lock, flags);
	return 0;
}

int iomem_write_bit(struct iomem_object *iomem, u8 bitmap, u8 is_set)
{
	unsigned long flags;
	u8 data = 0;
	
	if(!iomem) {
                return -1;
	}

	dprintk("%s: iomem phy %08x, bitmap %02x, %s.\n",
			__FUNCTION__, (unsigned int)iomem->phy_base, 
			(unsigned int) bitmap, (is_set ? "SET" : "CLEAR"));

	spin_lock_irqsave(&iomem->lock, flags);

	data = iomem->data ;
	if (bitmap & 0xff) {
		if (is_set) {
			data |= (bitmap);
		} else {
			data &= (~bitmap);
		}
		writeb(data, iomem->iomem_base);
	}

	iomem->data = data;
	spin_unlock_irqrestore(&iomem->lock, flags);
	return 0;
}

int iomem_read_byte(struct iomem_object *iomem, u8 *reslt, int flags)
{
	unsigned long fg;
	u8 data = 0;
	
	if (!iomem) {
                return -1;
	}

	spin_lock_irqsave(&iomem->lock, fg);

	if (flags == IO_WRONLY){
	        data = iomem->data;
	} else {
                data = readb(iomem->iomem_base) & 0xff;
	}
	spin_unlock_irqrestore(&iomem->lock, fg);
	
	dprintk("%s: iomem phy %08x, data %02x.\n",
			__FUNCTION__, (unsigned int)iomem->phy_base, data);

	*reslt = data;
	return 0;
}

int iomem_write_word(struct iomem_object *iomem, u16 word)
{
	unsigned long flags;
	
	if (!iomem) {
                return -1;
	}

	dprintk("%s: iomem phy %08x, word %04x.\n",
			__FUNCTION__, (unsigned int)iomem->phy_base, word);

	spin_lock_irqsave(&iomem->lock, flags);
	iomem->data = word;
	writew(word, iomem->iomem_base);
	spin_unlock_irqrestore(&iomem->lock, flags);
	return 0;
}

int iomem_writew_bit(struct iomem_object *iomem, u16 bitmap, u8 is_set)
{
	unsigned long flags;
	u16 data = 0;
	
	if (!iomem) {
                return -1;
	}

	dprintk("%s: iomem phy %08x, bitmap %04x, %s.\n",
			__FUNCTION__, (unsigned int)iomem->phy_base, 
			(unsigned int) bitmap, (is_set ? "SET" : "CLEAR"));

	spin_lock_irqsave(&iomem->lock, flags);

	data = iomem->data ;
	if (bitmap & 0xffff) {
		if (is_set) {
			data |= (bitmap);
		} else {
			data &= (~bitmap);
		}
		writew(data, iomem->iomem_base);
	}

	iomem->data = data;
	spin_unlock_irqrestore(&iomem->lock, flags);
	return 0;
}

int iomem_read_word(struct iomem_object *iomem, u16 *reslt, int flags)
{
	unsigned long fg;
	u16 data = 0;
	
	if (!iomem) {
		return -1;
	}

	spin_lock_irqsave(&iomem->lock, fg);
	if (flags == IO_WRONLY){
		data = iomem->data;
	} else {
		data = readw(iomem->iomem_base);
	}
	spin_unlock_irqrestore(&iomem->lock, fg);
	
	dprintk("%s: iomem phy %08x, data %04x.\n",
			__FUNCTION__, (unsigned int)iomem->phy_base, data);

	*reslt = data;
	return 0;
}


static int iomem_pool_empty(void)
{
	return ( ( 0 == nr_iomem ) ? 1 : 0 );
}

static  int iomem_pool_full(void)
{
	return ( ( NR_MAX_IOMEM == nr_iomem ) ? 1 : 0 );
}

static int iomem_object_insert(struct iomem_object *obj)
{
	int i = 0;
	int ret = 0;

	down(&iomem_lock);
	if (iomem_pool_full()) {
		printk(KERN_ERR "%s: iomem pool full.\n", __FUNCTION__);
		ret = -1;
		goto out;
	}

	/* Found an empty slot.*/
	for (i=0; i<NR_MAX_IOMEM; i++) {
		if (!iomem_pool[i]) {
			iomem_pool[i] = obj;
			nr_iomem = (nr_iomem + 1) % NR_MAX_IOMEM;
			HNOS_DEBUG_INFO("iomem(phy %08x, vir %08x) added to iomem_pool[%d], nr_iomem %d.\n", 
					 (unsigned int)obj->phy_base, (unsigned int)obj->iomem_base, i, nr_iomem);
			ret = 0;
			goto out;
		}
	}

	ret = -1;
	printk(KERN_ERR "%s: iomem_pool not full but we can't find an empty slot for phy_base %08x.\n", 
			__FUNCTION__, obj->phy_base);
out:
	up(&iomem_lock);
	return ret;
}

static int iomem_object_delete(struct iomem_object *obj)
{
	int i = 0;
	int ret = 0;

	down(&iomem_lock);
	if (iomem_pool_empty()) {
		printk(KERN_ERR "%s: iomem pool empty.\n", __FUNCTION__);
		ret = -1;
		goto out;
	}

	/* Found the specified object.*/
	for (i=0; i<NR_MAX_IOMEM; i++) {
		if (iomem_pool[i] && (iomem_pool[i] == obj)) {
			iounmap(obj->iomem_base);
			iomem_pool[i] = NULL;
			nr_iomem = (nr_iomem - 1) % NR_MAX_IOMEM;
			HNOS_DEBUG_INFO("iomem(phy %08x, vir %08x) removed from iomem_pool[%d], nr_iomem %d.\n", 
					(unsigned int) obj->phy_base, (unsigned int)obj->iomem_base, i, nr_iomem);
			ret = 0;
			goto out;
		}
	}

	ret = -1;
	printk(KERN_ERR "%s: iomem_pool not empty but we can't find the specified obj (phy %08x).\n", 
			__FUNCTION__, obj->phy_base);
out:
	up(&iomem_lock);
	return ret;
}

struct iomem_object* iomem_object_alloc(unsigned long phy_base, u32 data)
{
	struct iomem_object *iomem = NULL;

	iomem = kmalloc(sizeof(struct iomem_object), GFP_KERNEL);
	if (!iomem) {
		printk(KERN_ERR "%s: no enough memory.\n", __FUNCTION__);
		goto out;
	}
	memset(iomem, 0, sizeof(struct iomem_object));
	spin_lock_init(&iomem->lock);

	iomem->phy_base = phy_base;
	iomem->iomem_base = ioremap(phy_base, IOMEM_REMAP_SIZE);
	if (!iomem->iomem_base) {
		printk(KERN_ERR "%s: can't remap phy_base %x.\n", __FUNCTION__,(unsigned int) phy_base);
		kfree(iomem);
		iomem = NULL;
		goto out;
	}

	iomem->data = data;
	iomem->ref_cnt ++;

	iomem->write_byte = iomem_write_byte;
	iomem->write_bit = iomem_write_bit;
	iomem->read_byte = iomem_read_byte;

	iomem->write_word = iomem_write_word;
	iomem->writew_bit = iomem_writew_bit;
	iomem->read_word = iomem_read_word;

	if (iomem_object_insert(iomem) < 0) {
		printk(KERN_ERR "%s: can't insert the iomem(phy %08x) to the iomem pool.\n",
				__FUNCTION__, (unsigned int) phy_base);
		iounmap(iomem->iomem_base);
		kfree(iomem);
		iomem = NULL;
	}
out:
	return iomem;
}

struct iomem_object* iomem_object_get(unsigned long phy_base, u32 init_data)
{
	struct iomem_object *iomem = NULL;
	int i = 0;
	unsigned long flags;

    if(!phy_base)
        return NULL;
        
	if ( iomem_pool_empty() ) {
		goto new_object;
	}

	down(&iomem_lock);

	/* Check if this iomem already exsited. */
	for (i=0; i<NR_MAX_IOMEM; i++) {
		if (iomem_pool[i] && (iomem_pool[i]->phy_base == phy_base)) {
			iomem = iomem_pool[i];

			spin_lock_irqsave(&iomem->lock, flags);
			iomem->ref_cnt ++;
			iomem->data |= init_data; 
			spin_unlock_irqrestore(&iomem->lock, flags);

			HNOS_DEBUG_INFO("iomem (phy %08x, vir %08x) ref_cnt %ld, found in iomem_pool[%d].\n", 
					(unsigned int)phy_base, (unsigned int)iomem->iomem_base, iomem->ref_cnt, i);
			up(&iomem_lock);
			return iomem;
		}
	}

	up(&iomem_lock);
	
	/* Not found, allocated a new one. */
new_object:
	iomem = iomem_object_alloc(phy_base, init_data);
	return iomem;
}

static int iomem_object_free(struct iomem_object *obj)
{
	iomem_object_delete(obj);
	kfree(obj);
	obj = NULL;
	return 0;
}

int iomem_object_put(struct iomem_object *iomem)
{
	unsigned long flags;

	if ( !iomem || iomem_pool_empty() ) {
		printk(KERN_ERR "%s: empty iomem or iomem pool is empty.\n", __FUNCTION__);
		return -1;
	}

	spin_lock_irqsave(&iomem->lock, flags);
	iomem->ref_cnt --;
	if (!iomem->ref_cnt) {
		spin_unlock_irqrestore(&iomem->lock, flags);
		return iomem_object_free(iomem);
	}

	spin_unlock_irqrestore(&iomem->lock, flags);
	return 0;
}
EXPORT_SYMBOL(iomem_object_get);
EXPORT_SYMBOL(iomem_object_put);

MODULE_LICENSE("Dual BSD/GPL");

