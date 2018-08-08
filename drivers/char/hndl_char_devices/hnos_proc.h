/*
 * drivers/char/hndl_char_devices/hnos_proc.h   
 *
 * /proc items support.
 *
 *  Modefy zrm,eter_zrm@163.com
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
#ifndef __HNDL_AT91_PROC_H
#define __HNDL_AT91_PROC_H

#define HNOS_PROC_ROOT			"hndl"

struct proc_item 
{
    char *name;  
    unsigned pin;
    u16 id_mask;
    u8 settings;

    int (*read_data)(u8);
    int (*write_data)(u8);

    int (*read_func) (struct proc_item *, char *);
    int (*write_func) (struct proc_item *, const char *, unsigned long);
};

struct proc_dir_entry *hnos_proc_mkdir(void);
void   hnos_proc_rmdir(void);
int    hnos_proc_entry_create(struct proc_item *item);

int    hnos_proc_items_create(struct proc_item *items, size_t size);
int    hnos_proc_items_remove(struct proc_item *items, size_t size);

#endif
