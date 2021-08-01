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

#define DMA_MIN_BYTES			25

#define ENTER() printk("\n###### In %s ######\n", __func__);

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

static int omap2_mcspi_request_dma(struct spi_device *spi)
{
	struct spi_master	*master = spi->master;
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;
	int ret = 0;

	mcspi = spi_master_get_devdata(master);
	mcspi_dma = mcspi->dma_channels + spi->chip_select;

	init_completion(&mcspi_dma->dma_rx_completion);
	init_completion(&mcspi_dma->dma_tx_completion);
	// TODO 4.1.1: Request DMA channel for RX with dma_request_chan 
	// and assign it to dma_rx of mcspi_dma. Refer include/linux/dmaengine.h for API
	if (IS_ERR(mcspi_dma->dma_rx)) {
		ret = PTR_ERR(mcspi_dma->dma_rx);
		mcspi_dma->dma_rx = NULL;
		printk("Failed to get RX DMA\n");
		goto no_dma;
	}

	// TODO 4.1.2: Request DMA channel for RX with dma_request_chan 
	// and assign it to dma_tx of mcspi_dma
	if (IS_ERR(mcspi_dma->dma_tx)) {
		ret = PTR_ERR(mcspi_dma->dma_tx);
		mcspi_dma->dma_tx = NULL;
		dma_release_channel(mcspi_dma->dma_rx);
		mcspi_dma->dma_rx = NULL;
		printk("Failed to get TX DMA\n");
	}

no_dma:
	return ret;
}

static int omap2_mcspi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma	*mcspi_dma;
	u32 l = 0, div = 0;
    u8 word_len = spi->bits_per_word;
	u32 speed_hz = spi->max_speed_hz;
	int ret = 0;

	mcspi_dma = &mcspi->dma_channels[spi->chip_select];
	ENTER();

	if (!mcspi_dma->dma_rx || !mcspi_dma->dma_tx) {
		ret = omap2_mcspi_request_dma(spi);
		if (ret)
			dev_warn(&spi->dev, "not using DMA for McSPI (%d)\n",
				 ret);
	}

	if (t != NULL && t->bits_per_word)
        word_len = t->bits_per_word;
	if (t && t->speed_hz)
        speed_hz = t->speed_hz;

    speed_hz = min_t(u32, speed_hz, OMAP2_MCSPI_MAX_FREQ);
    div = omap2_mcspi_calc_divisor(speed_hz);

	l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0); 
	// TODO 4.10: Select Data line 0 from reception & Data line 1 for transmission 
	// TODO 4.11: Set the word len as per word_len

	// TODO 4.12: Set the SPIEN state as high during active state
	/* set clock divisor */
	// TODO 4.13: Set the clock divider
	// TODO 4.14: Set the PHA so that the data is latched on odd numbered edges

	// TODO 4.18: Update the chconf0 register
	return 0;
}

static void omap2_mcspi_set_dma_req(const struct spi_device *spi,
		int is_read, int enable)
{
	u32 l, rw;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);

	l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);


	if (is_read) /* 1 is read, 0 write */
		rw = OMAP2_MCSPI_CHCONF_DMAR;
	else
		rw = OMAP2_MCSPI_CHCONF_DMAW;

	if (enable)
		l |= rw;
	else
		l &= ~rw;

	mcspi_write_chconf0(mcspi, l);
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

static void omap2_mcspi_rx_callback(void *data)
{
	struct spi_device *spi = data;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	ENTER();

	/* We must disable the DMA RX request */
	omap2_mcspi_set_dma_req(spi, 1, 0);

	// TODO 4.1.17: Notify the RX Completion with 'complete'	
}

static void omap2_mcspi_tx_callback(void *data)
{
	struct spi_device *spi = data;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	ENTER();

	/* We must disable the DMA TX request */
	omap2_mcspi_set_dma_req(spi, 0, 0);

	// TODO 4.1.16: Notify the TX Completion with 'complete'	
}

static void omap2_mcspi_tx_dma(struct spi_device *spi,
				struct spi_transfer *xfer,
				struct dma_slave_config cfg)
{
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma  *mcspi_dma;
	struct scatterlist sg;

	ENTER();
	mcspi = spi_master_get_devdata(spi->master);
	mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	if (mcspi_dma->dma_tx) {
		struct dma_async_tx_descriptor *tx;
		
		// TODO 4.1.6: Configure the dma channel with dmaengine_slave_config
		sg_init_table(&sg, 1);
        sg_dma_address(&sg) = xfer->tx_dma;
        sg_dma_len(&sg) = xfer->len;

		// TODO 4.1.7: Prepare the descriptor with dmaengine_prep_slave_sg
		if (tx) {
			// TODO 4.1.8: Register the Tx callback
			tx->callback_param = spi;
		// TODO 4.1.9: Submit the descriptor to the DMA Engine
		} else {
			/* FIXME: fall back to PIO? */
		}
	}
	// TODO 4.1.10: Start the DMA Transaction

	omap2_mcspi_set_dma_req(spi, 0, 1);

}

static inline int mcspi_bytes_per_word(int word_len)
{
	if (word_len <= 8)
		return 1;
	else if (word_len <= 16)
		return 2;
	else /* word_len <= 32 */
		return 4;
}

static unsigned
omap2_mcspi_rx_dma(struct spi_device *spi, struct spi_transfer *xfer,
				struct dma_slave_config cfg)
{
	struct omap2_mcspi	*mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma  *mcspi_dma;
	unsigned int count;
	struct scatterlist sg;

	mcspi_dma = &mcspi->dma_channels[spi->chip_select];
	count = xfer->len;

	if (mcspi_dma->dma_rx) {
		struct dma_async_tx_descriptor *tx;

		// TODO 4.1.11: Configure the dma channel with dmaengine_slave_config
		sg_init_table(&sg, 1);
        sg_dma_address(&sg) = xfer->rx_dma;
        sg_dma_len(&sg) = count;
		// TODO 4.1.12: Prepare the descriptor with dmaengine_prep_slave_sg
		if (tx) {
			// TODO 4.1.13: Register the Rx callback
			tx->callback_param = spi;
			// TODO 4.1.14: Submit the descriptor to the DMA Engine
		} else {
				/* FIXME: fall back to PIO? */
		}
	}

	// TODO 4.1.15: Start the DMA Transaction
	omap2_mcspi_set_dma_req(spi, 1, 1);

	wait_for_completion(&mcspi_dma->dma_rx_completion);

	return count;
}

static unsigned
omap2_mcspi_txrx_dma(struct spi_device *spi, struct spi_transfer *xfer)
{
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma  *mcspi_dma;
	unsigned int		count;
	u8			*rx;
	const u8		*tx;
	struct dma_slave_config	cfg;
	enum dma_slave_buswidth width;
	void __iomem		*chstat_reg;
	void __iomem            *irqstat_reg;
	int			wait_res;

	mcspi = spi_master_get_devdata(spi->master);
	mcspi_dma = &mcspi->dma_channels[spi->chip_select];

	ENTER();

	if (mcspi_dma->dma_tx && xfer->tx_buf != NULL) {
		// TODO 4.1.3: Get the physical address for the tx_buf with dma_map_single
		// Assign the return value to xfer->tx_dma
		if (dma_mapping_error(mcspi->dev, xfer->tx_dma)) {
			dev_dbg(mcspi->dev, "dma %cX %d bytes error\n",
					'T', xfer->len);
			return -EINVAL;
		}
	}
	if (mcspi_dma->dma_rx && xfer->rx_buf != NULL) {
		// TODO 4.1.4: Get the physical address for the rx_buf with dma_map_single
		// Assign the return value to xfer->rx_dma
		if (dma_mapping_error(mcspi->dev, xfer->rx_dma)) {
			dev_dbg(mcspi->dev, "dma %cX %d bytes error\n",
					'R', xfer->len);
			if (xfer->tx_buf != NULL)
				dma_unmap_single(mcspi->dev, xfer->tx_dma,
						xfer->len, DMA_TO_DEVICE);
			return -EINVAL;
		}
	}

	width = DMA_SLAVE_BUSWIDTH_1_BYTE;

	count = xfer->len;

	memset(&cfg, 0, sizeof(cfg));
	// TODO 4.1.5: Initialize the DMA configuration Data structure:
	// src address to RX0 register (Use Physical address of RX0)
	// dst address to TX0 register (Use Physical address of TX0)
	// src and dst address width
	// Max burst for src and dst

	rx = xfer->rx_buf;
	tx = xfer->tx_buf;

	if (tx != NULL)
		omap2_mcspi_tx_dma(spi, xfer, cfg);

	if (rx != NULL)
		count = omap2_mcspi_rx_dma(spi, xfer, cfg);

	if (tx != NULL) {
		wait_for_completion(&mcspi_dma->dma_tx_completion);

		if (mcspi->fifo_depth > 0) {
			irqstat_reg = mcspi->base + OMAP2_MCSPI_IRQSTATUS;

			if (mcspi_wait_for_reg_bit(irqstat_reg,
						OMAP2_MCSPI_IRQSTATUS_EOW) < 0)
				dev_err(&spi->dev, "EOW timed out\n");

			mcspi_write_reg(mcspi, OMAP2_MCSPI_IRQSTATUS,
					OMAP2_MCSPI_IRQSTATUS_EOW);
		}

		/* for TX_ONLY mode, be sure all words have shifted out */
		if (rx == NULL) {
			chstat_reg = mcspi->base + OMAP2_MCSPI_CHSTAT0;
			if (mcspi->fifo_depth > 0) {
				wait_res = mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_TXFFE);
				if (wait_res < 0)
					dev_err(&spi->dev, "TXFFE timed out\n");
			} else {
				wait_res = mcspi_wait_for_reg_bit(chstat_reg,
						OMAP2_MCSPI_CHSTAT_TXS);
				if (wait_res < 0)
					dev_err(&spi->dev, "TXS timed out\n");
			}
			if (wait_res >= 0 &&
				(mcspi_wait_for_reg_bit(chstat_reg,
					OMAP2_MCSPI_CHSTAT_EOT) < 0))
				dev_err(&spi->dev, "EOT timed out\n");
		}
	}
	return count;
}

static void omap2_mcspi_set_fifo(const struct spi_device *spi,
				struct spi_transfer *t, int enable)
{
	struct spi_master *master = spi->master;
	struct omap2_mcspi *mcspi;
	unsigned int wcnt;
	int max_fifo_depth, bytes_per_word;
	u32 chconf, xferlevel;

	mcspi = spi_master_get_devdata(master);

	ENTER();

	chconf = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
	if (enable) {
		bytes_per_word = mcspi_bytes_per_word(8);
		if (t->len % bytes_per_word != 0)
			goto disable_fifo;

		if (t->rx_buf != NULL && t->tx_buf != NULL)
			max_fifo_depth = OMAP2_MCSPI_MAX_FIFODEPTH / 2;
		else
			max_fifo_depth = OMAP2_MCSPI_MAX_FIFODEPTH;

		wcnt = t->len / bytes_per_word;
		if (wcnt > OMAP2_MCSPI_MAX_FIFOWCNT)
			goto disable_fifo;

		xferlevel = wcnt << 16;
		if (t->rx_buf != NULL) {
			chconf |= OMAP2_MCSPI_CHCONF_FFER;
			xferlevel |= (bytes_per_word - 1) << 8;
		}

		if (t->tx_buf != NULL) {
			chconf |= OMAP2_MCSPI_CHCONF_FFET;
			xferlevel |= bytes_per_word - 1;
		}

		mcspi_write_reg(mcspi, OMAP2_MCSPI_XFERLEVEL, xferlevel);
		mcspi_write_chconf0(mcspi, chconf);
		mcspi->fifo_depth = max_fifo_depth;

		return;
	}

disable_fifo:
	if (t->rx_buf != NULL)
		chconf &= ~OMAP2_MCSPI_CHCONF_FFER;

	if (t->tx_buf != NULL)
		chconf &= ~OMAP2_MCSPI_CHCONF_FFET;

	mcspi_write_chconf0(mcspi, chconf);
	mcspi->fifo_depth = 0;
}

static void omap2_mcspi_cleanup(struct spi_device *spi)
{
	struct omap2_mcspi	*mcspi;
	struct omap2_mcspi_dma	*mcspi_dma;

	mcspi = spi_master_get_devdata(spi->master);

	if (spi->chip_select < spi->master->num_chipselect) {
		mcspi_dma = &mcspi->dma_channels[spi->chip_select];

		if (mcspi_dma->dma_rx) {
			dma_release_channel(mcspi_dma->dma_rx);
			mcspi_dma->dma_rx = NULL;
		}
		if (mcspi_dma->dma_tx) {
			dma_release_channel(mcspi_dma->dma_tx);
			mcspi_dma->dma_tx = NULL;
		}
	}
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
	struct omap2_mcspi_dma	*mcspi_dma;
	mcspi_dma = mcspi->dma_channels + spi->chip_select;

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

		omap2_mcspi_set_enable(mcspi, 0);
		l = mcspi_read_reg(mcspi, OMAP2_MCSPI_CHCONF0);
		l &= ~OMAP2_MCSPI_CHCONF_TRM_MASK;
		l &= ~OMAP2_MCSPI_CHCONF_TURBO;

		if (t->tx_buf == NULL)
			l |= OMAP2_MCSPI_CHCONF_TRM_RX_ONLY;
		else if (t->rx_buf == NULL)
			l |= OMAP2_MCSPI_CHCONF_TRM_TX_ONLY;
		mcspi_write_chconf0(mcspi, l);


		if (t->len) {
			if ((mcspi_dma->dma_rx && mcspi_dma->dma_tx) &&
					master->cur_msg_mapped &&
					master->can_dma(master, spi, t))
				omap2_mcspi_set_fifo(spi, t, 1);

			// TODO 4.19 Enable the channel 
			// TODO 4.20 Force the Chipselect
			/* RX_ONLY mode needs dummy data in TX reg */
			if (t->tx_buf == NULL)
				__raw_writel(0, mcspi->base
						+ OMAP2_MCSPI_TX0);
			if ((mcspi_dma->dma_rx && mcspi_dma->dma_tx) &&
					master->cur_msg_mapped &&
					master->can_dma(master, spi, t)) {
				count = omap2_mcspi_txrx_dma(spi, t);
			}
			else {
				while (count) {
					if (tx != NULL) {
						// TODO 4.21 Wait for TXS bit to be set
						if (status < 0) {
							printk("TXS timed out\n");
							status = -1;
							break;
						}
						// TODO 4.22 Write into the tx_reg with __raw_writel
					}
					if (rx != NULL) {
						// TODO 4.23 Wait for RXS bit to be set
						if (status < 0) {
							printk("RXS timed out\n");
							status = -1;
							break;
						}
						*rx = __raw_readl(rx_reg);
						printk("rx = %x\t", *rx++);
					}
					count--;
				}
			}
		}
	}
	// TODO 4.24 Disable the cs force
	// TODO 4.25 Disable the channel
	m->status = status;
	spi_finalize_current_message(master);
	return 0;
}

static bool omap2_mcspi_can_dma(struct spi_master *master,
				struct spi_device *spi,
				struct spi_transfer *xfer)
{
	return (xfer->len >= DMA_MIN_BYTES);
}

static void omap2_mcspi_set_master_mode(struct omap2_mcspi *mcspi)
{
    u32 l;
    ENTER();
	
	mcspi_write_reg(mcspi, OMAP2_MCSPI_WAKEUPENABLE,
            OMAP2_MCSPI_WAKEUPENABLE_WKEN);
    l = mcspi_read_reg(mcspi, OMAP2_MCSPI_MODULCTRL);
	//TODO 4.10: Set single channel master mode & put the controller in functional mode 
    mcspi_write_reg(mcspi, OMAP2_MCSPI_MODULCTRL, l);
}

static int my_mcspi_probe(struct platform_device *pdev)
{
    struct omap2_mcspi  *mcspi;
	struct spi_master *master;
    struct resource     *r;
	int status, i;
	struct device_node *node = pdev->dev.of_node;
	
	//TODO 4.5: Allocate the spi master along with mscpi 

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(4, 32);
	//TODO 4.6: Register callback handler for setup 
	master->can_dma = omap2_mcspi_can_dma;
	master->cleanup = omap2_mcspi_cleanup;
	//TODO 4.7: Register callback handler for transfer_one_message 
	master->dev.of_node = node;

    platform_set_drvdata(pdev, master);

	mcspi = spi_master_get_devdata(master);
	mcspi->master = master;

	master->num_chipselect = 1;

	// TODO 4.8 Get the base address from platform device
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (r == NULL) {
        status = -ENODEV;
		goto free_master;
    }
	mcspi->phys = r->start;
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
	mcspi->dma_channels = devm_kcalloc(&pdev->dev, master->num_chipselect, 
			sizeof(struct omap2_mcspi_dma), GFP_KERNEL);

	if (mcspi->dma_channels == NULL) {
		status = -ENOMEM;
		goto free_master;
	}

	for (i = 0; i < master->num_chipselect; i++) {
		sprintf(mcspi->dma_channels[i].dma_rx_ch_name, "rx%d", i);
		sprintf(mcspi->dma_channels[i].dma_tx_ch_name, "tx%d", i);
	}

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
