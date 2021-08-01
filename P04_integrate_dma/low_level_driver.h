#ifndef LOW_LEVEL_DRIVER_H
#define LOW_LEVEL_DRIVER_H

#define OMAP2_MCSPI_MAX_FREQ        48000000
#define OMAP2_MCSPI_MAX_FIFODEPTH   64
#define OMAP2_MCSPI_MAX_FIFOWCNT    0xFFFF
#define SPI_AUTOSUSPEND_TIMEOUT     2000

#define OMAP2_MCSPI_REVISION        0x00
#define OMAP2_MCSPI_SYSCONFIG       0x10
#define OMAP2_MCSPI_SYSSTATUS       0x14
#define OMAP2_MCSPI_IRQSTATUS       0x18
#define OMAP2_MCSPI_IRQENABLE       0x1c
#define OMAP2_MCSPI_WAKEUPENABLE    0x20
#define OMAP2_MCSPI_SYST        0x24
#define OMAP2_MCSPI_MODULCTRL       0x28
#define OMAP2_MCSPI_XFERLEVEL       0x7c
#define OMAP_MCSPI_SYS_RESET        0x02
#define SYSS_RESETDONE_MASK     0x01
/* per-channel banks, 0x14 bytes each, first is: */
#define OMAP2_MCSPI_CHCONF0     0x2c
#define OMAP2_MCSPI_CHSTAT0     0x30
#define OMAP2_MCSPI_CHCTRL0     0x34
#define OMAP2_MCSPI_TX0         0x38
#define OMAP2_MCSPI_RX0         0x3c

/* per-register bitmasks: */
#define OMAP2_MCSPI_IRQSTATUS_EOW   BIT(17)

#define OMAP2_MCSPI_MODULCTRL_SINGLE    BIT(0)
#define OMAP2_MCSPI_MODULCTRL_MS    BIT(2)
#define OMAP2_MCSPI_MODULCTRL_STEST BIT(3)

#define OMAP2_MCSPI_CHCONF_PHA      BIT(0)
#define OMAP2_MCSPI_CHCONF_POL      BIT(1)
#define OMAP2_MCSPI_CHCONF_CLKD_MASK    (0x0f << 2)
#define OMAP2_MCSPI_CHCONF_EPOL     BIT(6)
#define OMAP2_MCSPI_CHCONF_WL_MASK  (0x1f << 7)
#define OMAP2_MCSPI_CHCONF_TRM_RX_ONLY  BIT(12)
#define OMAP2_MCSPI_CHCONF_TRM_TX_ONLY  BIT(13)
#define OMAP2_MCSPI_CHCONF_TRM_MASK (0x03 << 12)
#define OMAP2_MCSPI_CHCONF_DMAW     BIT(14)
#define OMAP2_MCSPI_CHCONF_DMAR     BIT(15)
#define OMAP2_MCSPI_CHCONF_DPE0     BIT(16)
#define OMAP2_MCSPI_CHCONF_DPE1     BIT(17)
#define OMAP2_MCSPI_CHCONF_IS       BIT(18)
#define OMAP2_MCSPI_CHCONF_TURBO    BIT(19)
#define OMAP2_MCSPI_CHCONF_FORCE    BIT(20)
#define OMAP2_MCSPI_CHCONF_FFET     BIT(27)
#define OMAP2_MCSPI_CHCONF_FFER     BIT(28)

#define OMAP2_MCSPI_CHSTAT_RXS      BIT(0)
#define OMAP2_MCSPI_CHSTAT_TXS      BIT(1)
#define OMAP2_MCSPI_CHSTAT_EOT      BIT(2)
#define OMAP2_MCSPI_CHSTAT_TXFFE    BIT(3)

#define OMAP2_MCSPI_CHCTRL_EN       BIT(0)

#define OMAP2_MCSPI_WAKEUPENABLE_WKEN   BIT(0)

#define ENTER() printk("\n###### In %s ######\n", __func__);

struct omap2_mcspi_dma {
	struct dma_chan *dma_tx;
	struct dma_chan *dma_rx;

	struct completion dma_tx_completion;
	struct completion dma_rx_completion;

	char dma_rx_ch_name[14];
	char dma_tx_ch_name[14];
};

struct omap2_mcspi {
	struct spi_master *master;
	/* Virtual base address of the controller */
	void __iomem	*base;
	unsigned long		phys;
	struct omap2_mcspi_dma	*dma_channels;
	struct device   *dev;
	int	fifo_depth;
};

#endif
