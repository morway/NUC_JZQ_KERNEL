/*
 * drivers/char/hndl_char_devices/hnos_defines_hntt1800x.h 
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
#ifndef __HNOS_DEFINES_HNDL1000X_H 
#define __HNOS_DEFINES_HNDL1000X_H 

#define BASE_CS                 UL(0x30000000)
#define MAX_CS                  UL(0x300000e0)

#define BUSMEM_NUM              8


#define ID_CS1                  UL(0x30000000)
#define ID_CS1_INDEX            0
#define	STATE0		            (1 << 0)        //state0-5 gprs form detect
#define	STATE1		            (1 << 1)
#define	STATE2		            (1 << 2)
#define	STATE3		            (1 << 3)
#define	STATE4		            (1 << 4)
#define	STATE5		            (1 << 5)
#define	LID_EDTECT	            (1 << 6)        // tail-lid detect
#define	DOOR_DETECT	            (1 << 7)        // door detect


#define LCD_CS                  UL(0x30000020)
#define LCD_CS_INDEX            1


#define KEY_CS                  UL(0x30000040)
#define KEY_CS_INDEX            2
#define	KEY1_ES		            (1 << 0)
#define	KEY2_EN		            (1 << 1)
#define	KEY1_UP		            (1 << 2)
#define	KEY1_DOWN	            (1 << 3)
#define	KEY1_LEFT	            (1 << 4)
#define	AT91_RIGHT	            (1 << 5)


#define PULSE_CS2               UL(0x30000060)
#define PULSE_CS2_INDEX         3

#if 0
#define RELAY_CS1               UL(0x30000080)
#define RELAY_CS1_INDEX         4
#define	RELAYB0		            (1 << 0)        //relay0 B
#define	RELAYB1		            (1 << 1)        //relay1 B
#define	RELAYB2		            (1 << 2)        //relay2 B
#define	RELAYB3		            (1 << 3)        //relay3 B
#define	LEDBD		            (1 << 5)        //led     
#define	LEDGK	                (1 << 6)        //led
#define	LEDDK	                (1 << 7)        //led
#endif

//负荷控制灯控
#define RELAY_CS               UL(0x30000080)
#define RELAY_CS_INDEX         6

#define	FKLED1		            (1 << 7)        //relay0 B
#define	FKLED2		            (1 << 6)        //relay1 B
#define	FKLED3		            (1 << 5)        //relay2 B
#define	FKLED4		            (1 << 4)        //relay3 B
#define	ELECLED		            (1 << 3)        //relay3 B
#define	POWERLED		          (1 << 2)        //led     
#define	RUNLED	              (1 << 1)        //led
#define	YKWARN	              (1 << 0)        //led


#define RELAY_CS2               UL(0x300000a0)
#define RELAY_CS2_INDEX         5
#define	RELAYA0		            (1 << 0)        //relay0 A
#define	RELAYA1		            (1 << 1)        //relay1 a
#define	RELAYA2		            (1 << 2)        //relay2 a
#define	RELAYA3		            (1 << 3)        //relay3 a
#define	RELAYA4		            (1 << 4)        //relay4 a
#define	RELAYA_PWR_CTRL		    (1 << 7)        //relay3 b


#define GPRS_LCD_BEEP_CS        UL(0x300000c0)
#define GPRS_LCD_BEEP_CS_INDEX  6
#define	TPWR_CTL	            (1 << 0)        //gprs power
#define	OUTPUT0		            (1 << 1)        //gprs start
#define	NSFOTRESET	            (1 << 2)        //gprs soft reset
#define	BEEP		            (1 << 3)        //beep
#define	FAULT		            (1 << 4)        //fault led
#define	BLCD_BL_EN	            (1 << 5)        //lcd back power
#define	BLCD_NREST	            (1 << 6)        //lcd reset
#define	BRST7022	            (1 << 7)        //7022 reset

#define PULSE_CS1               UL(0x300000e0)
#define PULSE_CS1_INDEX          7


//有功输入输出
#define CF_VARCF                1
#define CF_IN_PIO               AT91_PIN_PA7
#define CF_OUT_PIO              AT91_PIN_PA20
//无功输入输出
#define VARCF_IN_PIO            AT91_PIN_PA8
#define VARCF_OUT_PIO           AT91_PIN_PA21

#define SIG_HIGH                1
#define SIG_LOW                 0
#define DETECT_POWER_OFF        1
#define DETECT_POWER_ON         0

#define DETECT_MODULE_OFF       1        //没有插入摇控摇信模块
#define DETECT_MODULE_ON        0
#define SCAN_FIRST              1
#define SCAN_SECOND             2

#define MEASURE_DEVICE_NO       0
#define MEASURE_DEVICE_7022     1
#define MEASURE_DEVICE_6513     2

#define LED_BD	                WORK_LED
#define LED_GK	                METER_LED
#define LED_DK	                NET_LED
#define LED_FAULT               RSSI_LED
#define LED_NUM                 5



#undef HW_DEBUG_OPT    

int vcc5v_poweroff(void);
int hnos_IsPowerOffNow(void);
int hnos_module_state(void);
void rmi_detect_opt(u8 scan_turn);
void smcbus_auto_write(void);
void kbd_hndl1000x_scan(void);
int hnos_at24c02_wr_state(void);
void parallel2serail_led_autoscan(void);
int att7022_work_of(void);

#endif
