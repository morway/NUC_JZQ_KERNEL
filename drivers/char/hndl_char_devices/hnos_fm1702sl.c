/****************************************Copyright (c)**************************************************
**                                  http://www.hnelec.com
**
**--------------File Info-------------------------------------------------------------------------------
** File name:            hnos_fm1702sl.c
** File direct:            ..\linux-2.6.21.1\drivers\char\hndl_char_devices
** Created by:            kernel team
** Last modified Date:  2008-09-18
** Last Version:        1.0
** Descriptions:        The fmcard reader chip fm1702sl is linux2.6 kernel device driver
**
********************************************************************************************************/

#include "hnos_generic.h"
#include "hnos_ioctl.h"                /*所有字符型的设备的IO操作命令都放在这个头文件中*/
#include "hnos_proc.h"
#include "hnos_gpio.h"
#include "hnos_fm1702sl.h"

unsigned char globalbuf[GLOBAL_BUFSIZE]={0};/*全局数组，用于存放临时数据*/
unsigned char Card_UID[5]={0};/*卡的SIN号，前4byte为有效数据，第5个byte为校验*/
unsigned char keybuf[8]={0};/*前2byte为密钥在E2PROM的首地址LSB、MSB，后6byte为密钥*/
unsigned char valuebuf[5]={0};/*第1byte为值所在块号，后4byte为增减的值，低位在前*/

/********************************************************************************************************
**                                  function announce
**                              fm1702sl 的API接口函数定义
********************************************************************************************************/

int fm1702sl_open(struct inode *inode, struct file *filp);
int fm1702sl_release(struct inode *inode, struct file *filp);
//ssize_t fm1702sl_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
//ssize_t fm1702sl_write(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
int fm1702sl_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
                  unsigned long arg);

/*********************************************************************************************************
**                  "全局和静态变量在这里定义"
**        global variables and static variables define here
********************************************************************************************************/

/*设备结构体*/
struct  fm1702sl_device
{
    unsigned long is_open;                                  /*用于原子操作*/
    struct class *myclass;
    struct cdev cdev;
    struct spi_device *spi;
    struct semaphore lock;                                  /*并发控制用的信号量*/
    u8 tx_buf[TX_BUFSIZE];
    u8 rx_buf[RX_BUFSIZE];
};

static struct fm1702sl_device *fm1702sl_devp;      /*设备结构体实例*/
static int fm1702sl_major = 0;                        /*设为0，动态获得主设备号*/
static int fm1702sl_minor = 0;

/*定义本驱动的file_operations结构体*/
struct file_operations fm1702sl_fops =
{
    .owner      = THIS_MODULE,
    .open       = fm1702sl_open,
    .ioctl      = fm1702sl_ioctl,
//    .read       = fm1702sl_read,
//    .write      = fm1702sl_write,
    .release    = fm1702sl_release,
};

/*********************************************************************************
**                                    基本操作函数
*********************************************************************************/

/****************************************************************
**名称: SPIWrite
**功能: 该函数实现写FM1702SL的寄存器
**输入: regadr  寄存器地址[0x01~0x3f]
        buffer  读出数据的存放数组指针
        width   读出数据的字节数
**输出: 成功返回0，失败返回-1
*****************************************************************/

int SPIWrite(unsigned char regadr, unsigned char *buffer, unsigned char width)
{
    struct spi_device *spi = fm1702sl_devp->spi;
    struct spi_message message;
    struct spi_transfer xfer;
    int status = 0;
    unsigned char i, adrtemp;

    if (down_interruptible(&fm1702sl_devp->lock))                   /*互斥锁，防止总线冲突*/
    {
        return - ERESTARTSYS ;
    }

    //down(&fm1702sl_devp->lock);

    adrtemp = regadr;
    if((adrtemp&0xc0) == 0)        /*因为寄存器地址没有超过0x3F的*/
    {
        adrtemp=(adrtemp<<1) & 0x7e;    /*写操作时，地址的最高位及最低位必须为0，中间为地址*/
        fm1702sl_devp->tx_buf[0] = adrtemp;     /*先发送地址*/

        for(i=0; i<width; i++)
        {
            fm1702sl_devp->tx_buf[i+1] = buffer[i];    /*再发送数据*/
        }

        /* Build our spi message */
        spi_message_init(&message);                    /*spi.h中INIT_LIST_HEAD 初始化链表头*/
        memset(&xfer, 0, sizeof(xfer));
        xfer.len = width+1;                            /*发送字节数*/
        xfer.tx_buf = fm1702sl_devp->tx_buf;           /*发送数据的指针*/
        xfer.rx_buf = fm1702sl_devp->rx_buf;           /*接收数据的指针*/

        spi_message_add_tail(&xfer, &message);  /*spi.h中list_add_tail 将子进程添加到运行队列中*/

        /* do the i/o */
        status = spi_sync(spi, &message);              /*spi.c中完成传输*/
        if(status)
        {
            printk("%s: error! spi status=%x\n", __FUNCTION__, status);
        }
        
        up(&fm1702sl_devp->lock);                      /*释放信号量*/
        return status;
    }
    else
    {
        up(&fm1702sl_devp->lock);                      /*释放信号量*/
        return(-1);
    }
}

/****************************************************************
**名称: SPIRead
**功能: 该函数实现读FM1702SL的寄存器
**输入: regadr  寄存器地址[0x01~0x3f]
        buffer  读出数据的存放数组指针
        width   读出数据的字节数
**输出: 成功返回0，失败返回-1
*****************************************************************/

int SPIRead(unsigned char regadr, unsigned char *buffer, unsigned char width)
{
    struct spi_device *spi = fm1702sl_devp->spi;
    struct spi_message message;
    struct spi_transfer xfer;
    int status = 0;
    unsigned char i, adrtemp;

    if (down_interruptible(&fm1702sl_devp->lock))                   /*互斥锁，防止总线冲突*/
    {
        return - ERESTARTSYS ;
    }

    //down(&fm1702sl_devp->lock);
    adrtemp = regadr;
    if((adrtemp&0xc0) == 0)        /*因为寄存器地址没有超过0x3F的*/
    {
        adrtemp = (adrtemp<<1) | 0x80;    /*读操作时，地址的最高位为1、最低位必须为0，中间为地址*/
        fm1702sl_devp->tx_buf[0] = adrtemp;

        for(i=0; i<width; i++)
        {
            if(i != width-1)
            {
                adrtemp = (regadr<<1) | 0x80;
            }
            else
            {
                adrtemp = 0; /*最后要多发送一字节0x00*/
            }
            fm1702sl_devp->tx_buf[i+1] = adrtemp;
        }

        /* Build our spi message */
        spi_message_init(&message);                    /*spi.h中INIT_LIST_HEAD 初始化链表头*/
        memset(&xfer, 0, sizeof(xfer));
        xfer.len = width+1;                            /*发送字节数*/
        xfer.tx_buf = fm1702sl_devp->tx_buf;           /*发送数据的指针*/
        xfer.rx_buf = fm1702sl_devp->rx_buf;           /*接收数据的指针*/

        spi_message_add_tail(&xfer, &message);  /*spi.h中list_add_tail 将子进程添加到运行队列中*/

        /* do the i/o */
        status = spi_sync(spi, &message);              /*spi.c中完成传输*/
        if(status)
        {
            printk("%s: error! spi status=%x\n", __FUNCTION__, status); 
            return(-1);
        }

        memcpy(buffer, &fm1702sl_devp->rx_buf[1], width);/*从第二个字节开始才为有效数据*/
        
        up(&fm1702sl_devp->lock);                      /*释放信号量*/
        return(0);
    }
    else
    {
        up(&fm1702sl_devp->lock);                      /*释放信号量*/
        return(-1);
    }
}

/****************************************************************
**名称: ClearFIFO
**功能: 该函数实现清空FM1702SL中FIFO的数据
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int ClearFIFO(void)
{
    unsigned char acktemp, temp[1];
    unsigned int i;

    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] |= 0x1;
    acktemp = SPIWrite(Control_Reg, temp, 1);  /*控制清空FIFO以及读写指针*/
    if(acktemp)
    {
        return(-1);
    }
    for(i=0; i<CLEREFIFO_TIMEOUT; i++)/*等待FIFO清空完毕*/
    {
        acktemp = SPIRead(FIFOLength_Reg, temp, 1);
        if(acktemp == 0)
        {
            if(temp[0] == 0)
            {
                return(0);
            }
        }
        else
        {
        	return(-1);
        }	
    }
    return(-1);
}

/****************************************************************
**名称: WriteFIFO
**功能: 该函数实现往FM1702SL中的FIFO写数据
**输入: databuf 写入数据的数组指针
        width   写入数据的字节数
**输出: 成功返回0，失败返回-1
*****************************************************************/

int WriteFIFO(unsigned char *databuf, unsigned char width)
{
    unsigned char acktemp;

    acktemp = SPIWrite(FIFOData_Reg, databuf, width);
    if(acktemp)
    {
        return(-1);
    }
   
    return(0);
}

/****************************************************************
**名称: ReadFIFO
**功能: 该函数实现从FM1702SL中的FIFO读数据
**输入: width 从FIFOLength寄存器读出的数据存放地址
**输出: 成功返回0，失败返回-1
*****************************************************************/

int ReadFIFO(unsigned char *width)
{
    unsigned char acktemp;

    acktemp = SPIRead(FIFOLength_Reg, width, 1);  /*读取FIFO中的字节数*/
    if(acktemp)
    {
        return(-1);
    }
    if(width[0] == 0)
    {
        printk("%s:FIFO is empty, can't read\n",  __FUNCTION__);
        return(-1);
    }

    width[0] = (width[0]>GLOBAL_BUFSIZE)?GLOBAL_BUFSIZE:width[0];
    acktemp = SPIRead(FIFOData_Reg, globalbuf, width[0]);
    if(acktemp)
    {
        printk("%s:Read FIFO error!\n", __FUNCTION__);
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: CommandSend
**功能: 该函数实现向FM1702SL发送命令集的功能
**输入: comm    需要发送的FM1702命令
        databuf 命令的参数
        width   参数字节数
**输出: 成功返回0，失败返回-1
*****************************************************************/

int CommandSend(unsigned char comm, unsigned char *databuf, unsigned char width)
{
    unsigned char acktemp, temp[1];
    unsigned int i;
		
    temp[0] = IDLE;
    acktemp = SPIWrite(Command_Reg, temp, 1);
    if(acktemp)
    {		
        return(-1);
    }

    if(width)/*有的命令没有要通过FIFO传递参数和数据*/
    {
        acktemp = ClearFIFO();
        if(acktemp)
        {
            return(-1);
        }
        
        acktemp = WriteFIFO(databuf, width);
        if(acktemp)
        {
            return(-1);
        }
    }

    acktemp = SPIWrite(Command_Reg, &comm, 1);
    if(acktemp)
    {
        return(-1);
    }
    
    /*WriteE2命令只能通过上位机往指令寄存器写Idle命令中止*/
    if(comm == WriteE2)
    {
        return(0);
    }    
        
    for(i=0; i<COMMAND_TIMEOUT; i++)/*等待操作结束，进入空闲状态Idle=0x00*/
    {
        acktemp = SPIRead(Command_Reg, temp, 1);
        if(acktemp == 0)
        {   
            if(temp[0] == IDLE)
            {
                return(0);
            }
        }
        else
        {
        	return(-1);
        }
        
        msleep_interruptible(1);	
    }

    return(-1);
}

/****************************************************************
**名称: Fm1702slReset
**功能: 该函数实现对FM1702SL的重启
**输入: 无
**输出: 无
*****************************************************************/

void Fm1702slReset(void)
{
    at91_set_gpio_output(AT91_PIN_PA13, 1);
		
    msleep_interruptible(10);
		
    at91_set_gpio_value(AT91_PIN_PA13, 0);
		
    msleep_interruptible(10);		
}		
		
/****************************************************************
**名称: Fm1702slSpiinit
**功能: 该函数实现对FM1702SL的SPI进行初始化
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Fm1702slSpiinit(void)
{
    unsigned char acktemp, temp[1];
    unsigned int i;

    for(i=0; i<INSIDEINIT_TIMEOUT; i++)/*等待内部初始化结束，进入空闲*/
    {
        acktemp = SPIRead(Command_Reg, temp, 1);
        if(acktemp)
        {
            return(-1);
        }
        if(temp[0] == IDLE)
        {
            break;
        }
    }         

    if(temp[0])                
    {
        printk("%s:Inside init error!\n", __FUNCTION__);
        return(-1);
    }
    else
    {
        temp[0] = 0x80;
        acktemp = SPIWrite(Page0_Reg, temp, 1);/*初始化SPI接口*/
        if(acktemp)
        {
            return(-1);
        }
        
        for(i=0; i<SPIINIT_TIMEOUT; i++)/*等待SPI初始化结束，进入空闲*/
        {            
            acktemp = SPIRead(Command_Reg, temp, 1);
            if(acktemp)
            {
                return(-1);
            }
            if(temp[0] == IDLE)
            {
                break;
            }
        }    
  
        if(temp[0])                
        {
            printk("%s:FM1702SL SPI init error!\n", __FUNCTION__);
            return(-1);
        }

        temp[0] = 0x00;
        acktemp = SPIWrite(Page0_Reg, temp, 1); /*切换到线性寻址方式*/
        if(acktemp)
        {
            return(-1);
        }

        return(0);
    }
}

/****************************************************************
**名称: Fm1702slInit
**功能: 该函数实现对FM1702SL的各相关寄存器进行初始化
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Fm1702slInit(void)
{
    unsigned char acktemp, temp[1];

    temp[0] = 0x3f;
    acktemp = SPIWrite(InterruptEn_Reg, temp, 1);/*禁止所有中断*/
    if(acktemp)
    {
        return(-1);
    }
    temp[0] = 0x3f;
    acktemp = SPIWrite(InterruptRq_Reg, temp, 1);/*清除所有中断标志*/
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = 0x5b;/*发射器采用内部编码器、TX1TX2输出13.56MHz经发送数据反相调制的能量载波*/
    acktemp = SPIWrite(TxControl_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    temp[0] = 0x1;/*选择Q时钟作为接收器时钟、接收器始终打开、内部解码器*/
    acktemp = SPIWrite(RxControl2_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = 0x7;/*数据发送后，接收器等待7个bit时钟数(1443A帧保护时间为94us)*/
    acktemp = SPIWrite(RxWait_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    return 0 ;
}

/****************************************************************
**名称: FM1702Standby
**功能: 该函数让射频芯片处于休眠模式，以降低系统的功耗
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Fm1702slStandby(void)
{
    unsigned char acktemp, temp[1];
    
    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    
    temp[0] |= 0x20;/*置位Control寄存器的StandBy位*/
    acktemp = SPIWrite(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    
    return(0);
} 

/****************************************************************
**名称: Fm1702slWakeup
**功能: 该函数用于唤醒射频读卡芯片
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Fm1702slWakeup(void)
{
    unsigned char acktemp, temp[1];
    
    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    
    temp[0] &= 0xdf;/*复位Control寄存器的StandBy位*/
    acktemp = SPIWrite(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    
    msleep_interruptible(1);/*需要等待一定时间才能退出StandBy模式*/
    
    return(0);
}

/***********************************************************************************
**                                        RF低级函数
***********************************************************************************/

/****************************************************************
**名称: RequestAll
**功能: 寻天线区内全部卡
**输入: 无
**输出: 成功返回0，失败返回-1
**说明: 寻卡成功后会把卡的类型字写入globalbuf[0]、globalbuf[1]中
*****************************************************************/

int RequestAll(void)
{
    unsigned char acktemp, temp[1];

    temp[0] = 0x7;/*收到的第一个byte的最低位放在FIFO中的第0位、*/
    acktemp = SPIWrite(BitFraming_Reg, temp, 1);/*最后一byte中要发送出去的bit数为7*/
    if(acktemp)
    {
        return(-1);
    }

    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    temp[0] &= 0xf7;            /*关闭加密单元*/
    acktemp = SPIWrite(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*接收过程不进行CRC、不发送CRC、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x3;/*由于TxLastBits不为0，所以必须禁止CRC，否则会CRC校验出错*/
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = REQALL;/*对外发送Request All(0x52)，并接收数据*/
    acktemp = CommandSend(Transceive, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    acktemp = ReadFIFO(temp);/*读取接收到的数据，应该为两个byte ATQA*/
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 0x2)
    {
        return(-1);
    }
    
    return(0);
}

/****************************************************************
**名称: RequestIdle
**功能: 寻天线区内未进入休眠状态的卡
**输入: 无
**输出: 成功返回0，失败返回-1
**说明: 寻卡成功后会把卡的类型字写入globalbuf[0]、globalbuf[1]中
*****************************************************************/

int RequestIdle(void)
{
    unsigned char acktemp, temp[1];

    temp[0] = 0x7;/*收到的第一个byte的最低位放在FIFO中的第0位、*/
    acktemp = SPIWrite(BitFraming_Reg, temp, 1);/*最后一byte中要发送出去的bit数为7*/
    if(acktemp)
    {
        return(-1);
    }

    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    temp[0] &= 0xf7;            /*关闭加密单元*/
    acktemp = SPIWrite(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*接收过程不进行CRC、不发送CRC、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x3;/*由于TxLastBits不为0，所以必须禁止CRC，否则会CRC校验出错*/
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = REQIDL;/*对外发送Request std(0x26)，并接收数据*/
    acktemp = CommandSend(Transceive,temp,1);
    if(acktemp)
    {
        return(-1);
    }
    acktemp = ReadFIFO(temp);/*读取接收到的数据，应该为两个byte ATQA*/
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 0x2)
    {
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: GetUID
**功能: 防冲撞的获得工作范围内某一张卡的序列号
**输入: 无
**输出: 成功返回0，失败返回-1
**说明: 获得的序列号存入Card_UID数组中，前4byte为有效数据，
        第5byte为前4byte的异或校验值
*****************************************************************/

int GetUID(void)
{
    unsigned char acktemp,temp[2],i;

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*接收过程不进行CRC、不发送CRC、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x3;
    acktemp = SPIWrite(ChannelRedundancy_Reg,temp,1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = ANTICOLL; /*防重叠命令*/
    temp[1] = 0x20; /*ARG*/
    acktemp = CommandSend(Transceive,temp,2);
    if(acktemp)
    {
        return(-1);
    }
    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 0x5)/*CT SN0 SN1 SN2 BCC1*/
    {
        return(-1);
    }
    acktemp = 0;
    for(i=0; i<5; i++)
    {
        acktemp ^= globalbuf[i];/*前四个接连异或的结果应该等于第五个数*/
    }
    if(acktemp)
    {
        return(-1);
    }
    for(i=0; i<5; i++)
    {
        Card_UID[i] = globalbuf[i];
    }
    return(0);
}

/****************************************************************
**名称: SelectTag
**功能: 选中特定的一张卡
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int SelectTag(void)
{
    unsigned char acktemp, temp[1], i;

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*对接收和发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0xf;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    globalbuf[0] = SELECT; /*选卡片命令*/
    globalbuf[1] = 0x70; /*ARG*/
    for(i=0; i<5; i++)
    {
        globalbuf[i+2] = Card_UID[i];/*CT SN0 SN1 SN2 BCC1*/
    }
    acktemp = CommandSend(Transceive, globalbuf, 7);
    if(acktemp)
    {
        return(-1);
    }
    acktemp = ReadFIFO(temp);
    if(temp[0] != 0x1)/*判断接收到的字节数是否为1*/
    {
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: WriteKeytoE2
**功能: 把密钥写到E2PROM中
**输入: 无
**输出: 成功返回0，失败返回-1
**说明: 对于初次使用FM1702SL射频芯片，必须要调用该函数将认证时
使用的认证密码装入FM1702SL的EEPROM中
*****************************************************************/

int WriteKeytoE2(void)
{
    unsigned char acktemp, temp[1], i;

    globalbuf[0] = keybuf[0];/*E2PROM起始地址LSB*/
    globalbuf[1] = keybuf[1];/*E2PROM起始地址MSB*/
    for(i=1; i<7; i++)
    {
        globalbuf[i+i]=(((keybuf[i+1]&0xf0)>>4)|((~keybuf[i+1])&0xf0));/*key的格式*/
        globalbuf[1+i+i]=((keybuf[i+1]&0xf)|(~(keybuf[i+1]&0xf)<<4));
    }
    acktemp = CommandSend(WriteE2, globalbuf, 0x0e);/*2byte地址+12byte密钥*/
    if(acktemp)
    {
        return(-1);
    }
    
    msleep_interruptible(4);
    
    acktemp = SPIRead(SecondaryStatus_Reg, temp, 1);/*读取状态寄存器*/
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0x40)/*E2PROM擦除结束*/
    {
        temp[0] = IDLE;
        acktemp = SPIWrite(Command_Reg, temp, 0x1);/*写入空闲命令Idle*/
        if(acktemp)
        {
            return(-1);
        }
        return(0);
    }
    temp[0] = IDLE;
    acktemp = SPIWrite(Command_Reg, temp, 0x1);
    return(-1);
}

/****************************************************************
**名称: LoadKeyFromE2
**功能: 将密钥从E2PROM复制到KEY缓存
**输入: 无
**输出: 成功返回0，失败返回-1
**说明: 如果是第一次需要通过上位机先把密钥写到E2PROM中，
        即调用上面的WriteKeytoE2函数
*****************************************************************/

int LoadKeyFromE2(void)
{
    unsigned char acktemp, temp[2];
    temp[0] = keybuf[0]; /*E2PROM起始地址LSB*/
    temp[1] = keybuf[1];/*E2PROM起始地址MSB*/
    acktemp = CommandSend(LoadKeyE2, temp, 0x2);
    if(acktemp)
    {
        return(-1);
    }
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);/*读取上一条指令的错误标识*/
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0x40)/*输入的数据不符合规定的密钥格式*/
    {
        return(-1);
    }
    return(0);
}

/****************************************************************
**名称: LoadKeyFromFifo
**功能: 将密钥从FIFO复制到KEY缓存
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int LoadKeyFromFifo(void)
{
	unsigned char acktemp, temp[2], i;
    
    for(i=1; i<7; i++)
    {
        globalbuf[i+i]=(((keybuf[i+1]&0xf0)>>4)|((~keybuf[i+1])&0xf0));/*key的格式*/
        globalbuf[1+i+i]=((keybuf[i+1]&0xf)|(~(keybuf[i+1]&0xf)<<4));
    }
    
    acktemp = CommandSend(LoadKey, &globalbuf[2], 12);
    if(acktemp)
    {
        return(-1);
    }
    
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);/*读取上一条指令的错误标识*/
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0x40)/*输入的数据不符合规定的密钥格式*/
    {
        return(-1);
    }
    
    return(0);
}    
    
/****************************************************************
**名称: AuthenticationA
**功能: 验证卡片密码A
**输入: sectornum:扇区号(0~15)
**输出: 成功返回0，失败返回-1
*****************************************************************/

int AuthenticationA(unsigned char sectornum)
{
    unsigned char acktemp, temp[6], i;

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*对接收和发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0xf;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
    return(-1);
    }

    temp[0] = AUTHENTA; /*KEYA卡认证命令*/
    temp[1] = sectornum*4+3;/*这个扇区存放密钥的地址（块号)*/
    for(i=0; i<4; i++)
    {
        temp[2+i] = Card_UID[i];
    }
    acktemp = CommandSend(Authent1, temp, 0x6);/*认证过程第一步*/
    if(acktemp)
    {
        return(-1);
    }
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0xe)/*判断是否出错*/
    {
        return(-1);
    }
    acktemp = CommandSend(Authent2, NULL, 0);/*认证过程第二步*/
    if(acktemp)
    {
        return(-1);
    }
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0xe)/*判断是否出错*/
    {
        return(-1);
    }
    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0x8)/*判断加密单元是否已经打开*/
    {
        return(0);
    }
    return(-1);
}

/****************************************************************
**名称: AuthenticationB
**功能: 验证卡片密码B
**输入: sectornum:扇区号(0~15)
**输出: 成功返回0，失败返回-1
*****************************************************************/

int AuthenticationB(unsigned char sectornum)
{
    unsigned char acktemp, temp[6], i;

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*对接收和发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0xf;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = AUTHENTB; /*KEYB卡认证命令*/
    temp[1] = sectornum*4+3;/*这个扇区存放密钥的地址（块号)*/
    for(i=0; i<4; i++)
    {
        temp[2+i] = Card_UID[i];
    }
    acktemp = CommandSend(Authent1, temp, 0x6);/*认证过程第一步*/
    if(acktemp)
    {
        return(-1);
    }
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] & 0xe)/*判断是否出错*/
    {
        return(-1);
    }
    acktemp = CommandSend(Authent2, NULL, 0);/*认证过程第二步*/
    if(acktemp)
    {
        return(-1);
    }
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0]&0xe)/*判断是否出错*/
    {
        return(-1);
    }
    acktemp = SPIRead(Control_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0]&0x8)/*判断加密单元是否已经打开*/
    {
        return(0);
    }
    return(-1);
}

/****************************************************************
**名称: ReadBlock
**功能: 读取M1卡一块数据
**输入: blocknum:块地址
**输出: 成功返回0，失败返回-1
**说明：返回数据存放globalbuf[]中
*****************************************************************/

int ReadBlock(unsigned char blocknum)
{
    unsigned char acktemp, temp[2];

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*对接收和发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0xf;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = READBLOCK;/*读命令*/
    temp[1] = blocknum;/*块号*/
    acktemp = CommandSend(Transceive, temp, 2);
    if(acktemp)
    {
        return(-1);
    }
    acktemp = SPIRead(ErrorFlag_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0]&0xe)/*判断是否出错*/
    {
        return(-1);
    }
    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 16)/*读取了16个字节*/
    {
        return(-1);
    }
    return(0);
}

/****************************************************************
**名称: WriteBlock
**功能: 读取M1卡一块数据
**输入: blocknum:块地址
**输出: 成功返回0，失败返回-1
**说明：写数据globalbuf[]到指定块
*****************************************************************/

int WriteBlock(void)
{
    unsigned char acktemp, temp[2];

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*只对发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x7;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = WRITEBLOCK;/*写命令*/
    temp[1] = globalbuf[0];/*块号*/
    acktemp = CommandSend(Transceive, temp, 2);
    if(acktemp)
    {
        return(-1);
    }

    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 1)
    {
        return(-1);
    }
    if(globalbuf[0] != 0xa)
    {
        return(-1);
    }

    acktemp = CommandSend(Transceive, globalbuf+1, 0x10);/*发送16byte数据*/
    if(acktemp)
    {
        return(-1);
    }

    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 1)
    {
        return(-1);
    }
    if(globalbuf[0] != 0xa)
    {
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: Increment
**功能: 该函数实现对值操作的块进行增值操作
**输入: blocknum:值操作的块地址
**输出: 成功返回0，失败返回-1
**说明：valuebuf中为加的值，低字节在前
*****************************************************************/

int Increment(void)
{
    unsigned char acktemp, temp[2];

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*对接收和发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x7;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = INCREMENT; /*增值命令*/
    temp[1] = valuebuf[0];/*块号*/
    acktemp = CommandSend(Transceive, temp, 2);
    if(acktemp)
    {
        return(-1);
    }

    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 1)
    {
        return(-1);
    }
    if(globalbuf[0] != 0xa)
    {
        return(-1);
    }

    acktemp = CommandSend(Transmit, valuebuf+1, 4);/*发送4byte加的值，低字节在前*/
    if(acktemp)
    {
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: Decrement
**功能: 该函数实现对值操作的块进行减值操作
**输入: blocknum:值操作的块地址
**输出: 成功返回0，失败返回-1
**说明：valuebuf中为加的值，低字节在前
*****************************************************************/

int Decrement(void)
{
    unsigned char acktemp, temp[2];

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*只对发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x7;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = DECREMENT;/*减值命令*/
    temp[1] = valuebuf[0];/*块号*/
    acktemp = CommandSend(Transceive, temp, 2);
    if(acktemp)
    {
        return(-1);
    }

    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 1)
    {
        return(-1);
    }
    if(globalbuf[0] != 0xa)
    {
        return(-1);
    }

    acktemp = CommandSend(Transmit, valuebuf+1, 4);/*发送4byte减的值，低字节在前*/
    if(acktemp)
    {
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: Restore
**功能: 该函数实现MIFARE卡自动恢复,备份操
**输入: blocknum:卡片上将读出数据的块地址
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Restore(unsigned char blocknum)
{
    unsigned char acktemp, temp[2], i;

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*只对发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x7;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = PRESTORE;/*重储命令*/
    temp[1] = blocknum;/*块号*/
    acktemp = CommandSend(Transceive, temp, 2);
    if(acktemp)
    {
        return(-1);
    }

    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 1)
    {
        return(-1);
    }
    if(globalbuf[0] != 0xa)
    {
        return(-1);
    }

    for(i=0; i<4; i++) 
    {
        globalbuf[i] = 0x00;
    }
    
    acktemp = CommandSend(Transmit, globalbuf, 4);
    if(acktemp)
    {
        return(-1);
    }
    
    return(0);
}

/****************************************************************
**名称: Transfer
**功能: 该函数实现MIFARE卡电子钱包保存操作
**输入: blocknum:内部寄存器的内容将存放的地址
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Transfer(unsigned char blocknum)
{
    unsigned char acktemp, temp[2];

    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*只对发送的数据进行CRC校验、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x7;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = TRANSFER;/*发送命令*/
    temp[1] = blocknum;/*块号*/
    acktemp = CommandSend(Transceive, temp, 2);
    if(acktemp)
    {
        return(-1);
    }

    acktemp = ReadFIFO(temp);
    if(acktemp)
    {
        return(-1);
    }
    if(temp[0] != 1)
    {
        return(-1);
    }
    if(globalbuf[0] != 0xa)
    {
        return(-1);
    }

    return(0);
}

/****************************************************************
**名称: Halt
**功能: 该函数实现暂停MIFARE卡
**输入: 无
**输出: 成功返回0，失败返回-1
*****************************************************************/

int Halt(void)
{
    unsigned char acktemp, temp[2];

    /*temp[0] = 0x63;
    acktemp = SPIWrite(CRCPresetLSB_Reg, temp, 1);
    if(acktemp)
    {		
        return(-1);
    }
    
    temp[0] = 0x3f;
    acktemp = SPIWrite(CWConductance_Reg, temp, 1);
    if(acktemp)
    {		
        return(-1);
    }*/
    
    /*数据流低位先进CRC协处理器、CRC算法为ISO1443A、计算16bitCRC、*/
    /*接收过程不进行CRC、不发送CRC、*/
    /*奇校验、发送和接收都进行奇偶校验*/
    temp[0] = 0x7;
    acktemp = SPIWrite(ChannelRedundancy_Reg, temp, 1);
    if(acktemp)
    {
        return(-1);
    }

    temp[0] = HALT;/*停机命令*/
    temp[1] = 0x00;
    acktemp = CommandSend(Transmit, temp, 2);
    if(acktemp)
    {
        return(-1);
    }
    
    msleep_interruptible(5);

    return(0);
}       
    
#if 0
/*************************************************************************
**                            RF高级函数
*************************************************************************/
int rf_card(request)
{
    unsigned char acktemp;

    if(request)
    {
        acktemp = RequestAll();
    }
    else
    {
        acktemp = RequestStd();
    }

    if(acktemp)
    {
        return(-1);
    }

    acktemp = GetUID();
    if(acktemp)
    {
        return(-1);
    }

    acktemp = SelectTag();
    if(acktemp)
    {
        return(-1);
    }

    return(0);
}
#endif

/*********************************************************************************************************
** Function name: fm1702sl_cdev_setup
** Descriptions:  Create a cdev for fm1702sl
** Input: *dev:   fm1702sl's device struct pointer
**        devno:  fm1702sl's device number
** Output :
********************************************************************************************************/

static void  fm1702sl_cdev_setup(struct fm1702sl_device *dev, dev_t devno)
{
    int err;

    cdev_init(&dev->cdev, &fm1702sl_fops);        /*初始化cdev结构*/

    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &fm1702sl_fops;
    err = cdev_add(&dev->cdev, devno, 1);        /*将字符设备加入到内核的字符设备数组中（chrdev[]）*/

    if(err != 0)
    {
        printk(KERN_NOTICE "Error %d adding fm1702sl device, major_%d.\n", err, MAJOR(devno));
    }

    return;
}

/*********************************************************************************************************
** Function name: fm1702sl_open
** Descriptions:  fm1702sl open function
                  App layer will invoke this interface to open fm1702sl device, just set a
                  flag which comes from device struct.
** Input:inode:   information of device
**       filp:    pointer of file
** Output 0:      OK
**        other:  not OK
********************************************************************************************************/

int fm1702sl_open(struct inode *inode, struct file *filp)
{
    static struct fm1702sl_device *dev;

    dev = container_of(inode->i_cdev, struct fm1702sl_device, cdev); /*得到包含某个结构成员的结构的指针*/
    filp->private_data = dev; /* for other methods */                  /*将设备结构体指针赋值给文件私有数据指针*/

    if(test_and_set_bit(0, &dev->is_open) != 0)                        /*设置某一位并返回该位原来的值*/
    {
        return -EBUSY;
    }

    return(0); /* success. */
}

/*********************************************************************************************************
** Function name: fm1702sl_release
** Descriptions:  fm1702sl release function
                  App layer will invoke this interface to release fm1702sl device, just clear a
                  flag which comes from device struct.
** Input:inode:   information of device
**       filp:    pointer of file
** Output 0:      OK
**        other:  not OK
********************************************************************************************************/

int fm1702sl_release(struct inode *inode, struct file *filp)
{
    struct fm1702sl_device *dev = filp->private_data;                /*获得设备结构体指针*/

    if(test_and_clear_bit(0, &dev->is_open) == 0)   /* release lock, and check... */
    {
        return -EINVAL;     /* already released: error */
    }

    return(0);
}

/*********************************************************************************************************
** Function name: fm1702sl_ioctl
** Descriptions:  fm1702sl ioctl function
                  App layer will invoke this interface to control fm1702sl chip
** Input:inode:   information of device
**       filp:    pointer of file
**       cmd:     command
**       arg:     additive parameter
** Output 0:      OK
**        other:  not OK
********************************************************************************************************/

int fm1702sl_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret=0;
    unsigned char temp;

    /*对命令参数进行容错处理*/
    if (_IOC_TYPE(cmd) != HNDL_AT91_IOC_MAGIC)          /*判断是否属于该设备*/
    {
        return -ENOTTY;
    }

    if (_IOC_NR(cmd) > HNDL_AT91_IOC_MAXNR )            /*判断是否超过了最大命令数*/
    {
        return -ENOTTY;
    }

    switch(cmd)                                            /*根据不同的参数cmd发送不同的命令*/
    {
        case IOCTL_FM1702SL_RESET:
            Fm1702slReset();
            break;
        
        case IOCTL_FM1702SL_SPIINIT:
            ret = Fm1702slSpiinit();
            break;

        case IOCTL_FM1702SL_INIT:
            ret = Fm1702slInit();
            break;

        case IOCTL_FM1702SL_REQUESTALL:
            ret = RequestAll();
            if(ret)
            {
                return -1;
            }
            
            ret = copy_to_user((unsigned char *)arg, globalbuf, 2);
            if(ret)
            {
                return -EFAULT;
            }

            break;

        case IOCTL_FM1702SL_REQUESTIDLE:
            ret = RequestIdle();
            if(ret)
            {
                return -1;
            }
            
            ret = copy_to_user((unsigned char *)arg, globalbuf, 2);
            if(ret)
            {
                return -EFAULT;
            }

            break;

        case IOCTL_FM1702SL_GETUID:
            ret = GetUID();
            break;

        case IOCTL_FM1702SL_SELECTTAG:
            ret = SelectTag();
            if(ret)
            {
                return(-1);
            }
            ret = put_user(globalbuf[0], (unsigned char *)arg);
            if(ret)
            {
                return -EFAULT;
            }
            break;

        case IOCTL_FM1702SL_WRITEKEYTOE2:
            ret = copy_from_user(keybuf, (unsigned char *)arg, 8);
            if(ret)
            {
                return -EFAULT;
            } 
 
            ret = WriteKeytoE2();

            break;

        case IOCTL_FM1702SL_LOADKEYE2:
            ret = copy_from_user(keybuf, (unsigned char *)arg, 2);
            if(ret)
            {
                return -EFAULT;
            } 

            ret = LoadKeyFromE2();
            if(ret)
            {
                return -1;
            }
            break;

	    case IOCTL_FM1702SL_LOADKEYFIFO:
            ret = copy_from_user(&keybuf[2], (unsigned char *)arg, 6);
            if(ret)
            {
                return -EFAULT;
            }
 
            ret = LoadKeyFromFifo();

            break;

        case IOCTL_FM1702SL_AUTHENTICATIONA:
            temp = arg & 0xff;
            
            ret = AuthenticationA(temp);

            break;

         case IOCTL_FM1702SL_AUTHENTICATIONB:
            temp = arg & 0xff;
            
            ret = AuthenticationB(temp);
            
            break;

        case IOCTL_FM1702SL_WRITEBLOCK:
            ret = copy_from_user(globalbuf, (unsigned char *)arg, 17);/*第1个byte放写入的blocknum*/
            if(ret)
            {
                return -EFAULT;
            }

            ret = WriteBlock();
            
            break;

        case IOCTL_FM1702SL_READBLOCK:
            if (get_user(temp, (unsigned char *)arg))/*第1个byte放需要读的blocknum*/
            {
                return -EFAULT;
            }
            
            ret = ReadBlock(temp);
            if(ret)
            {
                return -1;
            }
            
            ret = copy_to_user((unsigned char *)(arg+1), globalbuf, 16);/*读出的16byte数据放在后面的用户空间*/
            if(ret)
            {
                return -EFAULT;
            }

            break;

        case IOCTL_FM1702SL_INCREMENT:
            ret = copy_from_user(valuebuf, (unsigned char *)arg, 5);/*第1个byte放写入的blocknum*/
            if(ret)
            {
                return -EFAULT;
            }
  
            ret = Increment();
            
            break;

        case IOCTL_FM1702SL_DECREMENT:
            ret = copy_from_user(valuebuf, (unsigned char *)arg, 5);/*第1个byte放写入的blocknum*/
            if(ret)
            {
                return -EFAULT;
            }
 
            ret = Decrement();
            
            break;

        case IOCTL_FM1702SL_RESTORE:
            temp = arg & 0xff;
                
            ret = Restore(temp);
                       
            break;

        case IOCTL_FM1702SL_TRANSFER:
            temp = arg & 0xff;
                
            ret = Transfer(temp);
            
            break;

        case IOCTL_FM1702SL_HALT:
            ret = Halt();
            break;
            
        case IOCTL_FM1702SL_STANDBY:
            ret = Fm1702slStandby();
            break;  

        case IOCTL_FM1702SL_WAKEUP:
            ret = Fm1702slWakeup();
            break; 
            
        default:
            return -ENOTTY;
    }

    return ret;
}

/*********************************************************************************************************
** Function name: fm1702sl_remove
** Descriptions:  fm1702sl remove function
                  SPI core will invoke this interface to remove fm1702sl
** Input: *spi    spi core device struct pointer
** Output 0:      OK
**        other:  not OK
********************************************************************************************************/

static int  fm1702sl_remove(struct spi_device *spi)
{
    dev_t devno = MKDEV(fm1702sl_major, fm1702sl_minor);  /*将主设备号和从设备号组合成设备号*/
    struct class* myclass;

    if(fm1702sl_devp != 0)
    {
        /* Get rid of our char dev entries */
        cdev_del(&fm1702sl_devp->cdev);                          /*从系统中移除一个字符设备*/

        myclass = fm1702sl_devp->myclass;
        if(myclass != 0)
        {
            class_device_destroy(myclass, devno);                /*销毁一个类设备*/
            class_destroy(myclass);                                /*销毁一个类*/
        }

        kfree(fm1702sl_devp);                                                   /*释放fm1702sl_probe函数中申请的内存*/
        fm1702sl_devp = NULL;
    }

    /* cleanup_module is never called if registering failed */
    unregister_chrdev_region(devno, 1);                    /*释放分配的一系列设备号*/
    return(0);
}

/*********************************************************************************************************
** Function name: fm1702sl_probe
** Descriptions:  fm1702sl probe function
                  SPI core will invoke this probe interface to init fm1702sl.
** Input: *spi    spi core device struct pointer
** Output 0:      OK
**        other:  not OK
********************************************************************************************************/

static int __devinit fm1702sl_probe(struct spi_device *spi)
{
    int result  = 0;
    dev_t dev   = 0;
    struct class* myclass;

     /*
     * Get a range of minor numbers to work with, asking for a dynamic
     * major unless directed otherwise at load time.
     */
    if(fm1702sl_major)            /*知道主设备号*/
    {
        dev = MKDEV(fm1702sl_major, fm1702sl_minor);
        result = register_chrdev_region(dev, 1, FM1702SL_DEV_NAME);
    }
    else                        /*不知道主设备号，动态分配*/
    {
        result = alloc_chrdev_region(&dev, fm1702sl_minor, 1, FM1702SL_DEV_NAME);
        fm1702sl_major = MAJOR(dev);

    }

    if(result < 0)
    {
        printk(KERN_WARNING "hndl_kb: can't get major %d\n", fm1702sl_major);
        return result;
    }

    /* allocate the devices -- we do not have them static. */
    fm1702sl_devp = kmalloc(sizeof(struct fm1702sl_device), GFP_KERNEL);   /*为fm1702sl设备结构体申请内存*/
    if(!fm1702sl_devp)
    {
        /* Can not malloc memory for fm1702sl */
        printk("fm1702sl Error: Can not malloc memory\n");
        fm1702sl_remove(spi);
        return -ENOMEM;
    }
    memset(fm1702sl_devp, 0, sizeof(struct fm1702sl_device));

    init_MUTEX(&fm1702sl_devp->lock);                                        /*初始化信号量*/

    /* Register a class_device in the sysfs. */
    myclass = class_create(THIS_MODULE, FM1702SL_DEV_NAME);             /*class.c中，在系统中建立一个类*/
    if(NULL == myclass)
    {
        printk("fm1702sl Error: Can not create class\n");
        fm1702sl_remove(spi);
        return result;
    }

    class_device_create(myclass, NULL, dev, NULL, FM1702SL_DEV_NAME);

    fm1702sl_devp->myclass = myclass;

    fm1702sl_cdev_setup(fm1702sl_devp, dev);

    fm1702sl_devp->spi = spi;

    HNOS_DEBUG_INFO("Initialized device %s, major %d.\n", FM1702SL_DEV_NAME, fm1702sl_major);

    return(0);
}

/*fm1702sl的驱动链接到spi 核心驱动*/
static struct spi_driver fm1702sl_driver = {
    .driver = {
        .name   = "FM1702SL",
        .owner  = THIS_MODULE,
    },
    .probe  = fm1702sl_probe,
    .remove = __devexit_p(fm1702sl_remove),
};

/*********************************************************************************************************
** Function name: Fm1702slInit
** Descriptions:  fm1702sl init function
                  register fm1702sl driver to SPI core
** Input:none
** Output 0:      OK
**        other:  not OK
********************************************************************************************************/

static int __init fm1702sl_init(void)
{
    int ret;

    ret = spi_register_driver(&fm1702sl_driver);                 /*spi.c中，driver_register*/
		
    return ret;
}

/*********************************************************************************************************
** Function name: fm1702sl_exit
** Descriptions:  fm1702sl exit function
                  unregister fm1702sl driver from SPI core
** Input:none
** Output none
********************************************************************************************************/

static void __exit fm1702sl_exit(void)
{
    spi_unregister_driver(&fm1702sl_driver);                     /*spi.h中，driver_unregister*/

    return;
}

module_init(fm1702sl_init);
module_exit(fm1702sl_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("kernel team");
