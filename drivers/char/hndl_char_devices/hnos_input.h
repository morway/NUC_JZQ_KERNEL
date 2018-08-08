#ifndef _HNOS_INPUT_H
#define _HNOS_INPUT_H

#define	INPUT_CHAN_MAX			32
#define NR_INPUT_DEVICES		10
#define SMCBUS_CHAN_ALL			0xff
#define SMCBUS_INPUT_NAME		"smcbus_rmi"

struct gpio_rmi_data
{
	u8 size;
	struct proc_item *items;
};

struct smcbus_rmi_data
{
	int (*read)(struct smcbus_rmi_data *bus, u8 ch, u32 *reslt);
	int (*proc_read)(struct smcbus_rmi_data *bus, char *buf);
};

/* 
 * Note:
 * The GPIO RMI channels must be located before the SMCBUS RMI channels, and continunous.
 * */

struct hndl_rmi_device
{
	unsigned long is_open;	  
	struct cdev cdev;
	struct semaphore lock;
	struct gpio_rmi_data *gpio;
	u8 gpio_offset;
	u8 gpio_end;
	struct smcbus_rmi_data *smcbus;
	u8 smcbus_offset;
	u8 smcbus_end;
};

struct hndl_rmi_device *rmi_device_alloc(void);
void rmi_device_free(struct hndl_rmi_device *dev);
int rmi_device_register(struct hndl_rmi_device *dev);
int rmi_device_unregister(struct hndl_rmi_device *dev);
int rmi_gpio_register(struct hndl_rmi_device *rmi, struct gpio_rmi_data *gpio, u8 offset, u8 size);
int rmi_smcbus_register(struct hndl_rmi_device *rmi, struct smcbus_rmi_data *bus, u8 offset, u8 size);
int rmi_gpio_unregister(struct hndl_rmi_device *rmi, struct gpio_rmi_data *gpio);
int rmi_smcbus_unregister(struct hndl_rmi_device *rmi, struct smcbus_rmi_data *bus);
int rmi_get_smcbus_offset(struct hndl_rmi_device *rmi);


#endif
