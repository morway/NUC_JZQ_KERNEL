/*
 * arch/arm/mach-at91/hndl_product.c
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

void hntt1800x_id_mask_set(u8 id) 
{
	u8 tmp = id & 0x1f;

	switch (tmp) {
		case 1:
			id_mask = PRODUCT_HNTT1800S_FJ;
			break;

		case 2: 
			id_mask = PRODUCT_HNTT1800S_JL;
			break;

		case 4: 
			id_mask = PRODUCT_HNTT1800F_V3;
			break;

		case 5: 
			id_mask = PRODUCT_HNTT1800ND_V3;
			break;

            	case 6: 
			id_mask = PRODUCT_HNTT1800S_SC;
			break;

            	case 7: 
			id_mask = PRODUCT_HNTT1800U_V1;
			break;

		default:
			id_mask = PRODUCT_HNTT1800S_JL;
			break;
	}

	return;
}

u32 hntt1800x_id_mask_get(void)
{
	return id_mask;
}

int hntt1800x_id_match(u32 mask)
{
	if (id_mask & (mask)) {
		return ID_MATCHED;
	} else {
		return ID_NOT_MATCHED;
	}
}

static void  hntt1800x_id_cleanup(void)
{
    HNOS_DEBUG_INFO("HNTT1800X Product ID module exit.\n");	
    return;
}

static int __init  hntt1800x_id_init(void)
{
    hntt1800x_id_mask_set(system_rev>>8);

    HNOS_DEBUG_INFO("HNTT1800X Product ID module init.\n");	
    return 0;

}

module_init(hntt1800x_id_init);
module_exit(hntt1800x_id_cleanup);

EXPORT_SYMBOL(hntt1800x_id_mask_set);
EXPORT_SYMBOL(hntt1800x_id_mask_get);
EXPORT_SYMBOL(hntt1800x_id_match);

MODULE_LICENSE("Dual BSD/GPL");
