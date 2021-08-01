# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := <Kernel source path>
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	obj-m := serial.o serial_intr.o serial_intr_multiple.o
	serial-y := char_serial.o low_level_driver.o
	serial_intr-y := uart_intr.o char_serial.o
	serial_intr_multiple-y := uart_intr_multiple.o char_serial.o

endif
