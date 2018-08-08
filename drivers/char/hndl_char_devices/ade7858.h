#ifndef _ADE7858_H
#define _ADE7858_H
#include "atmel_spi.h"
#define ADE7858_AIGAIN    0x4380
#define ADE7858_AVGAIN    0x4381
#define ADE7858_BIGAIN    0x4382
#define ADE7858_BVGAIN    0x4383
#define ADE7858_CIGAIN    0x4384
#define ADE7858_CVGAIN    0x4385
#define ADE7858_NIGAIN    0x4386
#define ADE7858_AIRMSOS   0x4387
#define ADE7858_AVRMSOS   0x4388
#define ADE7858_BIRMSOS   0x4389
#define ADE7858_BVRMSOS   0x438A
#define ADE7858_CIRMSOS   0x438B
#define ADE7858_CVRMSOS   0x438C
#define ADE7858_NIRMSOS   0x438D
#define ADE7858_AVAGAIN   0x438E
#define ADE7858_BVAGAIN   0x438F
#define ADE7858_CVAGAIN   0x4390
#define ADE7858_AWGAIN    0x4391
#define ADE7858_AWATTOS   0x4392
#define ADE7858_BWGAIN    0x4393
#define ADE7858_BWATTOS   0x4394
#define ADE7858_CWGAIN    0x4395
#define ADE7858_CWATTOS   0x4396
#define ADE7858_AVARGAIN  0x4397
#define ADE7858_AVAROS    0x4398
#define ADE7858_BVARGAIN  0x4399
#define ADE7858_BVAROS    0x439A
#define ADE7858_CVARGAIN  0x439B
#define ADE7858_CVAROS    0x439C
#define ADE7858_AFWGAIN   0x439D
#define ADE7858_AFWATTOS  0x439E
#define ADE7858_BFWGAIN   0x439F
#define ADE7858_BFWATTOS  0x43A0
#define ADE7858_CFWGAIN   0x43A1
#define ADE7858_CFWATTOS  0x43A2
#define ADE7858_AFVARGAIN 0x43A3
#define ADE7858_AFVAROS   0x43A4
#define ADE7858_BFVARGAIN 0x43A5
#define ADE7858_BFVAROS   0x43A6
#define ADE7858_CFVARGAIN 0x43A7
#define ADE7858_CFVAROS   0x43A8
#define ADE7858_VATHR1    0x43A9
#define ADE7858_VATHR0    0x43AA
#define ADE7858_WTHR1     0x43AB
#define ADE7858_WTHR0     0x43AC
#define ADE7858_VARTHR1   0x43AD
#define ADE7858_VARTHR0   0x43AE
#define ADE7858_RSV       0x43AF
#define ADE7858_VANOLOAD  0x43B0
#define ADE7858_APNOLOAD  0x43B1
#define ADE7858_VARNOLOAD 0x43B2
#define ADE7858_VLEVEL    0x43B3
#define ADE7858_DICOEFF   0x43B5
#define ADE7858_HPFDIS    0x43B6
#define ADE7858_ISUMLVL   0x43B8
#define ADE7858_ISUM      0x43BF
#define ADE7858_AIRMS     0x43C0
#define ADE7858_AVRMS     0x43C1
#define ADE7858_BIRMS     0x43C2
#define ADE7858_BVRMS     0x43C3
#define ADE7858_CIRMS     0x43C4
#define ADE7858_CVRMS     0x43C5
#define ADE7858_NIRMS     0x43C6
#define ADE7858_RUN       0xE228
#define ADE7858_AWATTHR   0xE400
#define ADE7858_BWATTHR   0xE401
#define ADE7858_CWATTHR   0xE402
#define ADE7858_AFWATTHR  0xE403
#define ADE7858_BFWATTHR  0xE404
#define ADE7858_CFWATTHR  0xE405
#define ADE7858_AVARHR    0xE406
#define ADE7858_BVARHR    0xE407
#define ADE7858_CVARHR    0xE408
#define ADE7858_AFVARHR   0xE409
#define ADE7858_BFVARHR   0xE40A
#define ADE7858_CFVARHR   0xE40B
#define ADE7858_AVAHR     0xE40C
#define ADE7858_BVAHR     0xE40D
#define ADE7858_CVAHR     0xE40E
#define ADE7858_IPEAK     0xE500
#define ADE7858_VPEAK     0xE501
#define ADE7858_STATUS0   0xE502
#define ADE7858_STATUS1   0xE503
#define ADE7858_OILVL     0xE507
#define ADE7858_OVLVL     0xE508
#define ADE7858_SAGLVL    0xE509
#define ADE7858_MASK0     0xE50A
#define ADE7858_MASK1     0xE50B
#define ADE7858_IAWV      0xE50C
#define ADE7858_IBWV      0xE50D
#define ADE7858_ICWV      0xE50E
#define ADE7858_VAWV      0xE510
#define ADE7858_VBWV      0xE511
#define ADE7858_VCWV      0xE512
#define ADE7858_AWATT     0xE513
#define ADE7858_BWATT     0xE514
#define ADE7858_CWATT     0xE515
#define ADE7858_AVA       0xE519
#define ADE7858_BVA       0xE51A
#define ADE7858_CVA       0xE51B
#define ADE7858_CHECKSUM  0xE51F
#define ADE7858_VNOM      0xE520
#define ADE7858_PHSTATUS  0xE600
#define ADE7858_ANGLE0    0xE601
#define ADE7858_ANGLE1    0xE602
#define ADE7858_ANGLE2    0xE603
#define ADE7858_PERIOD    0xE607
#define ADE7858_PHNOLOAD  0xE608
#define ADE7858_LINECYC   0xE60C
#define ADE7858_ZXTOUT    0xE60D
#define ADE7858_COMPMODE  0xE60E
#define ADE7858_GAIN      0xE60F
#define ADE7858_CFMODE    0xE610
#define ADE7858_CF1DEN    0xE611
#define ADE7858_CF2DEN    0xE612
#define ADE7858_CF3DEN    0xE613
#define ADE7858_APHCAL    0xE614
#define ADE7858_BPHCAL    0xE615
#define ADE7858_CPHCAL    0xE616
#define ADE7858_PHSIGN    0xE617
#define ADE7858_CONFIG    0xE618
#define ADE7858_MMODE     0xE700
#define ADE7858_ACCMODE   0xE701
#define ADE7858_LCYCMODE  0xE702
#define ADE7858_PEAKCYC   0xE703
#define ADE7858_SAGCYC    0xE704
#define ADE7858_CFCYC     0xE705
#define ADE7858_HSDC_CFG  0xE706
#define ADE7858_CONFIG2   0xEC01

#define ADE7858_READ_REG   0x1
#define ADE7858_WRITE_REG  0x0

#define ADE7858_MAX_TX    7
#define ADE7858_MAX_RX    7
#define ADE7858_STARTUP_DELAY 1

#define ADE7858_SPI_SLOW	(u32)(300 * 1000)
#define ADE7858_SPI_BURST	(u32)(1000 * 1000)
#define ADE7858_SPI_FAST	(u32)(2000 * 1000)

#define DRIVER_NAME		"ade7858"

/**
 * struct ade7858_state - device instance specific data
 * @spi:			actual spi_device
 * @work_trigger_to_ring: bh for triggered event handling
 * @inter:		used to check if new interrupt has been triggered
 * @last_timestamp:	passing timestamp from th to bh of interrupt handler
 * @indio_dev:		industrial I/O device structure
 * @trig:		data ready trigger registered with iio
 * @tx:			transmit buffer
 * @rx:			recieve buffer
 * @buf_lock:		mutex to protect tx and rx
 **/
struct ade7858_state {
	struct hsdc			*hsdc;
	struct i2c_client               *i2c;
	struct device			*dev;
	struct work_struct		work_trigger_to_ring;
	s64				last_timestamp;
	u8				*tx;
	u8				*rx;
	int				(*read_reg_8) (struct device *, u16, u8 *);
	int				(*read_reg_16) (struct device *, u16, u16 *);
	int				(*read_reg_24) (struct device *, u16, u32 *);
	int				(*read_reg_32) (struct device *, u16, u32 *);
	int				(*write_reg_8) (struct device *, u16, u8);
	int				(*write_reg_16) (struct device *, u16, u16);
	int				(*write_reg_24) (struct device *, u16, u32);
	int				(*write_reg_32) (struct device *, u16, u32);
	int                             irq;
	struct mutex			buf_lock;
};

extern int ade7858_probe(struct ade7858_state *st, struct device *dev);
extern int ade7858_hsdcreg(struct hsdc *hsdc);
#define ADE7858_AT91_IOC_MAGIC   'c'

#define ADE7858_RESET_DEV	_IOW(ADE7858_AT91_IOC_MAGIC,0, int)
#define ADE7858_RD_REG		_IOW(ADE7858_AT91_IOC_MAGIC,1, int)
#define ADE7858_WR_REG		_IOW(ADE7858_AT91_IOC_MAGIC,2, int)
#define ADE7858_ADJ_DEV		_IOW(ADE7858_AT91_IOC_MAGIC,3, int)
#define	ADE7558_HAR_REG		_IOW(ADE7858_AT91_IOC_MAGIC,4, int)
#define	ADE7858_BASE_REG	_IOW(ADE7858_AT91_IOC_MAGIC,5, int)
#define	ADE7858_AWAT_REG	_IOW(ADE7858_AT91_IOC_MAGIC,6, int)
#define ADE7858_RD_REG_8        _IOW(ADE7858_AT91_IOC_MAGIC,7, int)
#define ADE7858_RD_REG_16       _IOW(ADE7858_AT91_IOC_MAGIC,8, int)
#define ADE7858_WR_REG_8        _IOW(ADE7858_AT91_IOC_MAGIC,9, int)
#define ADE7858_WR_REG_16       _IOW(ADE7858_AT91_IOC_MAGIC,10, int)
#define ADE7858_INITIAL 	_IOW(ADE7858_AT91_IOC_MAGIC,11, int)

#define	ADE7558_FLICKER_REG		_IOW(ADE7858_AT91_IOC_MAGIC,12, int)

#endif
