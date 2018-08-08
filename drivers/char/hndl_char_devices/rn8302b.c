/*
 * RN8302B Driver
 *
 * Copyright 2016 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */
//#include <math.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <mach/map.h>
#include <mach/regs-gpio.h>
#include "hnos_debug.h"


#include "hnos_generic.h"
#include "rn8302b.h"
#include "hnos_ioctl.h"


int nuc970_gpio_core_direction_out1(unsigned gpio_num, int val);
int nuc970_gpio_core_set1(unsigned gpio_num,int val);
int nuc970_gpio_core_to_request1(unsigned offset);
int nuc970_gpio_core_direction_in1(unsigned gpio_num);
int nuc970_gpio_core_get1( unsigned gpio_num);





unsigned char  RN8302Status = 0x00;
unsigned char  RN8302Errcum = 0x00;


#define RN8302B_CLK	NUC970_PD4
#define RN8302B_CLK_PN	100

#define RN8302B_SDI	NUC970_PD5
#define RN8302B_SDI_PN	101

#define RN8302B_SDO	NUC970_PD2
#define RN8302B_SDO_PN	98

#define RN8302B_CS	NUC970_PD3
#define RN8302B_CS_PN	99

#define RN8302B_RST NUC970_PD6
#define RN8302B_RST_PN 102


#define RN8302B_MAJOR   40	/* 主设备号	*/
static  int rn8302b_spi_major = RN8302B_MAJOR;

#define P_ADE78XX_SCL_H {register unsigned int t= (*(volatile unsigned int *)REG_GPIOD_DATAOUT);((*(volatile u32 *) (REG_GPIOD_DATAOUT))=t|0x10);}   //pd4
#define P_ADE78XX_SCL_L {register unsigned int t= (*(volatile unsigned int *)REG_GPIOD_DATAOUT);((*(volatile u32 *) (REG_GPIOD_DATAOUT))=t&~(0x10));}//pd4

#define P_ADE78XX_SDO_H {register unsigned int t= (*(volatile unsigned int *)REG_GPIOD_DATAOUT);((*(volatile u32 *) (REG_GPIOD_DATAOUT))=t|0x04);}   //pd2
#define P_ADE78XX_SDO_L {register unsigned int t= (*(volatile unsigned int *)REG_GPIOD_DATAOUT);((*(volatile u32 *) (REG_GPIOD_DATAOUT))=t&~(0x04));}//pd2

#define P_ADE78XX_SDI_Read nuc970_gpio_core_get1(RN8302B_SDI_PN)

#define P_ADE78XX_CS_H {register unsigned int t= (*(volatile unsigned int *)REG_GPIOD_DATAOUT);((*(volatile u32 *) (REG_GPIOD_DATAOUT))=t|0x08);}   //pd3
#define P_ADE78XX_CS_L {register unsigned int t= (*(volatile unsigned int *)REG_GPIOD_DATAOUT);((*(volatile u32 *) (REG_GPIOD_DATAOUT))=t&~(0x08));}//pd3



ssize_t rn8302b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
ssize_t rn8302b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
int rn8302b_open(struct inode *inode, struct file *filp);
int rn8302b_release(struct inode *inode, struct file *filp);





void RN8302_SPI_Init(void)
{
	//CLK
	nuc970_gpio_core_direction_out1(RN8302B_CLK_PN,1);
	//SDO
	nuc970_gpio_core_direction_out1(RN8302B_SDO_PN,1);
	//SDI
	//nuc970_gpio_core_direction_in1(98);
	//CS
 	nuc970_gpio_core_direction_out1(RN8302B_CS_PN,1);
	//RST
	nuc970_gpio_core_direction_out1(RN8302B_RST_PN,1);
	

}

/************************************************************************
* 函数名 ：
* 功能 ：
* 输入参数 ：
* 输出参数 ：
* 返回值说明 ：
* 其他说明 ：
* 修改日期 :
* ------------------------------------------------------------------------
* 2013/06/16 V1.0 yangxing XXXX
**************************************************************************/
void RN8302_Delay( unsigned long n )//5us  --200k
{
    unsigned long i;

    for( i = 0; i < n*20; i++ )
    {
        nop();
    }
}

/************************************************************************
* 函数名 ：
* 功能 ：
* 输入参数 ：
* 输出参数 ：
* 返回值说明 ：
* 其他说明 ：
* 修改日期 :
* ------------------------------------------------------------------------
* 2013/06/16 V1.0 yangxing XXXX
**************************************************************************/
void RN8302_SendByte(unsigned char TxData)
{
    unsigned char i;

  //  WatchDog_Clear;
  //  RN8302_SPI_Init();
    for( i = 0; i < 8; i++ )
    {
        P_ADE78XX_SCL_H;
      //  RN8302_Delay(3);
		RN8302_Delay(10);
        if( (TxData & 0x80) == 0x80 )
        {
            P_ADE78XX_SDO_H;
			
        }
        else
        {
            P_ADE78XX_SDO_L;
        }
      //  RN8302_Delay(3);
		RN8302_Delay(10);
        P_ADE78XX_SCL_L;
       // RN8302_Delay(3);
		RN8302_Delay(10);
        TxData <<= 1;
    }
}

/************************************************************************
* 函数名 ：
* 功能 ：
* 输入参数 ：
* 输出参数 ：
* 返回值说明 ：
* 其他说明 ：
* 修改日期 :
* ------------------------------------------------------------------------
* 2013/06/16 V1.0 yangxing XXXX
**************************************************************************/
unsigned char RN8302_ReceiveByte(void)
{
    unsigned char i,RxData=0;
    
    //WatchDog_Clear;
  //  RN8302_SPI_Init();
    for( i = 0; i < 8; i++ )
    {
        P_ADE78XX_SCL_H;
       // RN8302_Delay(3);
		RN8302_Delay(10);
        P_ADE78XX_SCL_L;
        RxData <<= 1;
       // RN8302_Delay(3);
		RN8302_Delay(10);
        if( P_ADE78XX_SDI_Read )
            RxData |= 0x01;
        RN8302_Delay(3);
    }
    return(RxData);
}

/************************************************************************
* 函数名 ：
* 功能 ：
* 输入参数 ：
* 输出参数 ：
* 返回值说明 ：
* 其他说明 ：
* 修改日期 :
* ------------------------------------------------------------------------
* 2013/06/16 V1.0 yangxing XXXX
**************************************************************************/
unsigned char  RN8302_Read( unsigned char *RAM_Addr, unsigned int RN8302_Addr, unsigned char Lenth )
{
    unsigned char i,Temp=0;
    unsigned char chksum=0;
    
    RN8302Status = 0x01;
    P_ADE78XX_CS_L;
    RN8302_Delay(10);
    Temp = (unsigned char)(RN8302_Addr & 0x00ff);
    chksum = Temp;
    RN8302_SendByte( Temp );
    Temp = (((unsigned char)(RN8302_Addr >> 4)) & 0xf0) + 0x00;
    chksum += Temp;
    RN8302_SendByte( Temp );
    RN8302_Delay(10);
    for( i = Lenth; i > 0; i-- )
    {
        RAM_Addr[i-1] = RN8302_ReceiveByte(); 
        chksum += RAM_Addr[i-1];
    }
    chksum = ~chksum;
    Temp = RN8302_ReceiveByte();   
    if( Temp != chksum )
    {
        RN8302_SendByte( 0x8c );
        chksum = 0x8c;
        Temp = (((unsigned char)(0x018C >> 4)) & 0xf0) + 0x00;
        chksum += Temp;
        RN8302_SendByte( Temp );
        for(i = 4 ; i > 0 ; i--)
        {
            Temp = RN8302_ReceiveByte();
            if(Lenth >= i)
            {					
                if(Temp != RAM_Addr[i-1]) 
                {
                    RAM_Addr[i-1] = Temp;
                }	
            }
            chksum += Temp;
        }
        chksum = ~chksum;
        Temp = RN8302_ReceiveByte();   
        if( Temp != chksum )
        {
            
            for(i = Lenth ; i > 0 ; i--)
            {
                RAM_Addr[i-1] = 0;
            }
            RN8302Status = 0x00;
        }
    }
    P_ADE78XX_CS_H;  
	return RN8302Status;
}

/************************************************************************
* 函数名 ：
* 功能 ：
* 输入参数 ：
* 输出参数 ：
* 返回值说明 ：
* 其他说明 ：
* 修改日期 :
* ------------------------------------------------------------------------
* 2013/06/16 V1.0 yangxing XXXX
**************************************************************************/
unsigned char RN8302_Write( unsigned int RN8302_Addr, unsigned char *RAM_Addr, unsigned char Lenth )
{
    unsigned char i,Temp,Repeat;
    unsigned char chksum=0;
    
    for( Repeat =10; Repeat != 0 ; Repeat--)		
    {
        RN8302Status = 0x01;
        P_ADE78XX_CS_L;
        RN8302_Delay(10);
        Temp = (unsigned char)(RN8302_Addr & 0x00ff);
        chksum = Temp;
        RN8302_SendByte( Temp );
        Temp = (((unsigned char)(RN8302_Addr >> 4)) & 0xf0) + 0x80;
        chksum += Temp;
        RN8302_SendByte( Temp );
        RN8302_Delay(10);
        for(i = Lenth; i > 0;i-- )
        {
            RN8302_SendByte(RAM_Addr[i-1]);
            chksum += RAM_Addr[i-1];
        }
        chksum = ~chksum;
        RN8302_SendByte( chksum );
//-------------------读WData寄存器检查-----------------------
        RN8302_SendByte( 0x8D );
        chksum = 0x8D;
        Temp = (((unsigned char)(0x018D >> 4)) & 0xf0) + 0x00;
        chksum += Temp;
        RN8302_SendByte( Temp );
        RN8302_Delay(10);
        for(i = 3 ; i > 0 ; i--)
        {
            Temp = RN8302_ReceiveByte(); 
            if(Lenth >= i)
            {					
                if(Temp != RAM_Addr[i-1]) 
                {
                    RN8302Status = 0x00;
                    break;	
                }	
            }
            chksum += Temp;
        }
        if(RN8302Status == 0x01) 
        {
            chksum = ~chksum;
            Temp = RN8302_ReceiveByte();   
            if(Temp != chksum)  RN8302Status = 0x00; 
        }
        P_ADE78XX_CS_H;  
        if( RN8302Status == 0x01 )
            break; 
    }
	return RN8302Status;
}


struct file_operations rn8302b_fops =
{
	.read=rn8302b_read,
	.write=rn8302b_write,
//	ioctl:  esam_ioctl,
	.open=rn8302b_open,
	.release=rn8302b_release,
};

//static u8 CMD_SENT  = 0;


static struct rn8302b_spi_device *rn8302b_spi;


ssize_t rn8302b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct rn8302b_spi_device *dev = filp->private_data;
    int   ret   = 0;
	unsigned int RN8302_Addr;
	
  	if (!dev || (test_bit(0, &dev->is_open) == 0)) {
        printk("esam: write failed:no open.\n");	
		return -EFAULT;       
    }
	
	//rn8302b_spi_dump(buf, count, 1);
	//unsigned int RN8302_Addr;
	memcpy( &RN8302_Addr, buf, 2);
    ret = RN8302_Write(RN8302_Addr, buf+2, count-2 );
    if (ret == 0x00) {
	    printk("rn8302b_write: rn8302b_spi_write failed, %d.\n", ret);	
        return -EIO;
    }

    return count;
}

ssize_t rn8302b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct rn8302b_spi_device *dev = filp->private_data;
    int   ret   = 0;
	unsigned int RN8302_Addr=0;

  	if (!dev || (test_bit(0, &dev->is_open) == 0)) {
        printk("rn8302b_read: failed for no open.\n");	
		return -EFAULT;       
    }
	
 //   unsigned int RN8302_Addr=0;
	memcpy( &RN8302_Addr, buf, 2);
   // rn8302b_spi_dump(buf, count, 0);
    ret = RN8302_Read( buf, RN8302_Addr, count );
    if (ret == 0x00) {
	    printk("rn8302b_read: failed for rn8302b_spi_read. %d\n", ret);	
        return -EIO;
    }

    return count;
}
//////////////////////////////////////////////////////////////////////////////////////////////
/*初始化并注册 cdev*/
static void rn8302b_spi_setup_cdev(struct rn8302b_spi_device *dev, int devno)
{
	int err;

	cdev_init(&dev->cdev, &rn8302b_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &rn8302b_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d: adding rn8302b %d!", err, devno);
}

int rn8302b_open(struct inode *inode, struct file *filp)
{
    static struct rn8302b_spi_device *dev;
	
	dev = rn8302b_spi;
	if (dev && test_and_set_bit(0, &dev->is_open) != 0) {
        return -EBUSY;       
    }
    filp->private_data = dev;

    return 0; 
}

int rn8302b_release(struct inode *inode, struct file *filp)
{
    struct rn8302b_spi_device *dev = filp->private_data; 

	if (dev && test_and_clear_bit(0, &dev->is_open) == 0) {
        return -EINVAL; 
    }

    return 0;
}



static int __init  rn8302b_init(void)
{
    int ret;
	dev_t devno = MKDEV(rn8302b_spi_major, 0);
    struct class * class;

	RN8302_SPI_Init();
	
    //断电复位
   // es_pwrpin(0);
	nuc970_gpio_core_set1(RN8302B_RST_PN,0);
	
	mdelay(200);
	//at91_set_gpio_input(RN8302B_SDI, 1);	
	nuc970_gpio_core_direction_in1(RN8302B_SDI_PN);

	//es_pwrpin(1);	//上电
	nuc970_gpio_core_set1(RN8302B_RST_PN,1);
	
	HNOS_DEBUG_INFO("Module rn8302b_spi init.\n");

	

    rn8302b_spi = kmalloc(sizeof(struct rn8302b_spi_device), GFP_KERNEL);
    if (!rn8302b_spi) {
        printk("rn8302b: kmalloc esam_spi failed.\n");
		return -ENOMEM; 
    }
    memset(rn8302b_spi, 0, sizeof(struct rn8302b_spi_device));

    /* 申请设备号 */
	printk("%s: rn8302b_spi_major. \n",__FUNCTION__);
	if (rn8302b_spi_major)
		ret = register_chrdev_region(devno, 1, "rn8302b");
	else /* 动态申请设备号 */
	{
		ret = alloc_chrdev_region(&devno, 0, 1, "rn8302b");
		rn8302b_spi_major = MAJOR(devno);
	}
	if (ret < 0)
	{
		printk("%s:register_chrdev_region failed.\n",__FUNCTION__);
		return ret;
	}

    /* Register a class_device in the sysfs. */
    class = class_create(THIS_MODULE, "rn8302b");
    if (class == NULL)
	{
		printk("%s: class_create failed.\n",__FUNCTION__);
		return -ENOMEM;
	}

	if(!device_create(class, NULL, devno, NULL, "rn8302b"))
	{
		printk("%s: class_device_create failed, %d.\n",__FUNCTION__, ret);
        return -ENOMEM;
	}
    rn8302b_spi->class = class;

	rn8302b_spi_setup_cdev(rn8302b_spi, devno);
    printk("%s:rn8302b_spi_setup_cdev OK! \n",__FUNCTION__);

	return 0;	
}

static void __exit rn8302b_exit(void)
{
    dev_t devno = MKDEV(rn8302b_spi_major, 0);	
    struct class * class;

    cdev_del(&rn8302b_spi->cdev); /*注销 cdev*/
    class = rn8302b_spi->class;
    if (class)
	{
		device_destroy(class, devno);
        class_destroy(class);
    }

	unregister_chrdev_region(MKDEV(rn8302b_spi_major, 0), 1);
	printk("%s: unregister_chrdev_region OK.\n", __FUNCTION__);



	//关电源
 //   es_pwrpin(0);
	msleep(5);
	HNOS_DEBUG_INFO("Module rn8302b_spi_exit.\n");
    return;
}

module_init(rn8302b_init);
module_exit(rn8302b_exit);

MODULE_LICENSE("Dual BSD/GPL");

