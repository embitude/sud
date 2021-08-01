#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/module.h>

#define DELAY_TIME	HZ * 2	/* 2 seconds per character */
#define TINY_DATA_CHARACTER	't'
#define TINY_SERIAL_MAJOR	240	/* experimental range */
#define TINY_SERIAL_MINORS	1	/* only have one minor */
#define UART_NR	1	/* only use one port */
#define TINY_SERIAL_NAME	"ttytiny"
#define MY_NAME	TINY_SERIAL_NAME

static struct timer_list my_timer;
struct uart_port *dummy_upt;

#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

static void tiny_stop_tx(struct uart_port *port)
{
	FUNC_ENTER();
}

static void tiny_stop_rx(struct uart_port *port)
{
	FUNC_ENTER();
}

static void tiny_enable_ms(struct uart_port *port)
{
	FUNC_ENTER();
}

static void tiny_tx_chars(struct uart_port *port)
{
	struct circ_buf *xmit = &port->state->xmit;
	int count;
	FUNC_ENTER();
	if (port->x_char) {
		printk("wrote %2x", port->x_char);
		port->icount.tx++;
		port->x_char = 0;
		return;
	}
	if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
		tiny_stop_tx(port);
		return;
	}
	count = port->fifosize >> 1;
	do {
		//TODO 6.10: Print the byte at xmit->tail and increment the tail
		port->icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);
	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(port);
	if (uart_circ_empty(xmit))
		tiny_stop_tx(port);
}

static void tiny_start_tx(struct uart_port *port)
{
	FUNC_ENTER();
}

static void tiny_timer(struct timer_list *t)
{
	struct uart_port *port;
	struct tty_struct *tty;
	struct tty_port *tty_port;

	FUNC_ENTER();
	port = dummy_upt;
	if (!port)
		return;
	if (!port->state)
		return;
	tty = port->state->port.tty;
	if (!tty)
		return;
	tty_port = tty->port;
	//TODO 6.8: Send dummy characters to the TTY Layer with uart_insert_char

	tty_flip_buffer_push(&port->state->port);

	/* resubmit the timer again */
	printk("Firing\n");
	//TODO 6.9: Restart the timer for 2 secs 
	/* see if we have any data to transmit */
	tiny_tx_chars(port);
}

static unsigned int tiny_tx_empty(struct uart_port *port)
{
	FUNC_ENTER();
	return 0;
}

static unsigned int tiny_get_mctrl(struct uart_port *port)
{
	FUNC_ENTER();
	return 0;
}

static void tiny_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	FUNC_ENTER();
}

static void tiny_break_ctl(struct uart_port *port, int break_state)
{
	FUNC_ENTER();
}

static void tiny_set_termios(struct uart_port *port,
		struct ktermios *new, struct ktermios *old)
{
	int baud, quot, cflag = new->c_cflag;
	FUNC_ENTER();
	/* get the byte size */
	switch (cflag & CSIZE) {
		case CS5:
			printk(KERN_DEBUG " - data bits = 5\n");
			break;
		case CS6:
			printk(KERN_DEBUG " - data bits = 6\n");
			break;
		case CS7:
			printk(KERN_DEBUG " - data bits = 7\n");
			break;
		default: // CS8
			printk(KERN_DEBUG " - data bits = 8\n");
			break;
	}
	/* determine the parity */
	if (cflag & PARENB)
		if (cflag & PARODD)
			printk(" - parity = odd\n");
		else
			printk(" - parity = even\n");
	else
		printk(" - parity = none\n");
	/* figure out the stop bits requested */
	if (cflag & CSTOPB)
		printk(" - stop bits = 2\n");
	else
		printk(" - stop bits = 1\n");
	/* figure out the flow control settings */
	if (cflag & CRTSCTS)
		printk(" - RTS/CTS is enabled\n");
	else
		printk(" - RTS/CTS is disabled\n");
	/* Set baud rate */
	baud = uart_get_baud_rate(port, new, old, 0, 19200/16);
	quot = uart_get_divisor(port, baud);
}

static int tiny_startup(struct uart_port *port)
{
	int ret = 0;

	FUNC_ENTER();

	//TODO 6.6: Initialize the timer & add to the kernel's timer list
	dummy_upt = port;
	printk("Starting timer to fire in 2000ms (%ld)\n", jiffies);
	//TODO 6.7: Start the timer
	if (ret)
		printk("Error in mod_timer\n");
	return 0;
}

static void tiny_shutdown(struct uart_port *port)
{
	FUNC_ENTER();
	/* The port is being closed by the last user. */
	/* Do any hardware specific stuff here */
	/* shut down our timer */
	//TODO 6.11: Delete the timer
}

static const char *tiny_type(struct uart_port *port)
{
	FUNC_ENTER();
	return "tinytty";
}

static void tiny_release_port(struct uart_port *port)
{
	FUNC_ENTER();
}

static int tiny_request_port(struct uart_port *port)
{
	FUNC_ENTER();
	return 0;
}

static void tiny_config_port(struct uart_port *port, int flags)
{
	FUNC_ENTER();
}

static int tiny_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	FUNC_ENTER();
	return 0;
}

//TODO 6.5: Initialize the callback handers:
//tx_emtpty, set_mctrl, get_mctrl, stop_tx, start_tx and so on
static struct uart_ops tiny_ops = {
	.break_ctl	= tiny_break_ctl,
	.type	= tiny_type,
	.release_port	= tiny_release_port,
	.request_port	= tiny_request_port,
	.config_port	= tiny_config_port,
	.verify_port	= tiny_verify_port,
};

//TODO 6.4: Register the uart ops
static struct uart_port tiny_port = {
	.type = PORT_16550A,
};

//TODO 6.3: Populate the uart_driver structure
// Populate the driver_name, dev_name and number of uarts
static struct uart_driver tiny_reg = {
	.owner	= THIS_MODULE,
	.major	= TINY_SERIAL_MAJOR,
	.minor	= TINY_SERIAL_MINORS,
};

static int __init tiny_init(void)
{
	int result = 0;
	printk(KERN_INFO "Tiny serial driver loaded\n");
	//TODO 6.1: Register the uart driver
	if (result)
		return result;
	//TODO 6.2: Add the uart port with uart_add_one_port
	if (result)
		uart_unregister_driver(&tiny_reg);
	return result;
}

module_init(tiny_init);

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Dummy Uart Driver");
MODULE_LICENSE("GPL");
