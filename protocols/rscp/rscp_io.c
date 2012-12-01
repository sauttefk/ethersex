/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
 * (c) 2012 by Jörg Henne <hennejg@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <stdlib.h>
#include <stdio.h>

#include "rscp.h"

#ifdef RSCP_SUPPORT

const ioConfig_t rscp_portConfig[RSCP_IOS] PROGMEM = { RSCP_IO_CONFIG(C) };


static uint8_t rscp_setPortDDR(uint16_t portID, uint8_t value) {
  portPtrType portDDR =
      (portPtrType) pgm_read_word(&rscp_portConfig[portID].ddr);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG_IO("set DDR port %d (%x, bit %d): %d\n", portID,
      pgm_read_word(&rscp_portConfig[portID].ddr), bit, value);

  // set direction
  if(value)
    return *portDDR |= bit;
  else
    return *portDDR &= ~bit;
}


static uint8_t rscp_setPortPORT(uint16_t portID, uint8_t value) {
  portPtrType portOut =
      (portPtrType) pgm_read_word(&rscp_portConfig[portID].portOut);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG_IO("set PORT port %d (%x, bit %d): %d\n", portID,
      pgm_read_word(&rscp_portConfig[portID].portOut), bit, value);

  // set pullup
  if(value)
    return *portOut |= bit;
  else
    return *portOut &= ~bit;
}


//static uint8_t rscp_togglePortPORT(uint16_t portID) {
//  portPtrType portOut =
//      (portPtrType) pgm_read_word(&rscp_portConfig[portID].portOut);
//  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);
//
//  RSCP_DEBUG_IO("toggle PORT port %d (%x, bit %d)\n", portID, portOut, bit);
//
//  return *portOut ^= bit;
//}
//
//
//static uint8_t rscp_getPortPIN(uint16_t portID) {
//  portPtrType portIn =
//      (portPtrType) pgm_read_word(&rscp_portConfig[portID].portIn);
//  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);
//  return *portIn & bit ? 1 : 0;
//}

void rscp_parseBIC(void *ptr, uint16_t items) {
  rscp_binaryInputChannels = malloc(items * sizeof(rscp_binaryInputChannel));
  if (rscp_binaryInputChannels == NULL ) {
    rscp_numBinaryInputChannels = 0;
    RSCP_DEBUG_CONF("Out of memory\n");
    return;
  }

  rscp_numBinaryInputChannels = items;
  memset(rscp_binaryInputChannels, 0, items * sizeof(rscp_binaryInputChannel));
  RSCP_DEBUG_CONF(
      "Allocated %d bytes for %d binary input channels\n", items * sizeof(rscp_binaryInputChannel), items);

  for (uint16_t i = 0; i < items; ++i) {
    rscp_binaryInputChannels[i].channel =
        rscpEE_word(rscp_binaryInputChannel, channel, ptr);
    rscp_binaryInputChannels[i].port =
        rscpEE_word(rscp_binaryInputChannel, port, ptr);
    rscp_binaryInputChannels[i].flags =
        rscpEE_byte(rscp_binaryInputChannel, flags, ptr);

//    RSCP_DEBUG_CONF(
//        "binary input: port: %d - flags: %02x -> %c%c%c\n", rscp_binaryInputChannels[i].port, rscp_binaryInputChannels[i].flags, rscp_binaryInputChannels[i].pullup ? 'P' : 'p', rscp_binaryInputChannels[i].negate ? 'N' : 'n', rscp_binaryInputChannels[i].report ? 'R' : 'r');

    rscp_setPortDDR(rscp_binaryInputChannels[i].port, 0);
    rscp_setPortPORT(rscp_binaryInputChannels[i].port,
        rscp_binaryInputChannels[i].pullup);

    ptr += offsetof(rscp_binaryInputChannel, status);
  }
}

void rscp_parseBOC(void *ptr, uint16_t items) {
  rscp_binaryOutputChannels = malloc(items * sizeof(rscp_binaryOutputChannel));
  if (rscp_binaryOutputChannels == NULL ) {
    rscp_numBinaryOutputChannels = 0;
    RSCP_DEBUG_CONF("out of memory\n");
    return;
  }

  rscp_numBinaryOutputChannels = items;
  memset(rscp_binaryOutputChannels, 0,
      items * sizeof(rscp_binaryOutputChannel));
  RSCP_DEBUG_CONF(
      "allocated %d bytes for %d binary output channels\n", items * sizeof(rscp_binaryOutputChannel), items);

  for (uint16_t i = 0; i < items; ++i) {
    rscp_binaryOutputChannels[i].channel =
        rscpEE_word(rscp_binaryOutputChannel, channel, ptr);
    rscp_binaryOutputChannels[i].port =
        rscpEE_word(rscp_binaryOutputChannel, port, ptr);
    rscp_binaryOutputChannels[i].flags =
        rscpEE_byte(rscp_binaryOutputChannel, flags, ptr);

    RSCP_DEBUG_CONF(
        "binary output: port:%d - flags: 0x%02x -> %c%c%c%c\n", rscp_binaryOutputChannels[i].port, rscp_binaryOutputChannels[i].flags, rscp_binaryOutputChannels[i].openDrain ? 'D' : 'd', rscp_binaryOutputChannels[i].openSource ? 'S' : 's', rscp_binaryOutputChannels[i].negate ? 'N' : 'n', rscp_binaryOutputChannels[i].report ? 'R' : 'r');

    rscp_setPortDDR(rscp_binaryOutputChannels[i].port, 1);
    rscp_setPortPORT(rscp_binaryOutputChannels[i].port,
        rscp_binaryOutputChannels[i].negate);

    ptr += sizeof(rscp_binaryOutputChannel);
  }
}


/* ---------------------------------------------------------------------------
 * change of button state
 */

void
rscp_IOChannels_periodic(void)
{
  /* Check all configured inputs */
  for (uint8_t i = 0; i < rscp_numBinaryInputChannels; i++)
  {
    rscp_binaryInputChannel *bic = &rscp_binaryInputChannels[i];

    /* get current value from portpin... */
    volatile uint8_t portState =
        *((portPtrType) pgm_read_word(&rscp_portConfig[bic->port].portIn));
    uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[bic->port].pin);
    uint8_t curState = (portState & bit ? 1 : 0) ^ (bic->negate ? 1 : 0);

    /* current state hasn't changed since the last read... */
    if (bic->lastRawState == curState)
    {
      /* if the current button state is different from the last stable state,
       * run the debounce timer. Also keep the debounce timer running if the
       * button is pressed, because we need it for long press/repeat
       * recognition */
      if (bic->lastRawState != bic->lastDebouncedState) {
        RSCP_DEBUG_IO("c %d debounce for: %d - %d (%d)\n", bic->channel,
          curState, bic->debounceCounter, bic->debounceDelay);
        bic->debounceCounter++;
      }
    }
    else
    {
      /* current state has changed since the last read.
       * restart the debounce timer */
      RSCP_DEBUG_IO("c %d raw change to: %d\n", bic->channel, curState);
      bic->debounceCounter = 0;
      bic->lastRawState = curState;
    }

    /* button was stable for debounceDelay * 20 ms */
    if (bic->lastRawState != bic->lastDebouncedState &&
        bic->debounceDelay <= bic->debounceCounter)
    {
      bic->lastDebouncedState = bic->lastRawState;
      RSCP_DEBUG_IO("Debounced BinaryInputChannel %hu changed to %hu\n",
        bic->channel, bic->lastDebouncedState);
      rscp_txBinaryIOChannelChange(bic->channel, bic->lastDebouncedState);
    }
  }

  /* Check all configured outputs */
  for (uint8_t i = 0; i < rscp_numBinaryOutputChannels; i++)
  {
    rscp_binaryOutputChannel *boc = &rscp_binaryOutputChannels[i];
    rscp_pollBinaryOutputChannelState(boc);
  }
}

void rscp_pollBinaryOutputChannelState(rscp_binaryOutputChannel *boc)  {
	/* get current value from portpin... */
	volatile uint8_t portState = *((portPtrType) pgm_read_word(&rscp_portConfig[boc->port].portIn));
	uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[boc->port].pin);
	uint8_t curState = (portState & bit ? 1 : 0) ^ (boc->negate ? 1 : 0);

	/* current state hasn't changed since the last read... */
	if (boc->lastState != curState)
	{
	  boc->lastState = curState;
	  RSCP_DEBUG_IO("BinaryOutputChannel %hu changed to %hu\n", boc->channel,
		boc->lastState);
	  rscp_txBinaryIOChannelChange(boc->channel, boc->lastState);
	}
}
#endif /* RSCP_SUPPORT */

/*
  -- Ethersex META --
  header(protocols/rscp/rscp_io.h)
  timer(1, rscp_IOChannels_periodic())
  block(Miscelleanous)
 */
