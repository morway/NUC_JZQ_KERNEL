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
#ifndef __HNOS_DEFINES_HNTT1000_SHANGHAI_H 
#define __HNOS_DEFINES_HNTT1000_SHANGHAI_H 

/* HNTT1800X 遥信 通道定义 */
#define		INPUT_STATUS_0			(1 << 0)      /* 状态量1 */
#define		INPUT_STATUS_1			(1 << 1)      /* 状态量2 */
#define		INPUT_OPEN_COVER		(1 << 2)      /* 端盖开盖检测 */
#define		INPUT_OPEN_GLASS		(1 << 3)      /* 下透镜开盖检测 */
#define		INPUT_TDK6513_STATE		(1 << 4)      /* 校表状态 */
#define		INPUT_ADSORB_IRDA		(1 << 5)      /* 红外判定 */

#define		INPUT_SMCBUS_OFFSET		16              /* (总线扩展)遥信输入从第16路开始 */
#define		INPUT_SMCBUS_SIZE		16              /* (总线扩展)遥信共计16路 */

/* HNTT1800X 遥控 通道定义 */
#define		OUTPUT_CTRL_0			(1 << 0)      /* 负荷控制输出 */
#define		OUTPUT_CTRL_1			(1 << 1)      /* 告警输出 */
#define		OUTPUT_REMOTE_POWER		(1 << 2)      /* 遥控告警电源控制, 写1打开电源 */
#define		OUTPUT_REMOTE_ENABLE		(1 << 3)      /* 遥控告警允许, 写0允许 */
#define		OUTPUT_PLC_POWER		(1 << 4)      /* 载波电源控制 */

#define		OUTPUT_SMCBUS_OFFSET		16              /* (总线扩展)遥控输出从第16路开始 */
#define		OUTPUT_SMCBUS_SIZE		16              /* (总线扩展)遥信共计16路 */

#define		NCHANNEL_PER_SMCBUS		8      
#define		NR_SMCBUS			2

#endif
