/*
 * arch/arm/mach-at91/hndl_product_hndl1000x.c
 * Author ZhangRM, peter_zrm@163.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "hnos_generic.h"

u32  id_mask;
extern unsigned int system_rev;

void hndl1000x_id_mask_set(u8 id) 
{
	u8 tmp = id & 0xf8;

	switch (tmp) {
		case 0x08:
			id_mask = PRODUCT_HNDL1000X_CQ;
			break;

		case 0x10: 
			id_mask = PRODUCT_HNDL1000X_LN;
			break;
		case 0x18: 
			id_mask = PRODUCT_HNDL1000X_HLJ;
			break;			
		default:
			id_mask = PRODUCT_HNDL1000X_INVALID;
			break;
	}

	return;
}

u32 hndl1000x_id_mask_get(void)
{
	return id_mask;
}

int hndl1000x_id_match(u32 mask)
{
	if (id_mask == mask) {
		return ID_MATCHED;
	} else {
		return ID_NOT_MATCHED;
	}
}

static void  hndl1000x_id_cleanup(void)
{
    HNOS_DEBUG_INFO("WFE800X Product ID module exit.\n");	
    return;
}

static int __init  hndl1000x_id_init(void)
{
    hndl1000x_id_mask_set(system_rev >> 8);

    HNOS_DEBUG_INFO("HNDL800X Product ID module init.\n");	
    return 0;

}

module_init(hndl1000x_id_init);
module_exit(hndl1000x_id_cleanup);
MODULE_LICENSE("Dual BSD/GPL");

EXPORT_SYMBOL(hndl1000x_id_mask_set);
EXPORT_SYMBOL(hndl1000x_id_mask_get);
EXPORT_SYMBOL(hndl1000x_id_match);


