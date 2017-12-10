/*
 * Purpose: MPU-401 (UART mode) driver
 *
 * This driver/library can be used by the drivers for devices that provide
 * MPU-401 (UART) compatible interface.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include "oss_config.h"
#include "midi_core.h"

#include "uart401.h"

#define	DATAPORT   (devc->base)
#define	COMDPORT   (devc->base+1)
#define	STATPORT   (devc->base+1)

static int
uart401_status (uart401_devc * devc)
{
  return INB (devc->osdev, STATPORT);
}

#define input_avail(devc) (!(uart401_status(devc)&INPUT_AVAIL))
#define output_ready(devc)	(!(uart401_status(devc)&OUTPUT_READY))
static void
uart401_cmd (uart401_devc * devc, unsigned char cmd)
{
  OUTB (devc->osdev, cmd, COMDPORT);
}
static int
uart401_read (uart401_devc * devc)
{
  return INB (devc->osdev, DATAPORT);
}
static void
uart401_write (uart401_devc * devc, unsigned char byte)
{
  OUTB (devc->osdev, byte, DATAPORT);
}

#define	OUTPUT_READY	0x40
#define	INPUT_AVAIL	0x80
#define	MPU_ACK		0xFE
#define	MPU_RESET	0xFF
#define	UART_MODE_ON	0x3F

static int reset_uart401 (uart401_devc * devc);
static void enter_uart_mode (uart401_devc * devc);

static void
uart401_input_loop (uart401_devc * devc)
{
  unsigned char buf[128];
  int l = 0;

  while (input_avail (devc))
    {
      int dev;
      unsigned char c;

      c = uart401_read (devc);

      dev = devc->my_dev;
      if (midi_devs[dev]->input_callback != NULL)
	midi_devs[dev]->input_callback (dev, c);

      if (c == MPU_ACK)
	devc->input_byte = c;
      else if ((devc->opened & OPEN_READ))
	{
	  buf[l++] = c;

	  if (l >= sizeof (buf))	/* Buffer full */
	    {
	      if (devc->save_input_buffer != NULL)
		devc->save_input_buffer (devc->my_dev, buf, l);
	      l = 0;
	    }
	}

      if (l > 0)
	{
	  if (devc->save_input_buffer != NULL)
	    devc->save_input_buffer (devc->my_dev, buf, l);
	}
    }
}

void
uart401_irq (uart401_devc * devc)
{
  if (devc->base == 0)
    {
      /* cmn_err(CE_CONT, "uart401_irq: Bad base address\n"); */
      return;
    }

  if (input_avail (devc))
    {
      uart401_input_loop (devc);
    }
}

/*ARGSUSED*/
static int
uart401_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
	      oss_midi_inputbuf_t inputbuf, oss_midi_outputintr_t outputintr)
{
  uart401_devc *devc = (uart401_devc *) midi_devs[dev]->devc;

  if (devc->opened)
    {
      return OSS_EBUSY;
    }

  while (input_avail (devc))
    uart401_read (devc);

  devc->save_input_buffer = inputbuf;
  devc->opened = mode;
  enter_uart_mode (devc);
  devc->disabled = 0;

  return 0;
}

/*ARGSUSED*/
static void
uart401_close (int dev, int mode)
{
  uart401_devc *devc = (uart401_devc *) midi_devs[dev]->devc;

  reset_uart401 (devc);
  oss_udelay (10);
  enter_uart_mode (devc);
  reset_uart401 (devc);
  devc->save_input_buffer = NULL;
  devc->opened = 0;
}

static int
uart401_out (int dev, unsigned char midi_byte)
{
  int timeout;
  uart401_devc *devc = (uart401_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  if (devc->disabled)
    return 1;
  /*
   * Test for input since pending input seems to block the output.
   */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (input_avail (devc))
    uart401_input_loop (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  /*
   * Sometimes it takes about 130000 loops before the output becomes ready
   * (After reset). Normally it takes just about 10 loops.
   */

  for (timeout = 13000; timeout > 0 && !output_ready (devc); timeout--);

  if (!output_ready (devc))
    {
      return 0;
    }

  uart401_write (devc, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
uart401_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t uart401_driver = {
  uart401_open,
  uart401_close,
  uart401_ioctl,
  uart401_out
};

static void
enter_uart_mode (uart401_devc * devc)
{
  int ok, timeout;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

  devc->input_byte = 0;
  uart401_cmd (devc, UART_MODE_ON);

  ok = 0;
  for (timeout = 50000; timeout > 0 && !ok; timeout--)
    if (devc->input_byte == MPU_ACK)
      ok = 1;
    else if (input_avail (devc))
      if (uart401_read (devc) == MPU_ACK)
	ok = 1;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
reset_uart401 (uart401_devc * devc)
{
  int ok, timeout, n;
  oss_native_word flags;

  /*
   * Send the RESET command. Try again if no success at the first time.
   */
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  ok = 0;

  for (n = 0; n < 2 && !ok; n++)
    {
      for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

      devc->input_byte = 0;
      uart401_cmd (devc, MPU_RESET);

      /*
       * Wait at least 25 msec. This method is not accurate so let's make the
       * loop bit longer. Cannot sleep since this is called during boot.
       */

      for (timeout = 50000; timeout > 0 && !ok; timeout--)
	if (devc->input_byte == MPU_ACK)	/* Interrupt */
	  ok = 1;
	else if (input_avail (devc))
	  if (uart401_read (devc) == MPU_ACK)
	    ok = 1;

    }



  if (ok)
    uart401_input_loop (devc);	/*
				 * Flush input before enabling interrupts
				 */
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return ok;
}

int
uart401_init (uart401_devc * devc, oss_device_t * osdev, int base, char *name)
{
  int ok = 0;

  DDB (cmn_err (CE_CONT, "Entered uart401_init(%x)\n", base));

  devc->base = base;
  devc->irq = 0;
  devc->osdev = osdev;
  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV + 3);
  devc->running = 1;

  devc->save_input_buffer = NULL;
  devc->opened = 0;
  devc->input_byte = 0;
  devc->my_dev = 0;
  devc->share_irq = 0;

  ok = reset_uart401 (devc);

  if (ok)
    {
      DDB (cmn_err (CE_CONT, "Reset UART401 OK\n"));
    }
  else
    {
      DDB (cmn_err
	   (CE_CONT, "Reset UART401 failed (no hardware present?).\n"));
      DDB (cmn_err (CE_CONT, "mpu401 status %02x\n", uart401_status (devc)));
    }

  if (!ok)
    {
      return 0;
    }

  DDB (cmn_err (CE_CONT, "uart401 detected OK\n"));

  enter_uart_mode (devc);

  devc->my_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "UART401", name, &uart401_driver, sizeof (midi_driver_t),
				      0, devc, devc->osdev);
  devc->opened = 0;
#ifdef USE_POLLING
  if (!timer_armed)
    {
      timer_armed = 1;
      poll_interval = OSS_HZ / 10;
      if (poll_interval < 1)
	poll_interval = 1;
      INIT_TIMER (uart401_timer, uart401_poll);
      ACTIVATE_TIMER (uart401_timer, uart401_poll, poll_interval);
    }
#endif
  return 1;
}

void
uart401_disable (uart401_devc * devc)
{
#ifdef USE_POLLING
  if (timer_armed)
    {
      timer_armed = 0;
      REMOVE_TIMER (uart401_timer, uart401_poll);
    }
  else;
#endif
  if (!devc->running)
    return;
  reset_uart401 (devc);
  devc->running = 0;
}
