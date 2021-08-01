#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/platform_data/serial-omap.h>
#include "spi_char.h"

#define FIRST_MINOR 0
#define MINOR_CNT 1

static struct omap2_mcspi *mcspi;

static int my_open(struct inode *i, struct file *f)
{
	return omap2_mcspi_setup_transfer(mcspi, NULL);
}
static int my_close(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	// TODO 8: Invoke the low level TX/RX function
	return 0;
}

static struct file_operations driver_fops =
{
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read = my_read,
};

int chrdev_init(struct omap2_mcspi *lmcspi)
{
	int ret;
	struct device *dev_ret = NULL;
	mcspi = lmcspi;

	if ((ret = alloc_chrdev_region(&mcspi->devt, FIRST_MINOR, MINOR_CNT, "spi_driver")) < 0)
	{
		return ret;
	}

	cdev_init(&mcspi->cdev, &driver_fops);

    // TODO 3: Register the file_operations
	if (ret < 0)
	{
		unregister_chrdev_region(mcspi->devt, MINOR_CNT);
		return ret;
	}
	
	if (IS_ERR(lmcspi->spi_class = class_create(THIS_MODULE, "spi")))
	{
		cdev_del(&mcspi->cdev);
		unregister_chrdev_region(mcspi->devt, MINOR_CNT);
		return PTR_ERR(lmcspi->spi_class);
	}
	// TODO 4: Create the device file with name spi0
	if (IS_ERR(dev_ret))
	{
		class_destroy(mcspi->spi_class);
		cdev_del(&mcspi->cdev);
		unregister_chrdev_region(mcspi->devt, MINOR_CNT);
		return PTR_ERR(dev_ret);
	}

	return 0;
}

void chrdev_exit(void)
{
	// TODO 5: Delete the device file & the class
    // TODO 6: Unregister file operations
    // TODO 7: Unregister character driver
}
