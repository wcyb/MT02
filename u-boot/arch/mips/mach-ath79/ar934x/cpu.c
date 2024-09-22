// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Wojciech Cybowski <github.com/wcyb>
 */

#include <asm/io.h>
#include <mach/ar71xx_regs.h>

void lowlevel_init(void)
{
    void __iomem *rregs = map_physmem(AR71XX_RESET_BASE, AR71XX_RESET_SIZE,
					  MAP_NOCACHE);
    void __iomem *rtcregs = map_physmem(AR934X_RTC_BASE, AR934X_RTC_SIZE,
					  MAP_NOCACHE);
    u32 reg_val;

    /* RTC Reset */
    
    /* Set the necessary bits in the reset register */
    reg_val = readl(rregs + AR934X_RESET_REG_RESET_MODULE);
    reg_val |= 0x08000000;
    writel(reg_val, rregs + AR934X_RESET_REG_RESET_MODULE);
    
    /* Clear the necessary bits in the reset register */
    reg_val = readl(rregs + AR934X_RESET_REG_RESET_MODULE);
    reg_val &= 0xf7ffffff;
    writel(reg_val, rregs + AR934X_RESET_REG_RESET_MODULE);

    /* RTC Force Wake */
    writel(0x01, rtcregs + AR934X_RTC_REG_SYNC_RESET);

    /* Wait for RTC in on state */
    do {
        reg_val = readl(rtcregs + AR934X_RTC_REG_SYNC_STATUS);
    } while (!(reg_val & 0x02));
}
