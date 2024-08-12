/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2024 Wojciech Cybowski <github.com/wcyb>
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CFG_SYS_SDRAM_BASE           0x80000000

#define CFG_SYS_INIT_RAM_ADDR        0xbd000000
#define CFG_SYS_INIT_RAM_SIZE        0x2000

/*
 * Serial Port
 */
#define CFG_SYS_NS16550_CLK          25000000

#endif  /* __CONFIG_H */
