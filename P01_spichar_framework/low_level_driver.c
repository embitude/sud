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

struct omap2_mcspi mcspi;

int spi_rw(struct omap2_mcspi *mcspi, char *buff)
{
        ENTER();

		return 0;
}

static int __init omap_spi_init_driver(void)
{
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
