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

#include <linux/spi/spi.h>
#include <linux/platform_data/spi-omap2-mcspi.h>
#include "low_level_driver.h"

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

static int omap2_mcspi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	u32 l = 0, div = 0;
    u8 word_len = spi->bits_per_word;
	u32 speed_hz = spi->max_speed_hz;
	ENTER();
	if (t != NULL && t->bits_per_word)
        word_len = t->bits_per_word;
	if (t && t->speed_hz)
        speed_hz = t->speed_hz;

    speed_hz = min_t(u32, speed_hz, OMAP2_MCSPI_MAX_FREQ);
    div = omap2_mcspi_calc_divisor(speed_hz);

	l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0); 
	// TODO 4.10: Select Data line 0 from reception & Data line 1 for transmission 
	// TODO 4.11: Set the word len as per word_len
	l &= ~OMAP2_MCSPI_CHCONF_WL_MASK;

	// TODO 4.12: Set the SPIEN state as high during active state
	/* set clock divisor */
	// TODO 4.13: Set the clock divider
	// TODO 4.14: Set the PHA so that the data is latched on odd numbered edges

	// TODO 4.18: Update the chconf0 register
	return 0;
}

static int omap2_mcspi_setup(struct spi_device *spi) {
	return omap2_mcspi_setup_transfer(spi, NULL);
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

static int spi_transfer_one_message(struct spi_master *master,
		struct spi_message *m)
{
	struct omap2_mcspi *mcspi = spi_master_get_devdata(master);
	struct spi_device *spi = m->spi;
	void __iomem        *base = mcspi->base;
    void __iomem        *tx_reg;
    void __iomem        *rx_reg;
    void __iomem        *chstat_reg;
	unsigned int count;
    u8      *rx;
    const u8    *tx;
	struct spi_transfer *t = NULL;
	int status = 0;
	u32 l;

	ENTER();

    /* We store the pre-calculated register addresses on stack to speed
     * up the transfer loop. */
    tx_reg = base + OMAP2_MCSPI_TX0;
    rx_reg = base + OMAP2_MCSPI_RX0;
    chstat_reg = base + OMAP2_MCSPI_CHSTAT0;

	if (list_empty(&m->transfers))
		return -EINVAL;
	list_for_each_entry(t, &m->transfers, transfer_list) {
		if (t->speed_hz || t->bits_per_word) {
			status = omap2_mcspi_setup_transfer(spi, t);
			if (status < 0)
				break;
		}

		count = t->len;
		rx = t->rx_buf;
		tx = t->tx_buf;

		if (t->len) {
			while (count) {
				l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
				l &= ~OMAP2_MCSPI_CHCONF_TRM_MASK;
				l &= ~OMAP2_MCSPI_CHCONF_TURBO;

				if (t->tx_buf == NULL)
					l |= OMAP2_MCSPI_CHCONF_TRM_RX_ONLY;
				else if (t->rx_buf == NULL)
					l |= OMAP2_MCSPI_CHCONF_TRM_TX_ONLY;

				mcspi_write_chconf0(mcspi, l);

				// TODO 4.19 Enable the channel 
				// TODO 4.20 Force the Chipselect
				/* RX_ONLY mode needs dummy data in TX reg */
				if (t->tx_buf == NULL)
					__raw_writel(0, mcspi->base
                        + OMAP2_MCSPI_TX0);

				if (tx != NULL) {
					// TODO 4.21 Wait for TXS bit to be set
					if (status < 0) {
						printk("TXS timed out\n");
						break;
					}
					// TODO 4.22 Write into the tx_reg with __raw_writel
				}
				if (rx != NULL) {
					// TODO 4.23 Wait for RXS bit to be set
					if (status < 0) {
						printk("RXS timed out\n");
						break;
					}
					*rx = __raw_readl(rx_reg);
					printk("rx = %x\t", *rx++);
				}
				count--;
			}
		}
	}

	// TODO 4.24 Disable the cs force
	// TODO 4.25 Disable the channel
	m->status = status;
	spi_finalize_current_message(master);
	return 0;
}

static void omap2_mcspi_set_master_mode(struct omap2_mcspi *mcspi)
{
    u32 l;
    ENTER();
	
	mcspi_write_reg(mcspi, OMAP2_MCSPI_WAKEUPENABLE,
            OMAP2_MCSPI_WAKEUPENABLE_WKEN);
    l = mcspi_read_reg(mcspi, OMAP2_MCSPI_MODULCTRL);
	//TODO 4.10: Set single channel master mode & put the controller in functional mode 
    l &= ~(OMAP2_MCSPI_MODULCTRL_STEST | OMAP2_MCSPI_MODULCTRL_MS);
    l |= OMAP2_MCSPI_MODULCTRL_SINGLE;
    mcspi_write_reg(mcspi, OMAP2_MCSPI_MODULCTRL, l);
}

static int my_mcspi_probe(struct platform_device *pdev)
{
    struct omap2_mcspi  *mcspi;
	struct spi_master *master;
    struct resource *r = NULL;
	int status;
	struct device_node *node = pdev->dev.of_node;
	
	//TODO 4.5: Allocate the spi master along with mscpi 

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(4, 32);
	//TODO 4.6: Register callback handler for setup 
	//TODO 4.7: Register callback handler for transfer_one_message 
	master->dev.of_node = node;

    platform_set_drvdata(pdev, master);

	mcspi = spi_master_get_devdata(master);
	mcspi->master = master;

	// TODO 4.8 Get the base address from platform device
    if (r == NULL) {
        status = -ENODEV;
		goto free_master;
    }
	/*
     * TODO 4.9: Get the virtual address for the spi0 base address and store it
     * in 'base' field of mcspi. Add the offset of 0x100 to base address in trm
     * Use API void __iomem* ioremap((resource_size_t offset, unsigned long size)
    */
    if (IS_ERR(mcspi->base)) {
        printk(KERN_ERR "Unable to ioremap\n");
        status = PTR_ERR(mcspi->base);
		goto free_master;
    }
	mcspi->dev = &pdev->dev;

	omap2_mcspi_set_master_mode(mcspi);
	status = spi_register_master(master);
	if (status < 0)
         goto free_master;

    return status;

free_master:
	spi_master_put(master);
	return status;
}

static int my_mcspi_remove(struct platform_device *pdev)
{
	struct spi_master *master;
	struct omap2_mcspi *mcspi;
	master = platform_get_drvdata(pdev);
	mcspi = spi_master_get_devdata(master);
	spi_unregister_master(master);

	return 0;
}

//TODO 4.4 Populate the id table with compatible property as per dtb
static const struct of_device_id my_mcspi_of_match[] = {
    {
    },
    { },
};

//TODO 4.3 Populate the platform driver structure
// Hint: Refer gpio_dtb.c
static struct platform_driver my_mcspi_driver = {
};

static int __init omap_spi_init_driver(void)
{
	//TODO 4.1 Register the platform driver
}

static void __exit omap_spi_exit_driver(void)
{
	//TODO 4.2 De-register the platform driver
}
module_init(omap_spi_init_driver);
module_exit(omap_spi_exit_driver);

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Low level SPI driver");
MODULE_LICENSE("GPL");
