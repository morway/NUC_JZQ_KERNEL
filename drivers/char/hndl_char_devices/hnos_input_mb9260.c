/*
 *  drivers/char/hndl_char_devices/hnos_input_mb9260.c
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
#include "hnos_gpio.h" 
#include "hnos_input.h"
#include "hnos_iomem.h"

/* 16 bit data width with D8-D15 used as "status rmi". */
#define SMCBUS_INPUT_BASE0	UL(0x30000060)
#define NCHANNEL_PER_SMCBUS	8
#define NR_SMCBUS		1
#define INPUT_SMCBUS_OFFSET	0
#define INPUT_SMCBUS_SIZE	8

static struct iomem_object *iomem_mb9260;
static struct hndl_rmi_device *rmi_mb9260;
static struct smcbus_rmi_data smcbus_mb9260;

static int smcbus_read_all(struct smcbus_rmi_data *bus, u32 *reslt)
{
	u16 data = 0;

	iomem_mb9260->read_word(iomem_mb9260, &data, IO_RDONLY);
	dprintk("%s: %04x\n", __FUNCTION__, data);

	*reslt = ( data >> 8 ) & 0xff;
	return 0;
}

static int smcbus_read_channel(struct smcbus_rmi_data *bus, u8 ch, u32 *reslt)
{
	u16 data = 0;
	u8 shift = 0;
	u8 mask = (1 << 0);
	u8 smcbus_ch = 0;
	u8 smcbus_off = rmi_get_smcbus_offset(rmi_mb9260);

	if ((ch >= (smcbus_off + NR_SMCBUS * NCHANNEL_PER_SMCBUS))) {
		printk("%s: Invalid channel %d.\n", __FUNCTION__, ch);
		return -EFAULT;
	}

	smcbus_ch = ch - smcbus_off;
	shift = smcbus_ch % NCHANNEL_PER_SMCBUS;
	mask = (1 << shift);

	iomem_mb9260->read_word(iomem_mb9260, &data, IO_RDONLY);
	data = (data >> 8) & 0xff;

	dprintk("%s: ch %d, data %x, smcbus_ch %d, shift %d.\n", __FUNCTION__
			, ch, data, smcbus_ch, shift);

	data = (data & mask ) >> shift ;
	*reslt = data;
	return 0;
}

static int smcbus_read(struct smcbus_rmi_data *bus, u8 ch, u32 *reslt)
{
	int ret = 0;

	if (SMCBUS_CHAN_ALL == ch) {
		ret = smcbus_read_all(bus, reslt);
	} else {
		ret = smcbus_read_channel(bus, ch, reslt);
	}

	dprintk("%s: reslt %x.\n", __FUNCTION__, *reslt);
	return ret;
}

static int smcbus_proc_read(struct smcbus_rmi_data *bus, char *buf)
{
	int len = 0, ret = 0;
	u32 reslt = 0;
	ret = smcbus_read_all(bus, &reslt);

	dprintk("%s: %x.\n", __FUNCTION__, reslt);
	len = sprintf(buf + len, "%02x\n", reslt);
	return len;
}

static void  smcbus_channel_remove(void)
{
	rmi_smcbus_unregister(rmi_mb9260, &smcbus_mb9260);
	iomem_object_put(iomem_mb9260);
	return;

}

static int __devinit smcbus_channel_init(void)
{
	int reslt = -1;

#if 1
	iomem_mb9260 = iomem_object_get(SMCBUS_INPUT_BASE0, 0xffff);
	if (!iomem_mb9260) {
		printk(KERN_ERR "%s: can not get iomem object for %ud.\n",
				__FUNCTION__, SMCBUS_INPUT_BASE0);
		return -1;
	}
#endif

	smcbus_mb9260.read = smcbus_read;
	smcbus_mb9260.proc_read = smcbus_proc_read;
	reslt = rmi_smcbus_register(rmi_mb9260, &smcbus_mb9260, 
			INPUT_SMCBUS_OFFSET, INPUT_SMCBUS_SIZE);
	if (reslt < 0) {
		goto smcbus_failed;
	}
	return reslt;

smcbus_failed:
	iomem_object_put(iomem_mb9260);
	return reslt;
}


static int __devinit rmi_mb9260_init(void)
{
	rmi_mb9260 = rmi_device_alloc();
	if (!rmi_mb9260) {
		return -1;
	}

	smcbus_channel_init();
	rmi_device_register(rmi_mb9260);

        HNOS_DEBUG_INFO("RMI mobile9260 unregistered.");
	return 0;
}

static void rmi_mb9260_remove(void)
{
	smcbus_channel_remove();
	rmi_device_unregister(rmi_mb9260);
	rmi_device_free(rmi_mb9260);

        HNOS_DEBUG_INFO("RMI mobile9260 registered.");
	return;
}


module_init(rmi_mb9260_init);
module_exit(rmi_mb9260_remove);

MODULE_LICENSE("Dual BSD/GPL");


