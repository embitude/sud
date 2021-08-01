#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

struct dummy_data {
	struct spi_device *spi;
	struct spi_message msg;
	struct spi_transfer transfer[2];
	u8 tx_buf;
	u8 rx_buf[2];
	/* Character Driver Files */
	dev_t devt;
	struct cdev cdev;
	struct class *class;
};

static ssize_t dummy_read(struct file* f, char *buf, size_t count, loff_t *f_pos)
{
	struct dummy_data *dev = (struct dummy_data *)(f->private_data);
	int ret = -1;

	if (*f_pos == 0) {
		dev->tx_buf = 3;
		//TODO 7: Initiate the spi transaction

		if (ret < 0)
			return ret;
		//TODO 8: Exchange the rx_buf data with user space 

		*f_pos = 1;
		return 2;
	} 
	else {
		*f_pos = 0;
		return 0;
	}

	return 0;
}

static int dummy_close(struct inode *i, struct file *file)
{
	return 0;
}

static int dummy_open(struct inode *i, struct file *f)
{
	struct dummy_data *dev = container_of(i->i_cdev, struct dummy_data, cdev);
	if (dev == NULL) {
		printk("Data is null\n");
		return -1;
	}
	f->private_data = dev;

	return 0;
}

struct file_operations fops = {
	.open = dummy_open,
	.release = dummy_close,
	.read = dummy_read,
};

static int dummy_probe(struct spi_device *spi)
{
	struct dummy_data *data;
	int init_result;
	struct device *dev_ret = NULL;

	data = devm_kzalloc(&spi->dev, sizeof(struct dummy_data), GFP_KERNEL);
	data->spi = spi;

	//TODO 3: Assign the tx_buf and rx_buf of dummy_data to corresponding fields of transfer DS
	
	//TODO 4: Initialize the data->msg with transfer structures

	spi_set_drvdata(spi, data);

	init_result = alloc_chrdev_region(&data->devt, 0, 1, "spi_dmy");

	if (0 > init_result)
	{
		printk(KERN_ALERT "Device Registration failed\n");
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	printk("Major Nr: %d\n", MAJOR(data->devt));

	if ((data->class = class_create(THIS_MODULE, "spidummy")) == NULL)
	{
		printk( KERN_ALERT "Class creation failed\n" );
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	//TODO 5: Create the device file with name spi_dmy0
	if (dev_ret == NULL)
	{
		printk( KERN_ALERT "Device creation failed\n" );
		class_destroy(data->class);
		unregister_chrdev_region(data->devt, 1);
		return -1;
	}
	
	cdev_init(&data->cdev, &fops);

	//TODO 6: Register the file ops
	if (init_result == -1)
	{
		printk( KERN_ALERT "Device addition failed\n" );
		device_destroy(data->class, data->devt);
		class_destroy(data->class);
		unregister_chrdev_region(data->devt, 1 );
		return -1;
	}
	return 0;
}

static int dummy_remove(struct spi_device *spi)
{
	struct dummy_data *data = spi_get_drvdata(spi);
	// TODO 9: Delete the device file & the class
    // TODO 10: Unregister file operations
    // TODO 11: Unregister character driver

	return 0;
}

//TODO 2: Populate the id table as per dtb
static const struct spi_device_id dummy_id[] = {
	{ },
	{ }
};
MODULE_DEVICE_TABLE(spi, dummy_id);

//TODO 1: Populate the spi_driver data structure
static struct spi_driver dummy_driver = {
};
module_spi_driver(dummy_driver);

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Dummy Client driver");
MODULE_LICENSE("GPL v2");
