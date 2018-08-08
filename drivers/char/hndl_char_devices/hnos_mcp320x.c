/*
 * drivers/char/hndl_char_devices/hnos_mcp320x.c 
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

#include "hnos_generic.h"
#include "hnos_dc_intf.h" 

#define BUF_SIZE	6
#define TX_BUF_SIZE	(BUF_SIZE / 2)

struct  mcp320x_device
{
    struct spi_device *spi;
    u8 local_buf[BUF_SIZE];
    struct adc_ops    *adc;
};

static struct mcp320x_device *mcp320x;

static inline void mcp320x_dump(u8 *buf, unsigned int size, u8 ch)
{
#ifdef MCP320X_DEBUG
    int i = 0;

    printk("mcp320x ch%d recv: ", ch);
    for (i=0; i<size; i++) {
        printk("%2x ", buf[i]);
    }
    printk("\n");

#endif 
    return;
}

static int __mcp320x_read(u8 ch, u32 *reslt)
{
    struct mcp320x_device *dev = mcp320x;
    struct spi_device *spi;
    struct spi_message message;
    struct spi_transfer xfer;

    u8  rx_buf[3] = {0xff, 0xff, 0xff};
    int status = 0;

    if (!dev || !dev->spi || !reslt) {
        printk(KERN_ERR "%s: invalide params.\n", __FUNCTION__);
        return -EINVAL;
    }

    spi = dev->spi;

    memset(dev->local_buf, 0, BUF_SIZE);
    dev->local_buf[0] = 0x04;
    if (ch >= 1) {  /* Channel 1 */
        dev->local_buf[1] = 0x80;
    }

    /* Build our spi message */
    spi_message_init(&message);
    memset(&xfer, 0, sizeof(xfer));
    xfer.len = TX_BUF_SIZE;
    xfer.tx_buf = dev->local_buf;
    xfer.rx_buf = &dev->local_buf[TX_BUF_SIZE];

    spi_message_add_tail(&xfer, &message);

    /* do the i/o */
    status = spi_sync(spi, &message);
    if (status == 0) {
        status = message.status;
        memcpy(rx_buf, xfer.rx_buf, TX_BUF_SIZE);
    } else {
        printk(KERN_ERR "%s: read mcp320x failed, status = %x\n", __FUNCTION__, status);
        goto fail;
    }

    mcp320x_dump(rx_buf, TX_BUF_SIZE, ch);

    *reslt = (((rx_buf[1] & 0xf) << 8) | rx_buf[2]);
    dprintk("%s: ch %d, reslt %d.\n", __FUNCTION__, ch, *relst);

    return 0;

fail:
    *reslt = 0;
    return status;
}

#define NR_MCP320X_TIMES      4

static int mcp320x_read(u8 ch, u32 *reslt)
{
	unsigned int total = 0, tmp = 0, valid = 0;
	int i = 0, ret = 0;

	for (i=0; i<NR_MCP320X_TIMES; i++) {
		ret = __mcp320x_read(ch, &tmp);
		if (ret < 0) {
			continue;
		}

		total += tmp;
		valid ++;
        msleep_interruptible(10);
	}

	if (unlikely(valid == 0)) {
		printk(KERN_ERR "%s: no valid ADC data read, tried %d times.",
                __FUNCTION__, NR_MCP320X_TIMES);
		return -EIO;
	}

	*reslt = (total / valid);
    dprintk("%s: ch %d, relst %d.\n", __FUNCTION__, ch, *reslt);
	return 0;
}

static int  mcp320x_remove(struct spi_device *spi)
{
    if (mcp320x){
        if (mcp320x->adc) {
            dc_adc_unregister(mcp320x->adc);
            kfree(mcp320x->adc);
            mcp320x->adc = NULL;
        }

        kfree(mcp320x);
        mcp320x = NULL;
    }

    return 0;
}

static int __devinit mcp320x_probe(struct spi_device *spi)
{
    int          result = 0;
    struct adc_ops *adc = NULL;

    mcp320x = kmalloc(sizeof(struct mcp320x_device), GFP_KERNEL);
    if (!mcp320x) {
        result = -ENOMEM;
        goto fail; 
    }

    memset(mcp320x, 0, sizeof(struct mcp320x_device));	

    adc = kmalloc(sizeof(struct adc_ops), GFP_KERNEL);
    if (!adc) {
        result = -ENOMEM;
        goto adc_failed;
    }

    adc->read = mcp320x_read;
    result = dc_adc_register(adc);
    if (result) {
        goto reg_failed;
    }

    mcp320x->spi = spi;
    mcp320x->adc = adc;
    return 0;

reg_failed:
    kfree(adc);
    adc = NULL;

adc_failed:
    kfree(mcp320x);
    mcp320x = NULL;

fail:
    return result;
}

static struct spi_driver mcp320x_driver = {
    .driver = {
        .name	= "mcp320x",
        .owner	= THIS_MODULE,
    },
    .probe	= mcp320x_probe,
    .remove	= __devexit_p(mcp320x_remove),
};

static int __init  mcp320x_init(void)
{
    HNOS_DEBUG_INFO("Module mcp320x init.\n");
    return spi_register_driver(&mcp320x_driver);
}

static void __exit mcp320x_exit(void)
{
    spi_unregister_driver(&mcp320x_driver);
    HNOS_DEBUG_INFO("Module mcp320x exit.\n");
    return;
}

module_init(mcp320x_init);
module_exit(mcp320x_exit);

MODULE_LICENSE("Dual BSD/GPL");
