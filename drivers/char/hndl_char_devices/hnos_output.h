#ifndef _HNOS_OUTPUT_H
#define _HNOS_OUTPUT_H

#define	OUTPUT_CHAN_MAX			32
#define NR_OUTPUT_DEVICES		10
#define SMCBUS_CHAN_ALL			0xff
#define SMCBUS_OUTPUT_NAME		"smcbus_output"

struct gpio_rmc_data
{
	u8 size;
	struct proc_item *items;
};

#define REFRESH_FREQ_TIMEOUT	10

#define SMCBUS_CHIP_ENABLE() \
	do { \
		at91_set_gpio_output(AT91_PIN_PA13, 0); \
	} while(0)

#define SMCBUS_CHIP_DISABLE() \
	do { \
		at91_set_gpio_output(AT91_PIN_PA13, 1); \
	} while(0)


struct smcbus_refresh_data
{
	spinlock_t lock;
	unsigned int is_timer_started;
	struct timer_list timer;
	unsigned long refresh_cnt;
};

struct smcbus_rmc_data
{
	u32 smcbus_stat;
	int (*read)(struct smcbus_rmc_data *bus, u32 *reslt);
	int (*write)(struct smcbus_rmc_data *bus, u32 bitmap, int is_set);
	int (*proc_read)(struct smcbus_rmc_data *bus, char *buf);
	int (*proc_write)(struct smcbus_rmc_data *bus, const char __user *buf, unsigned long count);
};

/* 
 * Note:
 * The GPIO RMC channels must be located before the SMCBUS RMC channels, and continunous.
 * */
struct hndl_rmc_device
{
	unsigned long is_open;	  
	struct cdev cdev;
	spinlock_t lock;
	struct gpio_rmc_data *gpio;
	u8 gpio_offset;
	u8 gpio_end;
	struct smcbus_rmc_data *smcbus;
	u8 smcbus_offset;
	u8 smcbus_end;
	struct smcbus_refresh_data *bus_refresh;
};

struct hndl_rmc_device *rmc_device_alloc(void);
void rmc_device_free(struct hndl_rmc_device *dev);
int rmc_device_register(struct hndl_rmc_device *dev);
int rmc_device_unregister(struct hndl_rmc_device *dev);
int rmc_gpio_register(struct hndl_rmc_device *output, struct gpio_rmc_data *gpio, u8 offset, u8 size);
int rmc_smcbus_register(struct hndl_rmc_device *output, struct smcbus_rmc_data *bus, u8 offset, u8 size);
int rmc_gpio_unregister(struct hndl_rmc_device *output, struct gpio_rmc_data *gpio);
int rmc_smcbus_unregister(struct hndl_rmc_device *output, struct smcbus_rmc_data *bus);
int rmc_get_smcbus_offset(struct hndl_rmc_device *output);

int rmc_smcbus_refresh_start(struct hndl_rmc_device *dev);
int rmc_smcbus_refresh_stop(struct hndl_rmc_device *dev);

#endif
