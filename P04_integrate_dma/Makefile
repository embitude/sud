# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)

	#KERNEL_SOURCE := <Kernel source path>
	KERNEL_SOURCE := /home/expert/BbbExp/linux-4.19.103
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	obj-m := spi.o spi_dma.o
	spi-y := low_level_driver.o

endif
