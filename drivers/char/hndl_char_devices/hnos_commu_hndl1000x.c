/*
 * drivers/char/hndl_char_devices/hnos_commu_hndl1000x.c
 *
 * Communication modules for hndl1000x and etc.
 *
 * Author ZhangRM, peter_zrm@163.com
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
#include "hnos_commu.h"
#include "hnos_ioctl.h"
#include "hnos_iomem.h"                /* iomem object  */
#include "hnos_hndl1000x.h"

struct at91_commu_busmem 
{
	u8 start_bit;
	u8 power_bit;
	u8 reset_bit;
	struct iomem_object *ctrl_iomem;
	struct iomem_object *state_iomem;
};

static struct at91_commu_busmem commu_busmem_hndl1000x= {
	.start_bit = OUTPUT0,
	.power_bit = TPWR_CTL,
	.reset_bit = NSFOTRESET,
	.ctrl_iomem = NULL,
	.state_iomem = NULL,
};


static struct at91_commu_object  commu_hndl1000x = 
{
	.start_pin = 0,
	.power_pin = 0,
	.reset_pin = 0,
};

static int commu_type_hndl1000x(void)
{
	int ret = 0;
	u8 type =0;
	struct iomem_object *iomem = commu_busmem_hndl1000x.state_iomem;
	ret = iomem->read_byte(iomem, &type, IO_RDONLY);

	ret = type & 0x1f;

	return ret;
}

static int commu_opt_hndl1000x(struct at91_commu_object *commu, int level,u8 bit)
{
	u8 data = 0;
	int ret = 0;
	struct iomem_object *iomem = commu_busmem_hndl1000x.ctrl_iomem;


	if((level == OUTPUT_LEVEL_HIGH) || (level == OUTPUT_LEVEL_LOW))
		data = level;
	ret = iomem->write_bit(iomem, bit, data);
	return ret;

}

static int commu_start_hndl1000x(struct at91_commu_object *commu, int level)
{
	int ret = 0;

	ret = commu_opt_hndl1000x(commu,level,commu_busmem_hndl1000x.start_bit);
	return ret;
}

static int commu_reset_hndl1000x(struct at91_commu_object *commu, int level)
{
	int ret = 0;

	ret = commu_opt_hndl1000x(commu,level,commu_busmem_hndl1000x.reset_bit);

	return ret;
}

static int commu_power_hndl1000x(struct at91_commu_object *commu, enum EPower power)
{       
	int ret = 0;
	int level = power;
	ret = commu_opt_hndl1000x(commu,level,commu_busmem_hndl1000x.power_bit);

	return ret;
}

int __exit commu_exit(void)
{
	struct at91_commu_busmem *busdev = &commu_busmem_hndl1000x;

	iomem_object_put(busdev->ctrl_iomem);
	iomem_object_put(busdev->state_iomem);
	return 0;
}

int __init commu_init(void)
{
	int ret;
	struct at91_commu_object *commdev = &commu_hndl1000x;
	struct at91_commu_busmem *busdev = &commu_busmem_hndl1000x;

	commdev->commu_type = commu_type_hndl1000x; 
	commdev->power = commu_power_hndl1000x;
	commdev->start = commu_start_hndl1000x;
	commdev->reset = commu_reset_hndl1000x;


	busdev->ctrl_iomem = iomem_object_get(GPRS_LCD_BEEP_CS, 0);
	if (!busdev->ctrl_iomem) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) GPRS_LCD_BEEP_CS);
		return -1;
	}
	busdev->state_iomem = iomem_object_get(ID_CS1, 0);
	if (!busdev->state_iomem) {
		printk(KERN_ERR "%s: can't get iomem (phy %08x).\n", __FUNCTION__,
				(unsigned int) ID_CS1);
		iomem_object_put(busdev->ctrl_iomem);
		return -1;
	}        
	ret = commu_opt_hndl1000x(commdev,OUTPUT_LEVEL_LOW,busdev->power_bit);
	ret = commu_opt_hndl1000x(commdev,OUTPUT_LEVEL_LOW,busdev->start_bit);
	ret = commu_opt_hndl1000x(commdev,OUTPUT_LEVEL_HIGH,busdev->reset_bit);

	return 0;

}

static void __exit commu_hndl1000x_exit(void)
{       
        commu_exit();
	commu_module_unregister(&commu_hndl1000x);

        HNOS_DEBUG_INFO("unregistered the commu module for hndl1000x and etc.");
	return;
}

static int __init  commu_hndl1000x_init(void)
{
        HNOS_DEBUG_INFO("Registered a commu module for hndl1000x and etc.");

	commu_init();
	return commu_module_register(&commu_hndl1000x);
}

module_init(commu_hndl1000x_init);
module_exit(commu_hndl1000x_exit);

MODULE_AUTHOR("ZhangRM");
MODULE_LICENSE("Dual BSD/GPL");

