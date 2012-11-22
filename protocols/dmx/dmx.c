/*
 * Copyright (c) 2009 by Dirk Pannenbecker <dp@sd-gp.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * for DMX timing specifications see
 * http://www.erwinrol.com/dmx512/ or
 * http://opendmx.net/index.php/DMX512-A
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "config.h"
#include "services/dmx-storage/dmx_storage.h"
#ifndef DMX_USE_USART
#define DMX_USE_USART 0
#endif
#define USE_USART DMX_USE_USART
#define BAUD 250000
#include "core/usart.h"

#if 0

/* generating private usart init */
generate_usart_init_8N2()

volatile uint8_t dmx_index = 0;
volatile uint8_t dmx_txlen = DMX_STORAGE_CHANNELS;

/**
 * Init DMX
 */
void
dmx_init(void)
{
  /* initialize the usart module */
#if (USE_USART == 0 && defined(HAVE_RS485TE_USART0))
  PIN_CLEAR(RS485TE_USART0);            // disable RS485 driver for usart 0
  DDR_CONFIG_OUT(RS485TE_USART0);
  PIN_SET(TXD0);                        // mark
  DDR_CONFIG_OUT(TXD0);
#elif (USE_USART == 1  && defined(HAVE_RS485TE_USART1))
  PIN_CLEAR(RS485TE_USART1);            // disable RS485 driver for usart 1
  DDR_CONFIG_OUT(RS485TE_USART1);
  PIN_SET(TXD1);                        // mark
  DDR_CONFIG_OUT(TXD1);
#else
#warning no RS485 transmit enable pin for DMX defined
#endif

  usart_init();
}

/**
 * Send DMX-channels via USART
 */
void
dmx_tx_start(void)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
#if (USE_USART == 0)
#ifdef HAVE_RS485TE_USART0
    PIN_SET(RS485TE_USART0);            // enable RS485 driver for usart 0
#endif
    PIN_CLEAR(TXD0);                    // generate a break signal on usart 0
    _delay_us(176);                     // according to DMX512-A standard
    PIN_SET(TXD0);                      // mark after break
    _delay_us(12);                      // according to DMX512-A standard
#elif (USE_USART == 1)
#ifdef HAVE_RS485TE_USART1
    PIN_SET(RS485TE_USART1);            // enable RS485 driver for usart 1
#endif
    PIN_CLEAR(TXD1);                    // generate a break signal on usart 1
    _delay_us(176);                     // according to DMX512-A standard
    PIN_SET(TXD1);                      // mark after break
    _delay_us(12);                      // according to DMX512-A standard
#endif

    /* start a new dmx packet */
    usart(UCSR,B) = _BV(usart(TXEN));   // enable usart
    usart(UCSR,A) |= _BV(usart(TXC));   // reset transmit complete flag
    usart(UDR) = 0;                     // send startbyte (not always 0!)
    usart(UCSR,B) |= _BV(usart(TXCIE)); // enable usart interrupt
  }
}

void
dmx_tx_stop(void)
{
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    usart(UCSR, B) = 0;                 // disable usart

#if (USE_USART == 0 && defined(HAVE_RS485TE_USART0))
    PIN_CLEAR(RS485TE_USART0);          // disable RS485 driver for usart 0
    PIN_SET(TXD0);// mark
#elif (USE_USART == 1  && defined(HAVE_RS485TE_USART1))
    PIN_CLEAR(RS485TE_USART1);          // disable RS485 driver for usart 1
    PIN_SET(TXD1);// mark
#endif
    dmx_index = 0;                      // reset output channel index
  }
}

/**
 * Send DMX-packet
 */
void
dmx_periodic(void)
{
  wdt_kick();
  if(dmx_index == 0) {
    dmx_tx_start();
  }
}

ISR(usart(USART,_TX_vect))
{
  /* Send the rest */
  if(dmx_index < dmx_txlen) {
    if(usart(UCSR, A) & _BV(usart(UDRE))) {
      usart(UDR) = get_dmx_channel(DMX_OUTPUT_UNIVERSE, dmx_index++);
    }
  } else
  dmx_tx_stop();
}

#else

/* baudrate used for DMX data bytes */
#define DMX_BAUD 250000
/* baudrate to generate 176µs break signal at start of packet */
#define DMX_BAUD_BREAK 51000

#define DMX_VALUE (((F_CPU) + 8UL * (DMX_BAUD)) / (16UL * (DMX_BAUD)) -1UL)
#define DMX_BREAK_VALUE (((F_CPU) + 8UL * (DMX_BAUD_BREAK)) / (16UL * (DMX_BAUD_BREAK)) -1UL)

typedef enum {
  DMX_BREAK,
  DMX_START,
  DMX_DATA,
} dmx_tx_state_t;

static volatile uint16_t dmx_index = 0;
static volatile uint16_t dmx_txlen = DMX_STORAGE_CHANNELS;
static volatile dmx_tx_state_t dmx_tx_state = DMX_START;


/**
 * init DMX
 * after initialisation of IOs, all data transfer is done only within ISR
 */
void dmx_init(void) {
  /* initialize the usart module */
#if (USE_USART == 0 && defined(HAVE_RS485TE_USART0))
  PIN_SET(RS485TE_USART0);              // disable RS485 driver for usart 0
  DDR_CONFIG_OUT(RS485TE_USART0);
  PIN_SET(TXD0);                        // mark
  DDR_CONFIG_OUT(TXD0);
#elif (USE_USART == 1  && defined(HAVE_RS485TE_USART1))
  PIN_SET(RS485TE_USART1);              // disable RS485 driver for usart 1
  DDR_CONFIG_OUT(RS485TE_USART1);
  PIN_SET(TXD1);                        // mark
  DDR_CONFIG_OUT(TXD1);
#else
#warning no RS485 transmit enable pin for DMX defined
#endif

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    usart(UBRR,H) = (DMX_BREAK_VALUE >> 8);
    usart(UBRR,L) = (DMX_BREAK_VALUE & 0xff);
    /* set mode 8N2: 8 bits, 2 stop, no parity, asynchronous usart
     * and set URSEL, if present, */
    usart(UCSR,C) =  _BV(usart(USBS)) | (3 << (usart(UCSZ,0))) | _BV_URSEL;
    USART_2X();
//    /* transmitter enable, TX complete interrupt enable */
//    usart(UCSR, B) |= _BV(usart(TXEN)) | _BV(usart(TXCIE));
    /* transmitter enable, tx data empty interrupt enable */
    usart(UCSR, B) |= _BV(usart(TXEN)) | _BV(usart(UDRIE));

    /* start by sending a break signal */
    usart(UDR) = 0;
  }

}

/**
 * DMX interrupt service routine
 * DMX_BREAK: send a 176µs break signal
 * DMX_START: send a start byte with 250kbps
 * DMX_DATA:  send up to 511 bytes of DMX data with 250kbps
 */
//ISR(usart(USART, _TX_vect))
ISR(usart(USART,_UDRE_vect))
{
  switch (dmx_tx_state) {
  case DMX_BREAK:
    /* set break baudrate */
    usart(UBRR,H) = (DMX_BREAK_VALUE >> 8);
    usart(UBRR,L) = (DMX_BREAK_VALUE & 0xff);
    /* send break signal */
    usart(UDR) = 0;
    /* reset data pointer */
    dmx_index = 0;
    dmx_tx_state = DMX_START;
    break;

  case DMX_START:
    /* set normal DMX baudrate */
    usart(UBRR,H) = (DMX_VALUE >> 8);
    usart(UBRR,L) = (DMX_VALUE & 0xff);
    /* send start byte */
    usart(UDR) = 0;
    dmx_tx_state = DMX_DATA;
    break;

  case DMX_DATA:
    /* send DMX data bytes */
    usart(UDR) = get_dmx_channel(DMX_OUTPUT_UNIVERSE, dmx_index++);

    /* start packet if end of universe is reached */
    if (dmx_index >= dmx_txlen)
      dmx_tx_state = DMX_BREAK;
    break;
  }
}

#endif

/*
 -- Ethersex META --
 header(protocols/dmx/dmx.h)
# timer(1, dmx_periodic())
 init(dmx_init)
 */
