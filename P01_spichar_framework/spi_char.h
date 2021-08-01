#ifndef SPI_CHAR_H
#define SPI_CHAR_H

#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define ENTER() printk("\n###### In %s ######\n", __func__);

struct omap2_mcspi {
    /* for providing a character device access */
    struct class *spi_class;
    dev_t devt;
    struct cdev cdev;
};


int spi_rw(struct omap2_mcspi *mcspi, char *buff);

int chrdev_init(struct omap2_mcspi *lmcspi);
void chrdev_exit(void);

#endif
