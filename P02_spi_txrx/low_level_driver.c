#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/omap-dma.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gcd.h>
#include "spi_char.h"

#include <linux/spi/spi.h>
#include <linux/platform_data/spi-omap2-mcspi.h>
#include "low_level_driver.h"

struct omap2_mcspi mcspi;

static inline void mcspi_write_reg(struct omap2_mcspi *mcspi,
        int idx, u32 val)
{
    __raw_writel(val, mcspi->base + idx);
}

static inline u32 mcspi_read_reg(struct omap2_mcspi *mcspi, int idx)
{
    return __raw_readl(mcspi->base + idx);
}

static inline void mcspi_write_chconf0(struct omap2_mcspi *mcspi, u32 val)
{
    mcspi_write_reg(mcspi, OMAP2_MCSPI_CHCONF0, val);
    mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
}

static int mcspi_wait_for_reg_bit(void __iomem *reg, unsigned long bit)
{
    unsigned long timeout;

    timeout = jiffies + msecs_to_jiffies(1000);
    while (!(__raw_readl(reg) & bit)) {
        if (time_after(jiffies, timeout)) {
            if (!(__raw_readl(reg) & bit))
                return -ETIMEDOUT;
            else
                return 0;
        }
        cpu_relax();
    }
    return 0;
}

static void omap2_mcspi_force_cs(struct omap2_mcspi *mcspi, int cs_active)
{
    u32 l;
    ENTER();

    l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
    if (cs_active)
        l |= OMAP2_MCSPI_CHCONF_FORCE;
    else
        l &= ~OMAP2_MCSPI_CHCONF_FORCE;

    mcspi_write_chconf0(mcspi, l);
}

static u32 omap2_mcspi_calc_divisor(u32 speed_hz)
{
    u32 div;
    ENTER();

    for (div = 0; div < 15; div++)
        if (speed_hz >= (OMAP2_MCSPI_MAX_FREQ >> div))
            return div;
    return 15;
}

int omap2_mcspi_setup_transfer(struct omap2_mcspi *mcspi,
		struct spi_transfer *t)
{
	u32 l = 0, div = 0;
    u32 speed_hz = 500000;
    u8 word_len = 8;
	ENTER();

    speed_hz = min_t(u32, speed_hz, OMAP2_MCSPI_MAX_FREQ);
    div = omap2_mcspi_calc_divisor(speed_hz);

	l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
	// TODO 2.3: Select Data line 0 for reception & Data line 1 for transmission
	// TODO 2.4: Set the word length as per word_len
	// TODO 2.5: Set the SPIEN state as high during active state

	/* set clock divisor */

    l &= ~OMAP2_MCSPI_CHCONF_CLKD_MASK;
	// TODO 2.6: Set the clock divider
	// TODO 2.7: Set the PHA so that the data is latched on odd numbered edges
	// TODO 2.8: Update the chconf0 register with above values
	return 0;
}

static void omap2_mcspi_set_enable(struct omap2_mcspi *mcspi, int enable)
{
    u32 l;
    ENTER();

    l = enable ? OMAP2_MCSPI_CHCTRL_EN : 0;
    mcspi_write_reg(mcspi, OMAP2_MCSPI_CHCTRL0, l);
    /* Flash post-writes */
    mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCTRL0);
}

int spi_rw(struct omap2_mcspi *mcspi, char *buff)
{
	void __iomem        *base = mcspi->base;
    void __iomem        *tx_reg;
    void __iomem        *rx_reg;
    void __iomem        *chstat_reg;
    u8      rx[5];
    const u8    tx[5] = {0x90, 0x80, 0x50};
    u8 idx = 0;

	ENTER();

    /* We store the pre-calculated register addresses on stack to speed
     * up the transfer loop. */
    tx_reg = base + OMAP2_MCSPI_TX0;
    rx_reg = base + OMAP2_MCSPI_RX0;
    chstat_reg = base + OMAP2_MCSPI_CHSTAT0;

	// TODO 2.9 Enable the channel
	// TODO 2.10 Force the Chipselect
	// TODO 2.11 Wait for TXS bit to be set
	// TODO 2.12 Write into the tx_reg with __raw_writel
	// TODO 2.13 Wait for RXS bit to be set
	// TODO 2.14 Read a bytes of data into the rx buffer
	// TODO 2.15 Disable the cs force
	// TODO 2.16 Disable the channel
	return 0;
}

static void omap2_mcspi_set_master_mode(struct omap2_mcspi *mcspi)
{
    u32 l;
    ENTER();

	mcspi_write_reg(mcspi, OMAP2_MCSPI_WAKEUPENABLE,
            OMAP2_MCSPI_WAKEUPENABLE_WKEN);
    l = mcspi_read_reg(mcspi, OMAP2_MCSPI_MODULCTRL);
	//TODO 2.2: Set single channel master mode & put the controller in functional mode
}

static int __init omap_spi_init_driver(void)
{
	void __iomem    *spi_pad_base;
	/*
     * TODO 2.1: Get the virtual address for the spi0 base address and store it
     * in 'base' field of mcspi. Add the offset of 0x100 to base address in trm
     * Use API void __iomem* ioremap((resource_size_t offset, unsigned long size)
    */
    if (IS_ERR(mcspi.base)) {
        printk(KERN_ERR "Unable to ioremap\n");
        return PTR_ERR(mcspi.base);
    }
	/* Set up the pin mux for the spi0 pins */
	spi_pad_base = ioremap(0x44E10950, 0x10);
    if (IS_ERR(spi_pad_base)) {
        printk(KERN_ERR "Unable to ioremap\n");
        return PTR_ERR(spi_pad_base);
    }
	__raw_writel(0x30, spi_pad_base);
	__raw_writel(0x30, spi_pad_base + 0x4);
	__raw_writel(0x10, spi_pad_base + 0x8);
	__raw_writel(0x10, spi_pad_base + 0xc);

	omap2_mcspi_set_master_mode(&mcspi);

    // TODO 1: Initialize the character driver interface

    return 0;
}

static void __exit omap_spi_exit_driver(void)
{
    // TODO 2: De-initialize the character driver interface
}

module_init(omap_spi_init_driver);
module_exit(omap_spi_exit_driver);

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Low level SPI driver");
MODULE_LICENSE("GPL");
