/*
 * Driver for OMAP-UART controller.
 * Based on drivers/serial/8250.c
 *
 * Copyright (C) 2010 Texas Instruments.
 *
 * Authors:
 *	Govindraj R	<govindraj.raja@ti.com>
 *	Thara Gopinath	<thara@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Note: This driver is made separate from 8250 driver as we cannot
 * over load 8250 driver with omap platform specific configuration for
 * features like DMA, it makes easier to implement features like DMA and
 * hardware flow control and software flow control configuration with
 * this driver as required for the omap-platform.
 */

#if defined(CONFIG_SERIAL_OMAP_CONSOLE) && defined(CONFIG_MAGIC_SYSRQ)
#define SUPPORT_SYSRQ
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/serial_reg.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/serial_core.h>
#include <linux/irq.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/wait.h>
#include <linux/platform_data/serial-omap.h>
#include "char_serial.h"

#include <dt-bindings/gpio/gpio.h>


#define OMAP_UART_TX_WAKEUP_EN		BIT(7)

/* Feature flags */
#define OMAP_UART_WER_HAS_TX_WAKEUP	BIT(0)


#define DEFAULT_CLK_SPEED 48000000 /* 48Mhz*/

/* SCR register bitmasks */
#define OMAP_UART_SCR_RX_TRIG_GRANU1_MASK		(1 << 7)
#define OMAP_UART_SCR_TX_TRIG_GRANU1_MASK		(1 << 6)
#define OMAP_UART_SCR_TX_EMPTY			(1 << 3)

/* FCR register bitmasks */
#define OMAP_UART_FCR_RX_FIFO_TRIG_MASK			(0x3 << 6)
#define OMAP_UART_FCR_TX_FIFO_TRIG_MASK			(0x3 << 4)


/* WER = 0x7F
 * Enable module level wakeup in WER reg
 */
#define OMAP_UART_WER_MOD_WKUP	0X7F

#define OMAP_UART_TCR_TRIG	0x0F
#define DRIVER_NAME1	"my_uart"
#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

#define to_uart_omap_port(p)	((container_of((p), struct uart_omap_port, port)))

static void
serial_omap_set_termios(struct uart_port *port, struct ktermios *termios,
			struct ktermios *old);

static char start_flg = 0;

static inline unsigned int serial_in(struct uart_omap_port *up, int offset)
{
	offset <<= up->port.regshift;
	return readw(up->port.membase + offset);
}

static inline void serial_out(struct uart_omap_port *up, int offset, int value)
{
	offset <<= up->port.regshift;
	writew(value, up->port.membase + offset);
}
/* 
 * APIs
 * serial_out - Writing the uart registers
 * serial_in  - Reading from the uart registers
 *
 */

/* 
 * Registers & MASK
 * UART_LSR - Line Status Register
 * UART_TX -  Xmit Register
 * UART_LSR_THRE - Transmitter FIFO Empty mask
 * UART_LSR_DR   - RX FIFO Empty mask
 *
 */
static void transmit_chars(struct uart_omap_port *up, unsigned int lsr)
{
	int cnt = 10;
	//TODO 5.11: Wait for TX fifo empty bit to be set (In LSR register)
	while (cnt--)
			msleep(10);
	if (cnt <= 0) {
			printk("TX timed out\n");
			return;
	}
	//TODO 5.12: Write a character into the UART_TX register
}

void serial_omap_start_tx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();
	transmit_chars(up, 0);
}

int serial_read(struct uart_omap_port *up, size_t len)
{
	/* 
	* 1. Wait for UART_LSR_DR to be set
	* 2. Read the data from UART_RX register
	*
	*/
	int cnt = 10;
	//TODO 5.13: Wait for data receive bit (UART_LSR_DR) to be set
	while (cnt--)
			msleep(10);
	if (cnt <= 0) {
			printk("RX timed out\n");
			return -ETIMEDOUT;
	}
	//TODO 5.14: Read & print the data from UART_RX register 
	return 1;
}		

int serial_omap_startup(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	struct ktermios termios;

	if (start_flg)
		return 0;
	start_flg = 1;

	FUNC_ENTER();

	//TODO 5.7: Clear the FIFO buffers and disable them
	serial_out(up, UART_FCR, 0);

	/*
	 * Now, initialize the UART
	 */
	//TODO 5.8: Update the LCR register with 8 bit word length

	/* Disable Interrupts */
	up->ier = 0;
	/* Enable module level wake up */
	up->wer = OMAP_UART_WER_MOD_WKUP;
	if (up->features & OMAP_UART_WER_HAS_TX_WAKEUP)
		up->wer |= OMAP_UART_TX_WAKEUP_EN;

	serial_out(up, UART_OMAP_WER, up->wer);

	memset(&termios, 0, sizeof(struct ktermios));
	termios.c_cflag = CS8 | B115200 | CREAD;
	termios.c_iflag = IGNBRK | IGNPAR;
	serial_omap_set_termios(&up->port, &termios, NULL);
	up->rx_size = 10;
	up->rx_buff = devm_kzalloc(up->dev, up->rx_size, GFP_KERNEL);
	up->tx_size = 10;
	up->tx_buff = devm_kzalloc(up->dev, up->rx_size, GFP_KERNEL);
	return 0;
}

void serial_omap_shutdown(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	start_flg = 0;
	FUNC_ENTER();

	dev_dbg(up->port.dev, "serial_omap_shutdown+%d\n", up->port.line);

	/*
	 * Disable interrupts from this port
	 */
	up->ier = 0;
	serial_out(up, UART_IER, 0);


	/*
	 * Read data port to reset things, and then free the irq
	 */
	if (serial_in(up, UART_LSR) & UART_LSR_DR)
		(void) serial_in(up, UART_RX);

}

static void
serial_omap_set_termios(struct uart_port *port, struct ktermios *termios,
			struct ktermios *old)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned char cval = 0;
	unsigned long flags = 0;
	unsigned int baud, quot;
	FUNC_ENTER();
	printk("Termios Cflag = %x\n", termios->c_cflag);
	printk("Termios iflag = %x\n", termios->c_iflag);

	switch (termios->c_cflag & CSIZE) {
	case CS5:
		cval = UART_LCR_WLEN5;
		break;
	case CS6:
		cval = UART_LCR_WLEN6;
		break;
	case CS7:
		cval = UART_LCR_WLEN7;
		break;
	default:
	case CS8:
		cval = UART_LCR_WLEN8;
		break;
	}

	if (termios->c_cflag & CSTOPB)
		cval |= UART_LCR_STOP;
	if (termios->c_cflag & PARENB)
		cval |= UART_LCR_PARITY;
	if (!(termios->c_cflag & PARODD))
		cval |= UART_LCR_EPAR;
	if (termios->c_cflag & CMSPAR)
		cval |= UART_LCR_SPAR;

	quot = 13;

	up->dll = quot & 0xff;
	up->dlh = quot >> 8;
	up->mdr1 = UART_OMAP_MDR1_DISABLE;

	/*
	 * Ok, we're now changing the port state. Do it with
	 * interrupts disabled.
	 */
	//spin_lock_irqsave(&up->port.lock, flags);
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, cval);		/* reset DLAB */
	up->lcr = cval;
	up->scr = 0;

	/* FIFOs and DMA Settings */

	/* FCR can be changed only when the
	 * baud clock is not running
	 * DLL_REG and DLH_REG set to 0.
	 */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_DLL, 0);
	serial_out(up, UART_DLM, 0);
	serial_out(up, UART_LCR, 0);
	up->fcr = 0;
	up->efr = 0;

	serial_out(up, UART_FCR, up->fcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_OMAP_SCR, up->scr);

	/* Reset UART_MCR_TCRTLR: this must be done with the EFR_ECB bit set */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);

	/* Protocol, Baud Rate, and Interrupt Settings */
	serial_out(up, UART_OMAP_MDR1, up->mdr1);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, 0);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	//TODO 5.9: Update the dll and dlh values in corresponding registers

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, cval);

	//TODO 5.10 Set the uart mode to 16X

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);

	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, up->lcr);

	//spin_unlock_irqrestore(&up->port.lock, flags);
	dev_dbg(up->port.dev, "serial_omap_set_termios+%d\n", up->port.line);
}

static struct omap_uart_port_info *of_get_uart_port_info(struct device *dev)
{
	struct omap_uart_port_info *omap_up_info;

	omap_up_info = devm_kzalloc(dev, sizeof(*omap_up_info), GFP_KERNEL);
	if (!omap_up_info)
		return NULL; /* out of memory */

	of_property_read_u32(dev->of_node, "clock-frequency",
					 &omap_up_info->uartclk);
	return omap_up_info;
}


static int serial_omap_probe(struct platform_device *pdev)
{
	struct uart_omap_port *up = NULL;
	struct resource	*mem = NULL;
	//struct omap_uart_port_info *omap_up_info = dev_get_platdata(&pdev->dev);
	struct omap_uart_port_info *omap_up_info;
	int ret;

	if (pdev->dev.of_node) {
		omap_up_info = of_get_uart_port_info(&pdev->dev);
		pdev->dev.platform_data = omap_up_info;
	}
	//TODO 5.5: Get the base address for UART-1 and assign it to mem
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}


	if (!devm_request_mem_region(&pdev->dev, mem->start, resource_size(mem),
				pdev->dev.driver->name)) {
		dev_err(&pdev->dev, "memory region already claimed\n");
		return -EBUSY;
	}
	up = devm_kzalloc(&pdev->dev, sizeof(*up), GFP_KERNEL);
	if (!up)
		return -ENOMEM;

	up->dev = &pdev->dev;
	up->port.dev = &pdev->dev;
	up->port.type = PORT_16550A;
	up->port.iotype = UPIO_MEM;

	up->port.regshift = 2;

	sprintf(up->name, "MY UART%d", up->port.line);
	up->port.mapbase = mem->start;

	//TODO 5.6: Get the virtual address for the UART1 and assign it to up->port.membase

	if (!up->port.membase) {
		dev_err(&pdev->dev, "can't ioremap UART\n");
		ret = -ENOMEM;
		goto err_ioremap;
	}

	up->port.flags = omap_up_info->flags;
	up->port.uartclk = omap_up_info->uartclk;
	if (!up->port.uartclk) {
		up->port.uartclk = DEFAULT_CLK_SPEED;
		dev_warn(&pdev->dev, "No clock speed specified: using default:"
						"%d\n", DEFAULT_CLK_SPEED);
	}

	platform_set_drvdata(pdev, up);

	fcd_init(up);
	return 0;

err_ioremap:
	dev_err(&pdev->dev, "[UART%d]: failure [%s]: %d\n",
				pdev->id, __func__, ret);
	return ret;
}

static int serial_omap_remove(struct platform_device *dev)
{
	fcd_exit();

	return 0;
}

//TODO 5.4: Populate the compatible property. Check the dtb for compatible string
#if defined(CONFIG_OF)
static const struct of_device_id omap_serial_of_match[] = {
	{},
};
MODULE_DEVICE_TABLE(of, omap_serial_of_match);
#endif

//TODO 5.3 Populate the platform driver structure
//Add Probe and remove
//Initialize of_match_table
static struct platform_driver serial_omap_driver = {
	.driver		= {
		.name	= DRIVER_NAME1,
		.pm	= NULL,
	},
};

static int __init serial_omap_init(void)
{
	int ret = 0;
	//TODO 5.1 Register the platform driver
	return ret;
}
static void __exit serial_omap_exit(void)
{
	// TODO 5.2 Unregister the platform driver
}

module_init(serial_omap_init);
module_exit(serial_omap_exit);

MODULE_DESCRIPTION("OMAP High Speed UART driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Texas Instruments Inc");
