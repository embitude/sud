#ifndef SPI_CHAR_H
#define SPI_CHAR_H

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include "low_level_driver.h"

#define ENTER() printk("\n###### In %s ######\n", __func__);

int chrdev_init(struct omap2_mcspi *lmcspi);
void chrdev_exit(void);

#endif
