// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Wojciech Cybowski <github.com/wcyb>
 */

#include <init.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <mach/ar71xx_regs.h>
#include <mach/ddr.h>
#include <mach/ath79.h>
#include <debug_uart.h>

#ifdef CONFIG_DEBUG_UART_BOARD_INIT
void board_debug_uart_init(void)
{
	void __iomem *regs;
	u32 val;

	regs = map_physmem(AR71XX_GPIO_BASE, AR71XX_GPIO_SIZE,
			   MAP_NOCACHE);

	/*
	 * GPIO9 as input, GPIO10 as output
	 */
	val = readl(regs + AR71XX_GPIO_REG_OE);
	val |= QCA953X_GPIO(9);
	val &= ~QCA953X_GPIO(10);
	writel(val, regs + AR71XX_GPIO_REG_OE);

	/*
	 * Enable GPIO9 as UART0_SIN
	 */
	val = readl(regs + QCA953X_GPIO_REG_IN_ENABLE0);
	val &= ~QCA953X_GPIO_MUX_MASK(8);
	val |= QCA953X_GPIO_IN_MUX_UART0_SIN << 8;
	writel(val, regs + QCA953X_GPIO_REG_IN_ENABLE0);

	/*
	 * Enable GPIO10 as UART0_SOUT
	 */
	val = readl(regs + QCA953X_GPIO_REG_OUT_FUNC2);
	val &= ~QCA953X_GPIO_MUX_MASK(16);
	val |= QCA953X_GPIO_OUT_MUX_UART0_SOUT << 16;
	writel(val, regs + QCA953X_GPIO_REG_OUT_FUNC2);
}
#endif

int board_early_init_f(void)
{
	void __iomem *regs;
	u32 val;

	regs = map_physmem(AR71XX_GPIO_BASE, AR71XX_GPIO_SIZE,
			   MAP_NOCACHE);

	/*
	 * Disable JTAG and all CLK_OBSx
	 */
	val = 0;
	val |= QCA953X_GPIO_FUNC_JTAG_DISABLE;
	writel(val, regs + QCA953X_GPIO_REG_FUNC);

	ddr_init();
	ath79_eth_reset();
	return 0;
}
