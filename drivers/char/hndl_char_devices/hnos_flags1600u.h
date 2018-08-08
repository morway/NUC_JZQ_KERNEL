#ifndef __HNOS_FLAGS1600U_H
#define __HNOS_FLAGS1600U_H

/* add by dq, IO for PLC */
//090805 #define IOC_PLC_POWERON     _IOW(HNDL_AT91_IOC_MAGIC, 50, int)
//090805 #define IOC_PLC_POWEROFF    _IOW(HNDL_AT91_IOC_MAGIC, 51, int)
#define IOC_PLC_RESET       _IOW(HNDL_AT91_IOC_MAGIC, 52, int)
#define IOC_PLC_A           _IOW(HNDL_AT91_IOC_MAGIC, 53, int)
#define IOC_PLC_B           _IOW(HNDL_AT91_IOC_MAGIC, 54, int)
#define IOC_PLC_C           _IOW(HNDL_AT91_IOC_MAGIC, 55, int)

/* add by dq, IO for BATTERY */
#define IOC_BATTERY_CHARGE_ENABLE   _IOW(HNDL_AT91_IOC_MAGIC, 60, int)
#define IOC_BATTERY_CHARGE_DISABLE  _IOW(HNDL_AT91_IOC_MAGIC, 61, int)
#define IOC_BATTERY_SUPPLY_ENABLE   _IOW(HNDL_AT91_IOC_MAGIC, 62, int)
#define IOC_BATTERY_SUPPLY_DISABLE  _IOW(HNDL_AT91_IOC_MAGIC, 63, int)
#define IOC_BATTERY_STATE           _IOR(HNDL_AT91_IOC_MAGIC, 64, int)

/* add by dq, IO for Product IO */
#define IOC_PIO_BIT0        _IOW(HNDL_AT91_IOC_MAGIC, 70, int)
#define IOC_PIO_BIT1        _IOW(HNDL_AT91_IOC_MAGIC, 71, int)
#define IOC_PIO_BIT2        _IOW(HNDL_AT91_IOC_MAGIC, 72, int)
#define IOC_PIO_BIT3        _IOW(HNDL_AT91_IOC_MAGIC, 73, int)
#define IOC_PIO_BIT4        _IOW(HNDL_AT91_IOC_MAGIC, 74, int)
#define IOC_PIO_BIT5        _IOW(HNDL_AT91_IOC_MAGIC, 75, int)
#define IOC_PIO_BIT6        _IOW(HNDL_AT91_IOC_MAGIC, 76, int)
#define IOC_PIO_BIT7        _IOW(HNDL_AT91_IOC_MAGIC, 77, int)

#endif
