/*
 * drivers/unsigned char/hndl_unsigned char_devices/hnos_fm1702sl.h   
 *
 *
 */
 
#ifndef _HNOS_FM1702SL_H_
#define _HNOS_FM1702SL_H_

#define FM1702SL_DEV_NAME    "FM1702SL"   	//此即为/dev目录下FM1702SL的设备节点
#define GLOBAL_BUFSIZE 	    16
#define TX_BUFSIZE	 	    GLOBAL_BUFSIZE+1	            //因为发送时需要先发送一字节地址
#define RX_BUFSIZE	 	    GLOBAL_BUFSIZE+1	            //因为接收的第一个字节为无效数据
#define INSIDEINIT_TIMEOUT          0x0f
#define SPIINIT_TIMEOUT             0x0f
#define CLEREFIFO_TIMEOUT           0x0f
#define COMMAND_TIMEOUT             0x05
#define IDLE                        0x00
/*************************************************
**  FM1702SL控制寄存器定义
*************************************************/
#define		Page0_Reg		    	0x00
#define		Command_Reg		        0x01
#define		FIFOData_Reg		    0x02
#define		PrimaryStatus_Reg		0x03
#define		FIFOLength_Reg		    0x04
#define		SecondaryStatus_Reg  	0x05
#define		InterruptEn_Reg		    0x06
#define		InterruptRq_Reg		    0x07

#define		Page1_Reg		    	0x08
#define		Control_Reg		        0x09
#define		ErrorFlag_Reg		    0x0A
#define		CollPos_Reg		    	0x0B
#define		TimerValue_Reg		    0x0C
#define		CRCResultLSB_Reg		0x0D
#define		CRCResultMSB_Reg		0x0E
#define		BitFraming_Reg	     	0x0F

#define		Page2_Reg		    	0x10
#define		TxControl_Reg		    0x11
#define		CWConductance_Reg	    0x12
#define		ModWidth_Reg		    0x15

#define		Page3_Reg		    	0x18
#define		RxControl1_Reg		    0x19
#define		DecoderControl_Reg		0x1A
#define		BitPhase_Reg		    0x1B
#define		Rxthreshold_Reg		    0x1C
#define		RxControl2_Reg		    0x1E
#define		ClockQControl_Reg		0x1F

#define		Page4_Reg		    	0x20
#define   	RxWait_Reg            	0x21
#define		ChannelRedundancy_Reg	0x22
#define		CRCPresetLSB_Reg		0x23
#define		CRCPresetMSB_Reg		0x24

#define		Page5_Reg		    	0x28
#define		FIFOLevel_Reg		    0x29
#define		TimerClock_Reg		    0x2A
#define		TimerControl_Reg		0x2B
#define		TimerReload_Reg		    0x2C
#define		IRQPinConfig_Reg		0x2D

#define		Page6_Reg		    	0x30
#define		CryptoSelect_Reg		0x31

/*************************************************
**  FM1702SL命令字
*************************************************/
#define		WriteE2			0x01
#define		LoadKeyE2		0x0B
#define		LoadKey		    0x19
#define   	Transmit      	0x1A
#define		Transceive		0x1E
#define		Authent1		0x0C
#define		Authent2		0x14

/*************************************************
**  Mifare_One卡片命令字
*************************************************/
#define 	REQIDL			0x26               	//寻天线区内未进入休眠状态的卡
#define	 	REQALL			0x52               	//寻天线区内全部卡
#define 	ANTICOLL		0x93               	//防冲撞
#define 	SELECT			0x93               	//选择卡片
#define 	AUTHENTA		0x60               	//验证A密钥
#define 	AUTHENTB		0x61               	//验证B密钥
#define 	READBLOCK		0x30               	//读块
#define 	WRITEBLOCK		0xA0               	//写块
#define 	DECREMENT		0xC0               	//扣款
#define 	INCREMENT		0xC1               	//充值
#define 	PRESTORE		0xC2               	//调块数据到缓冲区
#define 	TRANSFER		0xB0               	//保存缓冲区中数据
#define 	HALT			0x50               	//休眠

/*************************************************
**  FM1702SL函数声明
*************************************************/
int SPIWrite(unsigned char regadr, unsigned char *buffer, unsigned char width);
int SPIRead(unsigned char regadr, unsigned char *buffer, unsigned char width);
int ClearFIFO(void);
int WriteFIFO(unsigned char *ramadr, unsigned char width);
int ReadFIFO(unsigned char *ramadr);
int CommandSend(unsigned char comm, unsigned char *ramadr, unsigned char width);
void Fm1702slReset(void);
int Fm1702slSpiinit(void);
int Fm1702slInit(void);
int Fm1702slStandby(void);
int Fm1702slWakeup(void);

int RequestAll(void);
int RequestIdle(void);
int GetUID(void);
int SelectTag(void);
int WriteKeytoE2(void);
int LoadKeyFromE2(void);
int LoadKeyFromFifo(void);
int AuthenticationA(unsigned char sectornum);
int AuthenticationB(unsigned char sectornum);
int ReadBlock(unsigned char blocknum);                    
int WriteBlock(void);
int Increment(void); 
int Decrement(void);
int Restore(unsigned char blocknum);
int Transfer(unsigned char blocknum);
int Halt(void);
  
int rf_card(void);
int rf_changeb3(unsigned char sector,unsigned char block0,unsigned char block1,
				unsigned char block2,unsigned char block3,unsigned char blockk);
int rf_check_writeA(unsigned char blocknum);
int rf_check_writeB(unsigned char blocknum);				
int rf_HL_initval(unsigned char sector,unsigned char blocknum);
int rf_HL_decrement(unsigned char sector,unsigned char blocknum,unsigned long value);
int rf_HL_increment(unsigned char sector,unsigned char blocknum,unsigned long value);
int rf_HL_write(unsigned char sector, unsigned char blocknum);
int rf_HL_read(unsigned char sector, unsigned char blocknum);
int rf_HL_authentication(unsigned char sector);    
#endif 
