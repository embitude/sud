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

struct dummy_master {
	struct spi_master	*master;
	/* Virtual base address of the controller */
	void __iomem		*base;
	unsigned long		phys;
	struct device		*dev;
};

static int dummy_spi_setup(struct spi_device *spi)
{
	printk("@@@ Dummy setup invoked @@@\n");

	return 0;
}

static void dummy_spi_cleanup(struct spi_device *spi)
{
	printk("@@@ Dummy clean up invoked @@@\n");
}
static int dummy_transfer_one_message(struct spi_master *master,
		struct spi_message *m)
{
	int status = 0;
	char *buf;
	struct spi_transfer *t = NULL;
	printk("@@@ Dummy transfer_one_message invoked @@@\n");

	list_for_each_entry(t, &m->transfers, transfer_list) {
        if (t->tx_buf == NULL && t->rx_buf == NULL && t->len) {
            status = -EINVAL;
            break;
        }
		//TODO 8: Print the contents of tx_buff and fill the rx_buff
	}
	m->status = status;
	spi_finalize_current_message(master);
	return 0;
}

//TODO 2: Populate the device-id table. Compatible property should match with dtb
static const struct of_device_id dummy_of_match[] = {
	{
	},
	{ }, /* Null for termination */
};
MODULE_DEVICE_TABLE(of, dummy_of_match);

static int dummy_spi_probe(struct platform_device *pdev)
{
	struct spi_master	*master = NULL;
	struct dummy_master	*mcspi;
	int	status = 0;
	struct device_node	*node = pdev->dev.of_node;

	printk("@@@ Dummy SPI Probe invoked @@@\n");
	//TODO 3: Allocate the spi master along with mscpi 
	if (master == NULL) {
		dev_dbg(&pdev->dev, "master allocation failed\n");
		return -ENOMEM;
	}

	/* the spi->mode bits understood by this driver: */
	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH;
	master->bits_per_word_mask = SPI_BPW_RANGE_MASK(4, 32);
	//TODO 4: Register callback handler for setup 
	//TODO 5: Register callback handler for transfer_one_message 
	//TODO 6: Register callback handler for clean up 

	master->dev.of_node = node;

	platform_set_drvdata(pdev, master);

	mcspi = spi_master_get_devdata(master);
	mcspi->master = master;

	//TODO 7: Register the spi master	
	return status;
}

static int dummy_spi_remove(struct platform_device *pdev)
{
	struct spi_master	*master;
	struct omap2_mcspi	*mcspi;

	master = platform_get_drvdata(pdev);
	mcspi = spi_master_get_devdata(master);
	//TODO 9: Unregister the master

	return 0;
}

//TODO 1: Populate the platform driver structure
static struct platform_driver dummy_spi_driver = {
};

module_platform_driver(dummy_spi_driver);
MODULE_LICENSE("GPL");
