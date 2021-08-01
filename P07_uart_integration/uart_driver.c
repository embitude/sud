#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/module.h>
#include <linux/platform_data/serial-omap.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/serial_reg.h>
#include <uapi/linux/serial_reg.h>

#define SERIAL_MAJOR	240	/* experimental range */
#define SERIAL_MINORS	1	/* only have one minor */
#define UART_NR	1	/* only use one port */
#define SERIAL_NAME	"myuart"
#define MY_NAME	SERIAL_NAME

#define to_uart_omap_port(p)	((container_of((p), struct uart_omap_port, port)))
#define DEFAULT_CLK_SPEED 48000000 /* 48Mhz*/

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
#define DRIVER_NAME	"my_uart"
#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

#define to_uart_omap_port(p)	((container_of((p), struct uart_omap_port, port)))

static char start_flg = 0;

#define FUNC_ENTER() do { printk(KERN_INFO "Enter: %s\n", __func__); } while (0)

struct uart_omap_port {
	struct uart_port	port;
	struct device		*dev;

	unsigned char		ier;
	unsigned char		lcr;
	unsigned char		mcr;
	unsigned char		fcr;
	unsigned char		efr;
	unsigned char		dll;
	unsigned char		dlh;
	unsigned char		mdr1;
	unsigned char		scr;
	unsigned char		wer;

	/*
	 * Some bits in registers are cleared on a read, so they must
	 * be saved whenever the register is read but the bits will not
	 * be immediately processed.
	 */
	char		name[20];
	u8			wakeups_enabled;
	u32			features;

	u32			latency;
};


static const char *serial_type(struct uart_port *port)
{
	FUNC_ENTER();
	return "uart_tty";
}

static void serial_release_port(struct uart_port *port)
{
	FUNC_ENTER();
}

static int serial_request_port(struct uart_port *port)
{
	FUNC_ENTER();
	return 0;
}

static void serial_config_port(struct uart_port *port, int flags)
{
	FUNC_ENTER();
}

static int serial_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	FUNC_ENTER();
	return 0;
}

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

static inline void serial_omap_clear_fifos(struct uart_omap_port *up)
{
	FUNC_ENTER();
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO);
	serial_out(up, UART_FCR, UART_FCR_ENABLE_FIFO |
		       UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
	//TODO 7.30: Clear the FIFO buffers and disable them
}

static void serial_omap_stop_tx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();
	//TODO 7.16: Disable the UART THRI interrupt, if enabled
}
static inline int calculate_baud_abs_diff(struct uart_port *port,
				unsigned int baud, unsigned int mode)
{
	unsigned int n = port->uartclk / (mode * baud);
	int abs_diff;

	if (n == 0)
		n = 1;

	abs_diff = baud - (port->uartclk / (mode * n));
	if (abs_diff < 0)
		abs_diff = -abs_diff;

	return abs_diff;
}

static bool
serial_omap_baud_is_mode16(struct uart_port *port, unsigned int baud)
{
	int abs_diff_13 = calculate_baud_abs_diff(port, baud, 13);
	int abs_diff_16 = calculate_baud_abs_diff(port, baud, 16);

	return (abs_diff_13 >= abs_diff_16);
}

/*
 * serial_omap_get_divisor - calculate divisor value
 * @port: uart port info
 * @baud: baudrate for which divisor needs to be calculated.
 */
static unsigned int
serial_omap_get_divisor(struct uart_port *port, unsigned int baud)
{
	unsigned int mode;

	if (!serial_omap_baud_is_mode16(port, baud))
		mode = 13;
	else
		mode = 16;
	return port->uartclk/(mode * baud);
}

static void
serial_omap_set_termios(struct uart_port *port, struct ktermios *termios,
			struct ktermios *old)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned char cval = 0;
	unsigned int baud, quot;
	unsigned long flags = 0;

	FUNC_ENTER();

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

	/*
	 * Ask the core to calculate the divisor for us.
	 */

	baud = uart_get_baud_rate(port, termios, old, 0, port->uartclk/13);
	quot = serial_omap_get_divisor(port, baud);

	up->dll = quot & 0xff;
	up->dlh = quot >> 8;
	up->mdr1 = UART_OMAP_MDR1_DISABLE;

	/*
	 * Ok, we're now changing the port state. Do it with
	 * interrupts disabled.
	 */
	spin_lock_irqsave(&up->port.lock, flags);
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

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	up->efr = serial_in(up, UART_EFR) & ~UART_EFR_ECB;
	up->efr &= ~UART_EFR_SCD;
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);

	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	up->mcr = serial_in(up, UART_MCR) & ~UART_MCR_TCRTLR;
	serial_out(up, UART_MCR, up->mcr | UART_MCR_TCRTLR);
	/* FIFO ENABLE, DMA MODE */

	up->scr |= OMAP_UART_SCR_RX_TRIG_GRANU1_MASK;
	/*
	 * NOTE: Setting OMAP_UART_SCR_RX_TRIG_GRANU1_MASK
	 * sets Enables the granularity of 1 for TRIGGER RX
	 * level. Along with setting RX FIFO trigger level
	 * to 1 (as noted below, 16 characters) and TLR[3:0]
	 * to zero this will result RX FIFO threshold level
	 * to 1 character, instead of 16 as noted in comment
	 * below.
	 */

	/* Set receive FIFO threshold to 16 characters and
	 * transmit FIFO threshold to 16 spaces
	 */
	up->fcr &= ~OMAP_UART_FCR_RX_FIFO_TRIG_MASK;
	up->fcr &= ~OMAP_UART_FCR_TX_FIFO_TRIG_MASK;
	//TODO 7.23: Set the RX & TX FIFO threshold to 16 and enable the FIFO

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

	//TODO 7.24: Update the dll and dlh values in corresponding registers

	serial_out(up, UART_LCR, 0);
	serial_out(up, UART_IER, up->ier);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, cval);

	//TODO 7.25 Set the uart mode to 16X or 13X as per serial_omap_baud_is_mode16
	
	/* Configure flow control */
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);

	/* Enable access to TCR/TLR */
	serial_out(up, UART_EFR, up->efr | UART_EFR_ECB);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_A);
	serial_out(up, UART_MCR, up->mcr | UART_MCR_TCRTLR);

	serial_out(up, UART_TI752_TCR, OMAP_UART_TCR_TRIG);
	serial_out(up, UART_MCR, up->mcr);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, up->lcr);

	spin_unlock_irqrestore(&up->port.lock, flags);
	dev_dbg(up->port.dev, "serial_omap_set_termios+%d\n", up->port.line);
}

static void serial_omap_break_ctl(struct uart_port *port, int break_state)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned long flags = 0;

	dev_dbg(up->port.dev, "serial_omap_break_ctl+%d\n", up->port.line);
	spin_lock_irqsave(&up->port.lock, flags);
	if (break_state == -1)
		up->lcr |= UART_LCR_SBC;
	else
		up->lcr &= ~UART_LCR_SBC;
	serial_out(up, UART_LCR, up->lcr);
	spin_unlock_irqrestore(&up->port.lock, flags);
}

static void transmit_chars(struct uart_omap_port *up, unsigned int lsr)
{
	struct circ_buf *xmit = &up->port.state->xmit;
	int count;

	FUNC_ENTER();

	if (uart_circ_empty(xmit) || uart_tx_stopped(&up->port)) {
		serial_omap_stop_tx(&up->port);
		return;
	}
	count = up->port.fifosize / 4;
	do {
		serial_out(up, UART_TX, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		up->port.icount.tx++;
		if (uart_circ_empty(xmit))
			break;
	} while (--count > 0);

	if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
		uart_write_wakeup(&up->port);

	if (uart_circ_empty(xmit))
		serial_omap_stop_tx(&up->port);
}

static inline void serial_omap_enable_ier_thri(struct uart_omap_port *up)
{
	FUNC_ENTER();
	//TODO 7.15: Enable the THRI interrupt, if not already enabled
}

void serial_omap_start_tx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	FUNC_ENTER();
	serial_omap_enable_ier_thri(up);
}

static void serial_omap_rlsi(struct uart_omap_port *up, unsigned int lsr)
{
	unsigned int flag;
	unsigned char ch = 0;
	FUNC_ENTER();

	if (likely(lsr & UART_LSR_DR))
		ch = serial_in(up, UART_RX);

	up->port.icount.rx++;
	flag = TTY_NORMAL;

	if (lsr & UART_LSR_BI) {
		flag = TTY_BREAK;
		lsr &= ~(UART_LSR_FE | UART_LSR_PE);
		up->port.icount.brk++;
		/*
		 * We do the SysRQ and SAK checking
		 * here because otherwise the break
		 * may get masked by ignore_status_mask
		 * or read_status_mask.
		 */
		if (uart_handle_break(&up->port))
			return;
	}

	if (lsr & UART_LSR_PE) {
		flag = TTY_PARITY;
		up->port.icount.parity++;
	}

	if (lsr & UART_LSR_FE) {
		flag = TTY_FRAME;
		up->port.icount.frame++;
	}

	if (lsr & UART_LSR_OE)
		up->port.icount.overrun++;

	uart_insert_char(&up->port, lsr, UART_LSR_OE, 0, flag);
}
	
static void serial_omap_rdi(struct uart_omap_port *up, unsigned int lsr)
{
	unsigned char ch = 0;
	unsigned int flag;

	FUNC_ENTER();

	//TODO 7.31: Check if (UART_LSR_DR) is set, otherwise return. Else read the char into ch

	flag = TTY_NORMAL;
	up->port.icount.rx++;
	
	//TODO 7.32: Pass on the bytes to TTY Layer using uart_insert_char 
	//TODO 7.33: Push the buffer to user space 
}

/**
 * serial_omap_irq() - This handles the interrupt from one port
 * @irq: uart port irq number
 * @dev_id: uart port info
 */
static irqreturn_t serial_omap_irq(int irq, void *dev_id)
{
	struct uart_omap_port *up = dev_id;
	unsigned int iir, lsr;
	unsigned int type;
	irqreturn_t ret = IRQ_NONE;
	int max_count = 256;
	FUNC_ENTER();

	spin_lock(&up->port.lock);

	do {
		//TODO 7.26: Read IIR register and break if not interrupt

		ret = IRQ_HANDLED;
		lsr = serial_in(up, UART_LSR);

		/* extract IRQ type from IIR register */
		type = iir & 0x3e;
		printk("type = %x\n", type);

		switch (type) {
		//TODO 7.27: Add the switch case for IIR_THRI and transmit the chars
		case UART_IIR_RX_TIMEOUT:
			/* FALLTHROUGH */
		// TODO 7.28: Invoke serial_omap_rdi, if UART_IIR_RDI is set and then return
		// TODO 7.29: Invoke serial_omap_rlsi, if UART_IIR_RLSI is set and then return
		case UART_IIR_CTS_RTS_DSR:
			/* simply try again */
			break;
		case UART_IIR_XOFF:
			/* FALLTHROUGH */
		default:
			break;
		}
	} while (!(iir & UART_IIR_NO_INT) && max_count--);

	spin_unlock(&up->port.lock);

	return ret;
}

int serial_omap_startup(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	int retval = 0;
	struct ktermios termios;

	if (start_flg)
		return 0;
	start_flg = 1;

	FUNC_ENTER();

	spin_lock_init(&up->port.lock);
	/*
	 * Allocate the IRQ
	 */
	//TODO 7.18: Register the ISR (serial_omap_irq)
	if (retval)
		return retval;

	dev_dbg(up->port.dev, "serial_omap_startup+%d\n", up->port.line);

	/*
	 * Clear the FIFO buffers and disable them.
	 * (they will be reenabled in set_termios())
	 */
	serial_omap_clear_fifos(up);

	/*
	 * Clear the interrupt registers.
	 */
	(void) serial_in(up, UART_LSR);
	if (serial_in(up, UART_LSR) & UART_LSR_DR)
		(void) serial_in(up, UART_RX);
	(void) serial_in(up, UART_IIR);
	(void) serial_in(up, UART_MSR);

	/*
	 * Now, initialize the UART
	 */
	//TODO 7.19: Update the LCR register with 8 bit word length

	/*
	 * Finally, enable interrupts. Note: Modem status interrupts
	 * are set via set_termios(), which will be occurring imminently
	 * anyway, so we don't enable them here.
	 */
	//TODO 7.20: Enable RLSI and RDI interrupts

	/* Enable module level wake up */
	up->wer = OMAP_UART_WER_MOD_WKUP;
	if (up->features & OMAP_UART_WER_HAS_TX_WAKEUP)
		up->wer |= OMAP_UART_TX_WAKEUP_EN;

	serial_out(up, UART_OMAP_WER, up->wer);

	memset(&termios, 0, sizeof(struct ktermios));
	termios.c_cflag = CS8 | B115200 | CREAD;
	termios.c_iflag = IGNBRK | IGNPAR;
	serial_omap_set_termios(&up->port, &termios, NULL);

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
	//TODO 7.21: Disable all the interrupts

	/*
	 * Disable break condition and FIFOs
	 */
	serial_out(up, UART_LCR, serial_in(up, UART_LCR) & ~UART_LCR_SBC);
	serial_omap_clear_fifos(up);

	/*
	 * Read data port to reset things, and then free the irq
	 */
	if (serial_in(up, UART_LSR) & UART_LSR_DR)
		(void) serial_in(up, UART_RX);

	//TODO 7.22: Free up the IRQ
}

static unsigned int serial_omap_tx_empty(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);

	return serial_in(up, UART_LSR) & UART_LSR_TEMT ? TIOCSER_TEMT : 0;
}

static void serial_omap_stop_rx(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	//TODO 7.17: Disable the RLSI and RDI interrupt
}
static void serial_omap_enable_ms(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);

	dev_dbg(up->port.dev, "serial_omap_enable_ms+%d\n", up->port.line);
	up->ier |= UART_IER_MSI;
	serial_out(up, UART_IER, up->ier);
}

static unsigned int serial_omap_get_mctrl(struct uart_port *port)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned int ret = 0;

	dev_dbg(up->port.dev, "serial_omap_get_mctrl+%d\n", up->port.line);
	return ret;
}

static void serial_omap_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct uart_omap_port *up = to_uart_omap_port(port);
	unsigned char mcr = 0, old_mcr, lcr;

	dev_dbg(up->port.dev, "serial_omap_set_mctrl+%d\n", up->port.line);
	if (mctrl & TIOCM_RTS)
		mcr |= UART_MCR_RTS;
	if (mctrl & TIOCM_DTR)
		mcr |= UART_MCR_DTR;
	if (mctrl & TIOCM_OUT1)
		mcr |= UART_MCR_OUT1;
	if (mctrl & TIOCM_OUT2)
		mcr |= UART_MCR_OUT2;
	if (mctrl & TIOCM_LOOP)
		mcr |= UART_MCR_LOOP;

	old_mcr = serial_in(up, UART_MCR);
	old_mcr &= ~(UART_MCR_LOOP | UART_MCR_OUT2 | UART_MCR_OUT1 |
		     UART_MCR_DTR | UART_MCR_RTS);
	up->mcr = old_mcr | mcr;
	serial_out(up, UART_MCR, up->mcr);

	/* Turn off autoRTS if RTS is lowered; restore autoRTS if RTS raised */
	lcr = serial_in(up, UART_LCR);
	serial_out(up, UART_LCR, UART_LCR_CONF_MODE_B);
	if ((mctrl & TIOCM_RTS) && (port->status & UPSTAT_AUTORTS))
		up->efr |= UART_EFR_RTS;
	else
		up->efr &= ~UART_EFR_RTS;
	serial_out(up, UART_EFR, up->efr);
	serial_out(up, UART_LCR, lcr);
}

//TODO 7.14: Initialize the callback handers:
//tx_emtpty, set_mctrl, get_mctrl, stop_tx, start_tx and so on
static struct uart_ops serial_ops = {
	.set_mctrl	= serial_omap_set_mctrl,
	.get_mctrl	= serial_omap_get_mctrl,
	.enable_ms	= serial_omap_enable_ms,
	.break_ctl	= serial_omap_break_ctl,
	.set_termios	= serial_omap_set_termios,
	.type	= serial_type,
	.config_port	= serial_config_port,
	.verify_port	= serial_verify_port,
};

//TODO 7.13: Populate the uart_driver structure
// Populate the driver_name, dev_name and number of uarts
static struct uart_driver serial_reg = {
	.owner	= THIS_MODULE,
	.major	= SERIAL_MAJOR,
	.minor	= SERIAL_MINORS,
};

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

static int serial_omap_remove(struct platform_device *dev)
{
	struct uart_omap_port *up = platform_get_drvdata(dev);

	//TODO 7.12: De-register the uart port

	return 0;
}

static int serial_omap_probe(struct platform_device *pdev)
{
	struct omap_uart_port_info *omap_up_info = dev_get_platdata(&pdev->dev);
	struct uart_omap_port *up;
	struct resource *mem = NULL, *irq = NULL;
	int ret = 0;

	if (pdev->dev.of_node) {
		omap_up_info = of_get_uart_port_info(&pdev->dev);
		pdev->dev.platform_data = omap_up_info;
	} else {
		return -ENODEV;
	}

	//TODO 7.7: Get the base address for UART-1 and assign it to mem
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	//TODO 7.8: Get the irq number using IORESOURCE_IRQ
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
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
	up->port.type = PORT_OMAP;
	up->port.iotype = UPIO_MEM;
	up->port.irq = irq->start;
	up->port.regshift = 2;
	up->port.fifosize = 64;
	//TODO 7.9: Assign the uart port ops to the appropriate field in up->port

	up->port.line = 0;

	sprintf(up->name, "OMAP UART%d", up->port.line);
	up->port.mapbase = mem->start;
	//TODO 7.10: Get the virtual address for the UART1 and assign it to up->port.membase

	if (!up->port.membase) {
		dev_err(&pdev->dev, "can't ioremap UART\n");
		return -ENOMEM;
	}

	up->port.flags = omap_up_info->flags;
	up->port.uartclk = omap_up_info->uartclk;
	if (!up->port.uartclk) {
		up->port.uartclk = DEFAULT_CLK_SPEED;
		dev_warn(&pdev->dev,
			 "No clock speed specified: using default: %d\n",
			 DEFAULT_CLK_SPEED);
	}

	platform_set_drvdata(pdev, up);

	//TODO 7.11: Register the port with TTY framework by using uart_add_one_port

	return ret;
}

//TODO 7.6: Populate the of-match table
static const struct of_device_id omap_serial_of_match[] = {
	{},
	{},
};
MODULE_DEVICE_TABLE(of, omap_serial_of_match);

//TODO 7.5: Populate the platform driver structure
static struct platform_driver serial_omap_driver = {
	.driver		= {
		.name	= "my_uart",
		.pm	= NULL,
	},
};

static int __init uart_init(void)
{
	int ret = 0;

	//TODO 7.2 Register the platform driver
	if (ret != 0)
		return ret;
	//TODO 7.1 Register the platform driver
	if (ret != 0)
		uart_unregister_driver(&serial_reg);
	return ret;
}

static void __exit uart_exit(void)
{
	//TODO 7.3 De-register the platform driver
	//TODO 7.4 De-register the uart driver
}

module_init(uart_init);
module_exit(uart_exit);

MODULE_AUTHOR("Embitude Trainings <info@embitude.in>");
MODULE_DESCRIPTION("Uart Driver");
MODULE_LICENSE("GPL");
