/*
 * drivers/char/hndl_char_devices/hnos_bat_adc.h   
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
#ifndef		HNOS_BAT_ADC_H
#define		HNOS_BAT_ADC_H

#define ADC_RESLT_INVALID		-1
#define ADC_CH_BATTERY			1  	// AD1 用于充电电池电压检测
#define ADC_CH_VCC5V			2	// AD2 用于交流供电5V电压检测
#define ADC_CH_RTC				3	// AD3 用于时钟电池电压检测

int bat_voltage_get_hex(struct proc_item *item, char *page);
int bat_voltage_get(struct proc_item *item, char *page);
int vcc5v_voltage_get(struct proc_item *item, char *page);
int rtc_voltage_get(struct proc_item *item, char *page);

#endif
