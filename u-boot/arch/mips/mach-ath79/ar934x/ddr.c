// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 Wojciech Cybowski <github.com/wcyb>
 *
 * Based on RAM init sequence by Piotr Dymacz <pepe2k@gmail.com>
 */

#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <mach/ar71xx_regs.h>
#include <mach/ath79.h>

DECLARE_GLOBAL_DATA_PTR;

#define BITS(_start, _bits)		(((1 << (_bits)) - 1) << _start)

/*
 * Below defines are "safe" DDR1/DDR2 timing parameters.
 * They should work for most chips, but not for all.
 *
 * For different values, user can define target value
 * of all memory controller related registers.
 *
 */
#define DDRx_tMRD_ns	10
#define DDRx_tRAS_ns	40
#define DDRx_tRCD_ns	15
#define DDRx_tRP_ns		15
#define DDRx_tRRD_ns	10
#define DDRx_tWR_ns		5
#define DDRx_tWTR_ns	10

#define DDR1_tRFC_ns	75
#define DDR2_tRFC_ns	120

#define DDR2_tFAW_ns	50
#define DDR2_tWL_ns		5

#define DDR_addit_lat	0
#define DDR_burst_len	8

/* All above values are safe for clocks not lower than below values */
#define DDR1_timing_clk_max	400
#define DDR2_timing_clk_max	533

/* Maximum timing values, based on register fields sizes */
#define MAX_tFAW	BITS(0, 6)
#define MAX_tMRD	BITS(0, 4)
#define MAX_tRAS	BITS(0, 5)
#define MAX_tRCD	BITS(0, 4)
#define MAX_tRFC	BITS(0, 6)
#define MAX_tRP		BITS(0, 4)
#define MAX_tRRD	BITS(0, 4)
#define MAX_tRTP	BITS(0, 4)
#define MAX_tRTW	BITS(0, 5)
#define MAX_tWL		BITS(0, 4)
#define MAX_tWR		BITS(0, 4)
#define MAX_tWTR	BITS(0, 5)

#define DDR_CTL_HALF_WIDTH				BIT(1)
#define DDR_CTL_SRAM_REQ_ACK			BIT(3)
#define CPU_PLL_DITHER_NFRAC_MIN_SHIFT	6
#define CPU_PLL_DITHER_NFRAC_MIN_MASK	BITS(CPU_PLL_DITHER_NFRAC_MIN_SHIFT, 6)
#define DDR_PLL_DITHER_NFRAC_MIN_SHIFT	10
#define DDR_PLL_DITHER_NFRAC_MIN_MASK	BITS(DDR_PLL_DITHER_NFRAC_MIN_SHIFT, 10)
#define DDR_CTRL_CFG_CPU_DDR_SYNC_MASK	BIT(2)
#define DDR_CTRL_CFG_PAD_DDR2_SEL_MASK	BIT(6)
#define DDR_DDR2_CFG_DDR2_EN_MASK		BIT(0)
#define DDR_DDR2_CFG_DDR2_TFAW_SHIFT	2
#define DDR_DDR2_CFG_DDR2_TFAW_MASK		BITS(DDR_DDR2_CFG_DDR2_TFAW_SHIFT, 6)
#define DDR_DDR2_CFG_DDR2_TWL_SHIFT		10
#define DDR_DDR2_CFG_DDR2_TWL_MASK		BITS(DDR_DDR2_CFG_DDR2_TWL_SHIFT, 4)
#define DDR_CFG_PAGE_CLOSE_MASK			BIT(30)
#define DDR_CFG_CAS_3LSB_SHIFT			27
#define DDR_CFG_CAS_3LSB_MASK			BITS(DDR_CFG_CAS_3LSB_SHIFT, 3)
#define DDR_CFG_CAS_MSB_MASK			BIT(31)
#define DDR_CFG_TMRD_SHIFT				23
#define DDR_CFG_TMRD_MASK				BITS(DDR_CFG_TMRD_SHIFT, 4)
#define DDR_CFG_TRFC_SHIFT				17
#define DDR_CFG_TRFC_MASK				BITS(DDR_CFG_TRFC_SHIFT, 6)
#define DDR_CFG_TRRD_SHIFT				13
#define DDR_CFG_TRRD_MASK				BITS(DDR_CFG_TRRD_SHIFT, 4)
#define DDR_CFG_TRP_SHIFT				9
#define DDR_CFG_TRP_MASK				BITS(DDR_CFG_TRP_SHIFT, 4)
#define DDR_CFG_TRCD_SHIFT				5
#define DDR_CFG_TRCD_MASK				BITS(DDR_CFG_TRCD_SHIFT, 4)
#define DDR_CFG_TRAS_SHIFT				0
#define DDR_CFG_TRAS_MASK				BITS(DDR_CFG_TRAS_SHIFT, 5)
#define DDR_CFG2_CKE_MASK				BIT(7)
#define DDR_CFG2_GATE_OPEN_LAT_SHIFT	26
#define DDR_CFG2_GATE_OPEN_LAT_MASK		BITS(DDR_CFG2_GATE_OPEN_LAT_SHIFT, 4)
#define DDR_CFG2_TWTR_SHIFT				21
#define DDR_CFG2_TWTR_MASK				BITS(DDR_CFG2_TWTR_SHIFT, 5)
#define DDR_CFG2_TRTP_SHIFT				17
#define DDR_CFG2_TRTP_MASK				BITS(DDR_CFG2_TRTP_SHIFT, 4)
#define DDR_CFG2_TRTW_SHIFT				12
#define DDR_CFG2_TRTW_MASK				BITS(DDR_CFG2_TRTW_SHIFT, 5)
#define DDR_CFG2_TWR_SHIFT				8
#define DDR_CFG2_TWR_MASK				BITS(DDR_CFG2_TWR_SHIFT, 4)
#define DDR_CFG2_BURST_LEN_SHIFT		0
#define DDR_CFG2_BURST_LEN_MASK			BITS(DDR_CFG2_BURST_LEN_SHIFT, 4)
#define DDR_CFG2_BURST_TYPE_MASK		BIT(4)
#define DDR_CTRL_FORCE_PRECHRG_ALL_MASK BIT(3)
#define DDR_CTRL_FORCE_EMR2S_MASK		BIT(4)
#define DDR_CTRL_FORCE_EMR3S_MASK		BIT(5)
#define DDR_CTRL_FORCE_AUTO_REFRH_MASK	BIT(2)
#define DDR_CTRL_FORCE_EMRS_MASK		BIT(1)
#define DDR_CTRL_FORCE_MRS_MASK			BIT(0)

#define ath79_reg_read_set(_addr, _mask)	\
		writel((readl((_addr)) | (_mask)), (_addr))

#define ath79_reg_read_clear(_addr, _mask)	\
		writel((readl((_addr)) & ~(_mask)), (_addr))

/* Prepare DDR SDRAM extended mode register 2 value */
#define DDR_SDRAM_EMR2_PASR_SHIFT	0
#define DDR_SDRAM_EMR2_PASR_MASK	BITS(DDR_SDRAM_EMR2_PASR_SHIFT, 3)
#define DDR_SDRAM_EMR2_DCC_EN_SHIFT	3
#define DDR_SDRAM_EMR2_DCC_EN_MASK	(1 << DDR_SDRAM_EMR2_DCC_EN_SHIFT)
#define DDR_SDRAM_EMR2_SRF_EN_SHIFT	7
#define DDR_SDRAM_EMR2_SRF_EN_MASK	(1 << DDR_SDRAM_EMR2_SRF_EN_SHIFT)

#define _ddr_sdram_emr2_val(_pasr,   \
			    _dcc_en, \
			    _srf_en) \
				     \
	((_pasr   << DDR_SDRAM_EMR2_PASR_SHIFT)   & DDR_SDRAM_EMR2_PASR_MASK)   |\
	((_dcc_en << DDR_SDRAM_EMR2_DCC_EN_SHIFT) & DDR_SDRAM_EMR2_DCC_EN_MASK) |\
	((_srf_en << DDR_SDRAM_EMR2_SRF_EN_SHIFT) & DDR_SDRAM_EMR2_SRF_EN_MASK)

/* Prepare DDR SDRAM extended mode register value */
#define DDR_SDRAM_EMR_DLL_EN_SHIFT			0
#define DDR_SDRAM_EMR_DLL_EN_MASK			(1 << DDR_SDRAM_EMR_DLL_EN_SHIFT)
#define DDR_SDRAM_EMR_WEAK_STRENGTH_SHIFT	1
#define DDR_SDRAM_EMR_WEAK_STRENGTH_MASK	(1 << DDR_SDRAM_EMR_WEAK_STRENGTH_SHIFT)
#define DDR_SDRAM_EMR_OCD_PRG_SHIFT			7
#define DDR_SDRAM_EMR_OCD_PRG_MASK			BITS(DDR_SDRAM_EMR_OCD_PRG_SHIFT, 3)
#define DDR_SDRAM_EMR_OCD_EXIT_VAL			0
#define DDR_SDRAM_EMR_OCD_DEFAULT_VAL		7
#define DDR_SDRAM_EMR_NDQS_DIS_SHIFT		10
#define DDR_SDRAM_EMR_NDQS_DIS_MASK			(1 << DDR_SDRAM_EMR_NDQS_DIS_SHIFT)
#define DDR_SDRAM_EMR_RDQS_EN_SHIFT			11
#define DDR_SDRAM_EMR_RDQS_EN_MASK			(1 << DDR_SDRAM_EMR_RDQS_EN_SHIFT)
#define DDR_SDRAM_EMR_OBUF_DIS_SHIFT		12
#define DDR_SDRAM_EMR_OBUF_DIS_MASK			(1 << DDR_SDRAM_EMR_OBUF_DIS_SHIFT)

#define _ddr_sdram_emr_val(_dll_dis,  \
			   _drv_weak, \
			   _ocd_prg,  \
			   _ndqs_dis, \
			   _rdqs_en,  \
			   _obuf_dis) \
				      \
	((_dll_dis  << DDR_SDRAM_EMR_DLL_EN_SHIFT)   & DDR_SDRAM_EMR_DLL_EN_MASK)   |\
	((_ocd_prg  << DDR_SDRAM_EMR_OCD_PRG_SHIFT)  & DDR_SDRAM_EMR_OCD_PRG_MASK)  |\
	((_ndqs_dis << DDR_SDRAM_EMR_NDQS_DIS_SHIFT) & DDR_SDRAM_EMR_NDQS_DIS_MASK) |\
	((_rdqs_en  << DDR_SDRAM_EMR_RDQS_EN_SHIFT)  & DDR_SDRAM_EMR_RDQS_EN_MASK)  |\
	((_obuf_dis << DDR_SDRAM_EMR_OBUF_DIS_SHIFT) & DDR_SDRAM_EMR_OBUF_DIS_MASK) |\
	((_drv_weak << DDR_SDRAM_EMR_WEAK_STRENGTH_SHIFT) & DDR_SDRAM_EMR_WEAK_STRENGTH_MASK)

/*
 * Prepare DDR SDRAM mode register value
 * For now use always burst length == 8
 */
#define DDR_SDRAM_MR_BURST_LEN_SHIFT		0
#define DDR_SDRAM_MR_BURST_LEN_MASK			BITS(DDR_SDRAM_MR_BURST_LEN_SHIFT, 3)
#define DDR_SDRAM_MR_BURST_INTERLEAVE_SHIFT	3
#define DDR_SDRAM_MR_BURST_INTERLEAVE_MASK	(1 << DDR_SDRAM_MR_BURST_INTERLEAVE_SHIFT)
#define DDR_SDRAM_MR_CAS_LAT_SHIFT			4
#define DDR_SDRAM_MR_CAS_LAT_MASK			BITS(DDR_SDRAM_MR_CAS_LAT_SHIFT, 3)
#define DDR_SDRAM_MR_DLL_RESET_SHIFT		8
#define DDR_SDRAM_MR_DLL_RESET_MASK			(1 << DDR_SDRAM_MR_DLL_RESET_SHIFT)
#define DDR_SDRAM_MR_WR_RECOVERY_SHIFT		9
#define DDR_SDRAM_MR_WR_RECOVERY_MASK		BITS(DDR_SDRAM_MR_WR_RECOVERY_SHIFT, 3)

#define _ddr_sdram_mr_val(_burst_i, \
			  _cas_lat, \
			  _dll_res, \
			  _wr_rcov) \
				    \
	((0x3            << DDR_SDRAM_MR_BURST_LEN_SHIFT)   & DDR_SDRAM_MR_BURST_LEN_MASK)   |\
	((_cas_lat       << DDR_SDRAM_MR_CAS_LAT_SHIFT)     & DDR_SDRAM_MR_CAS_LAT_MASK)     |\
	((_dll_res       << DDR_SDRAM_MR_DLL_RESET_SHIFT)   & DDR_SDRAM_MR_DLL_RESET_MASK)   |\
	(((_wr_rcov - 1) << DDR_SDRAM_MR_WR_RECOVERY_SHIFT) & DDR_SDRAM_MR_WR_RECOVERY_MASK) |\
	((_burst_i       << DDR_SDRAM_MR_BURST_INTERLEAVE_SHIFT) & DDR_SDRAM_MR_BURST_INTERLEAVE_MASK)

enum {
	AR934X_SDRAM = 0,
	AR934X_DDR1,
	AR934X_DDR2,
};

void ar934x_ddr_init(const u16 cpu_mhz, const u16 ddr_mhz, const u16 ahb_mhz)
{
	void __iomem *ddr_regs;
	void __iomem *pll_regs;
	u32 mem_type, tmp_clk;
	u32 cas_lat, ddr_width, reg, tmp, wr_recovery;

	ddr_regs = map_physmem(AR71XX_DDR_CTRL_BASE, AR71XX_DDR_CTRL_SIZE,
			       MAP_NOCACHE);

	reg = ath79_get_bootstrap();
	if (reg & AR934X_BOOTSTRAP_SDRAM_DISABLED) {	/* DDR */
		if (reg & AR934X_BOOTSTRAP_DDR1) {	/* DDR 1 */
			mem_type = AR934X_DDR1;
		} else {				/* DDR 2 */
			mem_type = AR934X_DDR2;
		}
	} else {					/* SDRAM */
		mem_type = AR934X_SDRAM;
	}

	/* Set CAS based on clock, but allow to set static value */
#ifndef CONFIG_BOARD_DRAM_CAS_LATENCY
	if (mem_type == AR934X_DDR1) {
		if (ddr_mhz <= 266) {
			cas_lat = 2;
		} else {
			cas_lat = 3;
		}
	} else if (mem_type == AR934X_DDR2) {
		if (ddr_mhz <= 400) {
			cas_lat = 3;
		} else if (ddr_mhz <= 533) {
			cas_lat = 4;
		} else if (ddr_mhz <= 666) {
			cas_lat = 5;
		} else if (ddr_mhz <= 800) {
			cas_lat = 6;
		} else {
			cas_lat = 7;
		}
	} else { /* SDRAM */

	}
#else
	cas_lat = CONFIG_BOARD_DRAM_CAS_LATENCY;
#endif

	/* AR933x supports only 16-bit memory */
	/* For other WiSoCs we can determine DDR width, based on bootstrap */
#ifndef CONFIG_BOARD_DRAM_DDR_WIDTH
	if (reg & BIT(3))
		ddr_width = 32;
	else
		ddr_width = 16;
#else
	ddr_width = CONFIG_BOARD_DRAM_DDR_WIDTH;
#endif

	if (ddr_width == 32) {
		/* For 32-bit clear HALF_WIDTH and set VEC = 0xFF */
		ath79_reg_read_clear(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTL_HALF_WIDTH);
		
		writel(0xFF, ddr_regs + AR71XX_DDR_REG_RD_CYCLE);
	} else {
		ath79_reg_read_set(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTL_HALF_WIDTH);

		writel(0xFFFF, ddr_regs + AR71XX_DDR_REG_RD_CYCLE);
	}

	/* If DDR_MHZ < 2 * AHB_MHZ, set DDR FSM wait control to 0xA24 */
	if (ddr_mhz < (2 * ahb_mhz))
		writel(0xA24, ddr_regs + AR934X_DDR_REG_FSM_WAIT_CTRL);

	/* If CPU clock < AHB clock, set SRAM REQ ACK */
	if (cpu_mhz < ahb_mhz)
		ath79_reg_read_set(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTL_SRAM_REQ_ACK);
	else
		ath79_reg_read_clear(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTL_SRAM_REQ_ACK);

	/*
	 * CPU/DDR sync mode only when we don't use
	 * fractional multipliers in PLL/clocks config
	 */
	tmp = 0;

	pll_regs = map_physmem(AR71XX_PLL_BASE, AR71XX_PLL_SIZE,
			       MAP_NOCACHE);

	reg = readl(pll_regs + AR934X_PLL_CPU_DIT_FRAC_REG);
	reg = (reg & CPU_PLL_DITHER_NFRAC_MIN_MASK)
	    	>> CPU_PLL_DITHER_NFRAC_MIN_SHIFT;

	if (reg)
		tmp = 1;

	reg = readl(pll_regs + AR934X_PLL_DDR_DIT_FRAC_REG);
	reg = (reg & DDR_PLL_DITHER_NFRAC_MIN_MASK)
	    	>> DDR_PLL_DITHER_NFRAC_MIN_SHIFT;

	if (reg)
		tmp = 1;

	if (!tmp && (cpu_mhz == ddr_mhz)) {
		ath79_reg_read_set(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTRL_CFG_CPU_DDR_SYNC_MASK);
	} else {
		ath79_reg_read_clear(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTRL_CFG_CPU_DDR_SYNC_MASK);
	}

	/* Check if clock is not too low for our "safe" timing values */
	tmp_clk = ddr_mhz;
	if (mem_type == AR934X_DDR1) {
		if (tmp_clk < DDR1_timing_clk_max)
			tmp_clk = DDR1_timing_clk_max;
	} else if (mem_type == AR934X_DDR2) {
		if (tmp_clk < DDR2_timing_clk_max)
			tmp_clk = DDR2_timing_clk_max;
	} else { /* SDRAM */

	}

	/* Enable DDR2 */
	if (mem_type == AR934X_DDR2) {
		ath79_reg_read_set(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTRL_CFG_PAD_DDR2_SEL_MASK);
#ifndef CONFIG_ATH79_DDR_DDR2_CFG_REG_VAL
	reg = readl(ddr_regs + AR934X_DDR_REG_DDR2_CONFIG);

	/* Enable DDR2 */
	reg = reg | DDR_DDR2_CFG_DDR2_EN_MASK;

	/* tFAW */
	tmp = ((DDR2_tFAW_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tFAW)
		tmp = MAX_tFAW;

	tmp = (tmp << DDR_DDR2_CFG_DDR2_TFAW_SHIFT)
	      & DDR_DDR2_CFG_DDR2_TFAW_MASK;
	reg = reg & ~DDR_DDR2_CFG_DDR2_TFAW_MASK;
	reg = reg | tmp;

	/* tWL */
	tmp = (2 * cas_lat) - 3;
	tmp = (tmp << DDR_DDR2_CFG_DDR2_TWL_SHIFT)
	      & DDR_DDR2_CFG_DDR2_TWL_MASK;
	reg = reg & ~DDR_DDR2_CFG_DDR2_TWL_MASK;
	reg = reg | tmp;

	writel(reg, ddr_regs + AR934X_DDR_REG_DDR2_CONFIG);
#else
	writel(CONFIG_ATH79_DDR_DDR2_CFG_REG_VAL,
					ddr_regs + AR934X_DDR_REG_DDR2_CONFIG);
#endif
	} else {
		ath79_reg_read_clear(ddr_regs + AR934X_DDR_REG_CTL_CONF,
						DDR_CTRL_CFG_PAD_DDR2_SEL_MASK);
	}

	/* Setup DDR timing related registers */
#ifndef CONFIG_ATH79_DDR_CFG_REG_VAL
	reg = readl(ddr_regs);

	/* Always use page close policy */
	reg = reg | DDR_CFG_PAGE_CLOSE_MASK;

	/* CAS should be (2 * CAS_LAT) or (2 * CAS_LAT) + 1/2/3 */
	tmp = 2 * cas_lat;
	tmp = (tmp << DDR_CFG_CAS_3LSB_SHIFT) & DDR_CFG_CAS_3LSB_MASK;
	if (cas_lat > 3) {
		tmp = tmp | DDR_CFG_CAS_MSB_MASK;
	}

	reg = reg & ~DDR_CFG_CAS_3LSB_MASK;
	reg = reg | tmp;

	/*
	 * Calculate rest of timing related values,
	 * always round up to closest integer
	 */

	/* tMRD */
	tmp = ((DDRx_tMRD_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tMRD)
		tmp = MAX_tMRD;

	tmp = (tmp << DDR_CFG_TMRD_SHIFT) & DDR_CFG_TMRD_MASK;
	reg = reg & ~DDR_CFG_TMRD_MASK;
	reg = reg | tmp;

	/* tRFC */
	if (mem_type == AR934X_DDR2) {
		tmp = ((DDR2_tRFC_ns * ddr_mhz) + 500) / 1000;
	} else {
		tmp = ((DDR1_tRFC_ns * ddr_mhz) + 500) / 1000;
	}

	if (tmp > MAX_tRFC)
		tmp = MAX_tRFC;

	tmp = (tmp << DDR_CFG_TRFC_SHIFT) & DDR_CFG_TRFC_MASK;
	reg = reg & ~DDR_CFG_TRFC_MASK;
	reg = reg | tmp;

	/* tRRD */
	tmp = ((DDRx_tRRD_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tRRD)
		tmp = MAX_tRRD;

	tmp = (tmp << DDR_CFG_TRRD_SHIFT) & DDR_CFG_TRRD_MASK;
	reg = reg & ~DDR_CFG_TRRD_MASK;
	reg = reg | tmp;

	/* tRP */
	tmp = ((DDRx_tRP_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tRP)
		tmp = MAX_tRP;

	tmp = (tmp << DDR_CFG_TRP_SHIFT) & DDR_CFG_TRP_MASK;
	reg = reg & ~DDR_CFG_TRP_MASK;
	reg = reg | tmp;

	/* tRCD */
	tmp = ((DDRx_tRCD_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tRCD)
		tmp = MAX_tRCD;

	tmp = (tmp << DDR_CFG_TRCD_SHIFT) & DDR_CFG_TRCD_MASK;
	reg = reg & ~DDR_CFG_TRCD_MASK;
	reg = reg | tmp;

	/* tRAS */
	tmp = ((DDRx_tRAS_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tRAS)
		tmp = MAX_tRAS;

	tmp = (tmp << DDR_CFG_TRAS_SHIFT) & DDR_CFG_TRAS_MASK;
	reg = reg & ~DDR_CFG_TRAS_MASK;
	reg = reg | tmp;

	writel(reg, ddr_regs);
#else
	writel(CONFIG_ATH79_DDR_CFG_REG_VAL, ddr_regs);
#endif
#ifndef CONFIG_ATH79_DDR_CFG2_REG_VAL
	reg = readl(ddr_regs + AR71XX_DDR_REG_CONFIG2);

	/* Enable CKE */
	reg = reg | DDR_CFG2_CKE_MASK;

	/* Gate open latency = 2 * CAS_LAT */
	tmp = 2 * cas_lat;
	tmp = (tmp << DDR_CFG2_GATE_OPEN_LAT_SHIFT)
					& DDR_CFG2_GATE_OPEN_LAT_MASK;
	reg = reg & ~DDR_CFG2_GATE_OPEN_LAT_MASK;
	reg = reg | tmp;

	/* tWTR */
	if (mem_type == AR934X_DDR2) {
		/* tWTR = 2 * WL + BL + 2 * max(tWTR/tCK, 2) */
		tmp = 2 * (cas_lat + DDR_addit_lat - 1) + DDR_burst_len + 4;

		if (ddr_mhz >= 600)
			tmp = tmp + 2;
	} else {
		/* tWTR = 2 + BL + (2 * tWTR/tCK) */
		tmp = 2 + DDR_burst_len
					+ (((DDRx_tWTR_ns * ddr_mhz) + 500) / 1000);
	}

	if (tmp > MAX_tWTR)
		tmp = MAX_tWTR;

	tmp = (tmp << DDR_CFG2_TWTR_SHIFT) & DDR_CFG2_TWTR_MASK;
	reg = reg & ~DDR_CFG2_TWTR_MASK;
	reg = reg | tmp;

	/* tRTP */
	if (ddr_width == 32) {
		tmp = DDR_burst_len;
	} else {
		tmp = MAX_tRTP;
	}

	tmp = (tmp << DDR_CFG2_TRTP_SHIFT) & DDR_CFG2_TRTP_MASK;
	reg = reg & ~DDR_CFG2_TRTP_MASK;
	reg = reg | tmp;

	/* tRTW */
	if (mem_type == AR934X_DDR2) {
		/* tRTW = 2 * (RL + BL/2 + 1 -WL), RL = CL + AL, WL = RL - 1 */
		tmp = DDR_burst_len + 4;
	} else {
		/* tRTW = 2 * (CL + BL/2) */
		tmp = DDR_burst_len + (2 * cas_lat);
	}

	if (tmp > MAX_tRTW)
		tmp = MAX_tRTW;

	tmp = (tmp << DDR_CFG2_TRTW_SHIFT) & DDR_CFG2_TRTW_MASK;
	reg = reg & ~DDR_CFG2_TRTW_MASK;
	reg = reg | tmp;

	/* tWR */
	tmp = ((DDRx_tWR_ns * ddr_mhz) + 500) / 1000;
	if (tmp > MAX_tWR)
		tmp = MAX_tWR;

	tmp = (tmp << DDR_CFG2_TWR_SHIFT) & DDR_CFG2_TWR_MASK;
	reg = reg & ~DDR_CFG2_TWR_MASK;
	reg = reg | tmp;

	/* Always use burst length = 8 and type: sequential */
	tmp = (DDR_burst_len << DDR_CFG2_BURST_LEN_SHIFT)
					& DDR_CFG2_BURST_LEN_MASK;
	reg = reg & ~(DDR_CFG2_BURST_LEN_MASK
					| DDR_CFG2_BURST_TYPE_MASK);
	reg = reg | tmp;

	writel(reg, ddr_regs + AR71XX_DDR_REG_CONFIG2);
#else
	writel(CONFIG_ATH79_DDR_CFG2_REG_VAL,
					ddr_regs + AR71XX_DDR_REG_CONFIG2);
#endif

	/* Precharge all */
	writel(DDR_CTRL_FORCE_PRECHRG_ALL_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);

	if (mem_type == AR934X_DDR2) {
		/* Setup target EMR2 and EMR3 */
		writel(_ddr_sdram_emr2_val(0, 0, 0),
						ddr_regs + AR934X_DDR_REG_EMR2);
		writel(DDR_CTRL_FORCE_EMR2S_MASK,
						ddr_regs + AR71XX_DDR_REG_CONTROL);
		writel(0, ddr_regs + AR934X_DDR_REG_EMR3);
		writel(DDR_CTRL_FORCE_EMR3S_MASK,
						ddr_regs + AR71XX_DDR_REG_CONTROL);
	}

	/* Enable and reset DLL */
	writel(_ddr_sdram_emr_val(0, 1, 0, 0, 0, 0),
					ddr_regs + AR71XX_DDR_REG_EMR);
	writel(DDR_CTRL_FORCE_EMRS_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);
	writel(_ddr_sdram_mr_val(0, 0, 1, 0),
					ddr_regs + AR71XX_DDR_REG_MODE);
	writel(DDR_CTRL_FORCE_MRS_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);

	/* Precharge all, 2x auto refresh */
	writel(DDR_CTRL_FORCE_PRECHRG_ALL_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);

	writel(DDR_CTRL_FORCE_AUTO_REFRH_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);
	writel(DDR_CTRL_FORCE_AUTO_REFRH_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);

	if (mem_type == AR934X_DDR2) {
		/* Setup target MR */
		wr_recovery = ((DDRx_tWR_ns * tmp_clk) + 1000) / 2000;
		writel(_ddr_sdram_mr_val(0, cas_lat, 0, wr_recovery),
					ddr_regs + AR71XX_DDR_REG_MODE);
		writel(DDR_CTRL_FORCE_MRS_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);

		/* OCD calibration, target EMR (nDQS disable, weak strength) */
		writel(_ddr_sdram_emr_val(0, 1, DDR_SDRAM_EMR_OCD_DEFAULT_VAL,
					1, 0, 0), ddr_regs + AR71XX_DDR_REG_EMR);
		writel(DDR_CTRL_FORCE_EMRS_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);

		writel(_ddr_sdram_emr_val(0, 1, DDR_SDRAM_EMR_OCD_EXIT_VAL,
					1, 0, 0), ddr_regs + AR71XX_DDR_REG_EMR);
		writel(DDR_CTRL_FORCE_EMRS_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);
	} else {
		/* Setup target MR */
		writel(_ddr_sdram_mr_val(0, cas_lat, 0, 0),
					ddr_regs + AR71XX_DDR_REG_MODE);
		writel(DDR_CTRL_FORCE_MRS_MASK,
					ddr_regs + AR71XX_DDR_REG_CONTROL);
	}

	/* Enable DDR refresh and setup refresh period */
	reg = ath79_get_bootstrap();
	if (reg & AR934X_BOOTSTRAP_REF_CLK_40)
		writel(BIT(14) | 312, ddr_regs + AR71XX_DDR_REG_REFRESH);
	else
		writel(BIT(14) | 195, ddr_regs + AR71XX_DDR_REG_REFRESH);

	/*
	 * At this point memory should be fully configured,
	 * so we can perform delay tap controller tune.
	 */
}

void ddr_tap_tuning(void)
{
	void __iomem *regs;
	u32 *addr_k0, *addr_k1, *addr;
	u32 val, tap, upper, lower;
	int i, j, dir, err, done;

	regs = map_physmem(AR71XX_DDR_CTRL_BASE, AR71XX_DDR_CTRL_SIZE,
			   MAP_NOCACHE);

	/* Init memory pattern */
	addr = (void *)CKSEG0ADDR(0x2000);
	for (i = 0; i < 256; i++) {
		val = 0;
		for (j = 0; j < 8; j++) {
			if (i & (1 << j)) {
				if (j % 2)
					val |= 0xffff0000;
				else
					val |= 0x0000ffff;
			}

			if (j % 2) {
				*addr++ = val;
				val = 0;
			}
		}
	}

	err = 0;
	done = 0;
	dir = 1;
	tap = readl(regs + AR71XX_DDR_REG_TAP_CTRL0);
	val = tap;
	upper = tap;
	lower = tap;
	while (!done) {
		err = 0;

		/* Update new DDR tap value */
		writel(val, regs + AR71XX_DDR_REG_TAP_CTRL0);
		writel(val, regs + AR71XX_DDR_REG_TAP_CTRL1);

		/* Compare DDR with cache */
		for (i = 0; i < 2; i++) {
			addr_k1 = (void *)CKSEG1ADDR(0x2000);
			addr_k0 = (void *)CKSEG0ADDR(0x2000);
			addr = (void *)CKSEG0ADDR(0x3000);

			while (addr_k0 < addr) {
				if (*addr_k1++ != *addr_k0++) {
					err = 1;
					break;
				}
			}

			if (err)
				break;
		}

		if (err) {
			/* Save upper/lower threshold if error  */
			if (dir) {
				dir = 0;
				val--;
				upper = val;
				val = tap;
			} else {
				val++;
				lower = val;
				done = 1;
			}
		} else {
			/* Try the next value until limitation */
			if (dir) {
				if (val < 0x20) {
					val++;
				} else {
					dir = 0;
					upper = val;
					val = tap;
				}
			} else {
				if (!val) {
					lower = val;
					done = 1;
				} else {
					val--;
				}
			}
		}
	}

	/* compute an intermediate value and write back */
	val = (upper + lower) / 2;
	writel(val, regs + AR71XX_DDR_REG_TAP_CTRL0);
	val++;
	writel(val, regs + AR71XX_DDR_REG_TAP_CTRL1);
}
