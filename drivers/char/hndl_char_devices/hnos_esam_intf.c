/*
 * drivers/char/hndl_char_devices/hnos_esam_intf.c 
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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include "hnos_generic.h"
#include "hnos_esam_intf.h"
#include "hnos_ioctl.h"
#include "hnos_debug.h"

#include <mach/map.h>
#include <mach/regs-gpio.h>

#define ESAM_CLK	259		//PI3
#define ESAM_MISO	261		//PI5
#define ESAM_MOSI	262		//PI6
#define ESAM_CS		260		//PI4
#define ESAM_PWR	263		//PI7
#define MAJOR_NUM   39	/* 主设备号	*/
static  int esam_spi_major = MAJOR_NUM;

ssize_t esam_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t esam_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
int esam_open(struct inode *inode, struct file *filp);
int esam_release(struct inode *inode, struct file *filp);

int nuc970_gpio_core_direction_out1(unsigned gpio_num, int val);
int nuc970_gpio_core_set1(unsigned gpio_num,int val);
int nuc970_gpio_core_get1( unsigned gpio_num);
int nuc970_gpio_core_direction_in1(unsigned gpio_num);


#define ESAM_CLK_H {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t|0x08);}   //pI3
#define ESAM_CLK_L {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t&~(0x08));}//pI3

#define ESAM_MOSI_H {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t|0x40);}   //pI6
#define ESAM_MOSI_L {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t&~(0x40));}//pI6

#define ESAM_MISO_Read nuc970_gpio_core_get1(ESAM_MISO)

#define ESAM_CS_H {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t|0x10);}   //pI4
#define ESAM_CS_L {register unsigned int t= (*(volatile unsigned int *)REG_GPIOI_DATAOUT);((*(volatile u32 *) (REG_GPIOI_DATAOUT))=t&~(0x10));}//pI4



static void rxtx_udelay(int udelay)
{
return ;
}

unsigned char ESAM_ReceiveByte(void)
{
    unsigned char i,RxData=0;
    
    for( i = 0; i < 8; i++ )
    {
        ESAM_CLK_L;
 
        RxData <<= 1;

		//rxtx_udelay(1);
		udelay(1);
        if( ESAM_MISO_Read )
            RxData |= 0x01;
        //udelay(1);
        ESAM_CLK_H;
		//rxtx_udelay(1);
		udelay(1);
		
    }
    return(RxData);
}



struct file_operations esam_fops =
{
	read: 	esam_read,
	write:	esam_write,
//	ioctl:  esam_ioctl,
	open:   esam_open,
	release:esam_release,
};

static u8 CMD_SENT  = 0;
static struct esam_spi_device *esam_spi;

static void es_cspin(unsigned char status)
{
	//at91_set_gpio_output(ESAM_CS, status ? 1:0);
	nuc970_gpio_core_direction_out1(ESAM_CS, status ? 1:0);
}

static void es_pwrpin(unsigned char status)
{
	//at91_set_gpio_output(ESAM_PWR, status ? 0:1);	//低电平 上电
	nuc970_gpio_core_direction_out1(ESAM_PWR, status ? 0:1);
}

static unsigned char  cal_lrc(u8 *data, u32 len)
{
	unsigned char LRCx;
    int i;

	if(len == 0 || !data)
		return 0;
	if(len == 1)
		return ~(data[0]);
	LRCx = data[0];
	for(i=1; i<len; i++)
		LRCx ^= data[i];
	LRCx = (~LRCx);
	return LRCx;
}



static int	esam_spi_write(u32 len, const u8 __user *reslt)
{
    struct esam_spi_device *dev = esam_spi;
    int  i,j;

	nuc970_gpio_core_set1(ESAM_MOSI, 0);
	nuc970_gpio_core_set1(ESAM_CLK, 0);
	ESAM_CLK_H;
	es_cspin(1);
	

    if (!dev || !reslt || (len + 2> ESAM_BUF_SIZE)) {
        printk(KERN_ERR "%s: invalide params.\n", __FUNCTION__);
        return -EINVAL;
    }


	//prepare data frame
    dev->tx_buf[0] = 0x55;
	copy_from_user(dev->tx_buf + 1, reslt, len);
	dev->tx_buf[1+len] = cal_lrc(dev->tx_buf + 1, len);	
	//for(i=0;i<len+2;i++)
	//printk("txbuf[i]=%02x\n",dev->tx_buf[i]);
	
	udelay(10);	//CS高 10us
	es_cspin(0);
	udelay(50);	//CS低 50us
	    
	/* do the i/o.  */
	for(i=0; i<len + 2; i++)
	{
		for(j=0;j<8;j++){
    		ESAM_CLK_L;
    		//rxtx_udelay(1);
			udelay(1);
       		if( ((dev->tx_buf[i]) & 0x80) == 0x80 )
        	{
            	ESAM_MOSI_H;
        	}
        	else
        	{
            	ESAM_MOSI_L;
        	}
 
			//rxtx_udelay(1);
			udelay(1);
        	ESAM_CLK_H;
      		//rxtx_udelay(2);
			udelay(1);
        	(dev->tx_buf[i]) <<= 1;
		}
	}
	
	udelay(1);	
	es_cspin(1);
	udelay(10);	//CS高 10us

	CMD_SENT = 1;  	
	return 0;	
	
}




#if 0
static int  esam_read_status(struct spi_device *spi, u8 *e_stat)
{
    struct spi_message message;
    struct spi_transfer xfer;
    int status = 0;
	u8  tx, rx;

    if (!spi || !e_stat) {
        printk(KERN_ERR "%s: invalide params.\n", __FUNCTION__);
        return -EINVAL;
    }

    udelay(1);	
    /* Build our spi message */
    spi_message_init(&message);
    memset(&xfer, 0, sizeof(xfer));
    xfer.len = 1;
    xfer.tx_buf = &tx;
    xfer.rx_buf = &rx;

	tx = 0x00;
	rx = 0x00;
    spi_message_add_tail(&xfer, &message);
	
	/* do the i/o. read the esam status */
	status = spi_sync(spi, &message);
	
	if (status == 0) {
	    *e_stat = rx;
		return 0; 
	} else {
		printk("esam_read_status: failed.\n");
		return status;
	}
}
#endif
static int	esam_spi_read(u32 maxlen, u8 __user *reslt)
{
    struct esam_spi_device *dev = esam_spi;
    int  len, timeout = 0,i;

//	at91_set_A_periph(ESAM_MOSI, 0);
//	at91_set_A_periph(ESAM_MISO, 0);
//    at91_set_A_periph(ESAM_CLK, 0);
	nuc970_gpio_core_set1(ESAM_MOSI, 1);
	

    if (!dev || !reslt) {
        printk(KERN_ERR "%s: invalide params.\n", __FUNCTION__);
        return -EINVAL;
    }


	
	if(CMD_SENT	== 0)
	{
		memset(dev->rx_buf, 0x00, maxlen > 5? 5:maxlen);
		copy_to_user(reslt, dev->rx_buf, maxlen > 5? 5:maxlen);
		return 0;
	}

	udelay(10);	//CS高 10us
	es_cspin(0);
	udelay(50);	//CS低 50us
#if 0
_waitread:
	if (esam_read_status(spi, &e_stat) == 0) {
		if(e_stat != 0x55)
		{
			if(++timeout >= 3200*1000/50)
			{
				udelay(1);	
				es_cspin(1);
				udelay(10);	//CS高 10us
				return -EIO;
			}
			
			udelay(50);
			goto _waitread; 
		}  
	} else {
	    udelay(1);	
		es_cspin(1);
		udelay(10);	//CS高 10us
		return -EIO;
	}

	udelay(2);		//字节间延迟 2us	
	/* do the i/o.  */
	for(i=0; i<4; i++)
	{
		status = esam_read_status(spi, dev->rx_buf + i);
		if(status)
		{
			printk(KERN_ERR "%s:failed, status = %x\n", __FUNCTION__, status);
			udelay(1);	
			es_cspin(1);
			udelay(10);	//CS高 10us
			return -EIO;
		}
	}
	copy_to_user(reslt, dev->rx_buf, 4);
	
	/* read the data, if exist */
    len = dev->rx_buf[2]*256 + dev->rx_buf[3];
	if(len + 4 > maxlen)
	{
		printk(KERN_ERR "%s: read buffer too small to read!\n", __FUNCTION__);
		es_cspin(1);
		udelay(10);	//CS高 10us
		return -EINVAL;
    }

	for(i=0; i<len; i++)
	{
		status = esam_read_status(spi, dev->rx_buf + i + 4);
		if(status)
		{
			printk(KERN_ERR "%s:failed, status = %x\n", __FUNCTION__, status);
			udelay(1);	
			es_cspin(1);
			udelay(10);	//CS高 10us
			return -EIO;
		}
	}
	copy_to_user(reslt + 4, dev->rx_buf + 4, len);

	esam_read_status(spi, &Lrcx);
#else
_waitread:
	dev->rx_buf[0]=ESAM_ReceiveByte();
	//printk("dev->rx_buf[0]=%02x\n",dev->rx_buf[0]);

	if(dev->rx_buf[0]!= 0x55)
	{
		if(++timeout >= 3200*1000/50)
		{
			udelay(1);	
			es_cspin(1);
			udelay(10);	//CS高 10us
			return -EIO;
		}
			
		udelay(50);
		goto _waitread; 
	}  

	udelay(2);		//字节间延迟 2us	

	for(i=0; i<4; i++)
	{
		dev->rx_buf[i]=ESAM_ReceiveByte();
	}

	copy_to_user(reslt, dev->rx_buf, 4);

	//for(i=0;i<4;i++)
	//	printk("dev->rx_buf[%d]=%02x\n",i,dev->rx_buf[i]);

	/* read the data, if exist */
	len = dev->rx_buf[2]*256 + dev->rx_buf[3];
	//printk("len=%d\n",len);

	if(len + 4 > maxlen)
	{
		printk(KERN_ERR "%s: read buffer too small to read!\n", __FUNCTION__);
		es_cspin(1);
		udelay(10); //CS高 10us
		return -EINVAL;
	}

	for(i=0; i<len; i++)
	{
		dev->rx_buf[i+4]=ESAM_ReceiveByte();
	}
	
	copy_to_user(reslt + 4, dev->rx_buf + 4, len);

	dev->rx_buf[len+4]=ESAM_ReceiveByte();
	//printk("dev->rx_buf[%d]=%02x\n",len+4,dev->rx_buf[len+4]);


#endif
	udelay(10);	
	es_cspin(1);
	udelay(10);	//CS高 10us
	CMD_SENT = 0;  //无论是否成功，总是回到初始状态

    if(dev->rx_buf[len+4] == cal_lrc(dev->rx_buf, len + 4))
		return 0;
	else
		return -EIO;
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
ssize_t esam_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct esam_spi_device *dev = filp->private_data;
    int   ret   = 0;

  	if (!dev || (test_bit(0, &dev->is_open) == 0)) {
        printk("esam: write failed:no open.\n");	
		return -EFAULT;       
    }

    ret = esam_spi_write(count, buf);
    if (ret) {
	    printk("esam_write: esam_spi_write failed, %d.\n", ret);	
        return -EIO;
    }

    return count;
}

ssize_t esam_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct esam_spi_device *dev = filp->private_data;
    int   ret   = 0;

  	if (!dev || (test_bit(0, &dev->is_open) == 0)) {
        printk("esam_read: failed for no open.\n");	
		return -EFAULT;       
    }

    ret = esam_spi_read(count, buf);
    if (ret) {
	    printk("esam_read: failed for esam_spi_read. %d\n", ret);	
        return -EIO;
    }

    return (buf[2]*256 + buf[3] + 4);
}
//////////////////////////////////////////////////////////////////////////////////////////////
/*初始化并注册 cdev*/
static void esam_spi_setup_cdev(struct esam_spi_device *dev, int devno)
{
	int err;

	cdev_init(&dev->cdev, &esam_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &esam_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d: adding esam %d!", err, devno);
}

int esam_open(struct inode *inode, struct file *filp)
{
    static struct esam_spi_device *dev;

	dev = esam_spi;
	if (dev && test_and_set_bit(0, &dev->is_open) != 0) {
        return -EBUSY;       
    }
    filp->private_data = dev;

    return 0; 
}

int esam_release(struct inode *inode, struct file *filp)
{
    struct esam_spi_device *dev = filp->private_data; 

	if (dev && test_and_clear_bit(0, &dev->is_open) == 0) {
        return -EINVAL; 
    }

    return 0;
}



static int __init  esam_spi_init(void)
{
    int ret;
	dev_t devno = MKDEV(esam_spi_major, 0);
    struct class * class;

    //断电复位
    es_pwrpin(0);

	nuc970_gpio_core_direction_out1(ESAM_CLK,0);
	nuc970_gpio_core_direction_in1(ESAM_MISO);
	nuc970_gpio_core_direction_out1(ESAM_MOSI,0);

	es_cspin(0);
	msleep(5);	//延迟5毫妙
	//at91_set_A_periph(ESAM_MISO, 0);
	es_cspin(1);
	es_pwrpin(1);	//上电

	
	//at91_set_A_periph(ESAM_MOSI, 0);
    //at91_set_A_periph(ESAM_CLK, 0);

	nuc970_gpio_core_set1(ESAM_MOSI, 1);
	nuc970_gpio_core_set1(ESAM_CLK, 1);
	
	msleep(10);
	HNOS_DEBUG_INFO("Module esam_spi init.\n");

    esam_spi = kmalloc(sizeof(struct esam_spi_device), GFP_KERNEL);
    if (!esam_spi) {
        printk("esam: kmalloc esam_spi failed.\n");
		return -ENOMEM; 
    }
    memset(esam_spi, 0, sizeof(struct esam_spi_device));

    /* 申请设备号 */
	printk("%s: esam_spi_major. \n",__FUNCTION__);
	if (esam_spi_major)
		ret = register_chrdev_region(devno, 1, "esam");
	else /* 动态申请设备号 */
	{
		ret = alloc_chrdev_region(&devno, 0, 1, "esam");
		esam_spi_major = MAJOR(devno);
	}
	if (ret < 0)
	{
		printk("%s:register_chrdev_region failed.\n",__FUNCTION__);
		return ret;
	}

    /* Register a class_device in the sysfs. */
    class = class_create(THIS_MODULE, "esam");
    if (class == NULL)
	{
		printk("%s: class_create failed.\n",__FUNCTION__);
		return -ENOMEM;
	}

	if(!device_create(class, NULL, devno, NULL, "esam"))
	{
		printk("%s: class_device_create failed, %d.\n",__FUNCTION__, ret);
        return -ENOMEM;
	}
    esam_spi->class = class;

	esam_spi_setup_cdev(esam_spi, devno);
    printk("%s:esam_spi_setup_cdev OK! \n",__FUNCTION__);

	return 0;	
}

static void __exit esam_spi_exit(void)
{
    dev_t devno = MKDEV(esam_spi_major, 0);	
    struct class * class;

    cdev_del(&esam_spi->cdev); /*注销 cdev*/
    class = esam_spi->class;
    if (class)
	{
		device_destroy(class, devno);
        class_destroy(class);
    }

	unregister_chrdev_region(MKDEV(esam_spi_major, 0), 1);
	printk("%s: unregister_chrdev_region OK.\n", __FUNCTION__);


	//关电源
    es_pwrpin(0);

	nuc970_gpio_core_set1(ESAM_MOSI, 0);
	//nuc970_gpio_core_set1(ESAM_MISO, 0);
	nuc970_gpio_core_set1(ESAM_CLK, 0);
	
	es_cspin(0); 
	msleep(5);
	HNOS_DEBUG_INFO("Module esam_spi_exit.\n");
    return;
}

module_init(esam_spi_init);
module_exit(esam_spi_exit);

MODULE_LICENSE("Dual BSD/GPL");
