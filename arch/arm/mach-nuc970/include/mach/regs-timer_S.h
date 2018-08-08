/*
 * arch/arm/mach-nuc970/include/mach/regs-timer.h
 *
 * Copyright (c) 2014 Nuvoton technology corporation
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __ASM_ARCH_REGS_TIMER_H
#define __ASM_ARCH_REGS_TIMER_H

/* Timer Registers */

#define TMR_BA			NUC970_VA_TIMER

#define REG_TMR_CSR(x)		(TMR_BA + 0x100 * (x) + 0x00)
#define REG_TMR_CMPR(x)		(TMR_BA + 0x100 * (x) + 0x04)
#define REG_TMR_DR(x)		(TMR_BA + 0x100 * (x) + 0x08)
#define REG_TMR_ISR			(TMR_BA + 0x060)


#endif /*  __ASM_ARCH_REGS_TIMER_H */
