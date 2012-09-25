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

#include <stdio.h>

#include "rscp_io.h"
#include "protocols/uip/uip.h"

#ifdef RSCP_SUPPORT

#ifdef DEBUG_BUTTONS_INPUT
  /*  For providing the actual name of the buttons for debug output */
  #define STR(_v)  const char _v##_str[] PROGMEM = #_v;
  #define STRLIST(_v) _v##_str,
  #define GET_BUTTON_NAME(i) ((PGM_P)pgm_read_word(&buttonNames[i]))

  /* This creates an array of string in ROM which hold the button names. */
  BTN_CONFIG(STR);
  PGM_P const buttonNames[CONF_NUM_BUTTONS] PROGMEM = { BTN_CONFIG(STRLIST) };
#endif

const ioConfig_t rscp_portConfig[CONF_NUM_BUTTONS] PROGMEM = { BTN_CONFIG(C) };


uint8_t rscp_setPortDDR(uint16_t portID, uint8_t value) {
  portPtrType portDDR = (portPtrType) pgm_read_word(&rscp_portConfig[portID].ddr);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG_IO("set DDR port %d (%x, bit %d): %d\n", portID, pgm_read_word(&rscp_portConfig[portID].ddr), bit, value);

  // set direction
  if(value)
    return *portDDR |= bit;
  else
    return *portDDR &= ~bit;
}


uint8_t rscp_setPortPORT(uint16_t portID, uint8_t value) {
  portPtrType portOut = (portPtrType) pgm_read_word(&rscp_portConfig[portID].portOut);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG_IO("set PORT port %d (%x, bit %d): %d\n", portID, pgm_read_word(&rscp_portConfig[portID].portOut), bit, value);

  // set pullup
  if(value)
    return *portOut |= bit;
  else
    return *portOut &= ~bit;
}


uint8_t rscp_togglePortPORT(uint16_t portID) {
  portPtrType portOut = (portPtrType) pgm_read_word(&rscp_portConfig[portID].portOut);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG_IO("toggle PORT port %d (%x, bit %d)\n", portID, portOut, bit);

  return *portOut ^= bit;
}


uint8_t rscp_getPortPIN(uint16_t portID) {
  portPtrType portIn = (portPtrType) pgm_read_word(&rscp_portConfig[portID].portIn);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);
  return *portIn & bit ? 1 : 0;
}


/* ---------------------------------------------------------------------------
 * change of button state
 */
void
rscp_txBinaryIOChannelChange (uint16_t channel, uint8_t state)
{
  RSCP_DEBUG_IO("BinaryIOChannel: %d status: %d\n", channel, state, state);

  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

  // set channel
  rscp_encodeChannel(channel, buffer);

  // set unit and value
  rscp_encodeUInt8(RSCP_UNIT_BOOLEAN, buffer);
  rscp_encodeBooleanField(state, buffer);

  rscp_transmit(RSCP_CHANNEL_EVENT);
}


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
      /* currect state has changed since the last read.
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
      BUTTONDEBUG("Debounced BinaryInputChannel % changed to %d\n",
        bic->channel, bic->lastDebouncedState);
      rscp_txBinaryIOChannelChange(bic->channel, bic->lastDebouncedState);
    }
  }

  /* Check all configured outputs */
  for (uint8_t i = 0; i < rscp_numBinaryOutputChannels; i++)
  {
    rscp_binaryOutputChannel *boc = &rscp_binaryOutputChannels[i];

    /* get current value from portpin... */
    volatile uint8_t portState =
        *((portPtrType) pgm_read_word(&rscp_portConfig[boc->port].portIn));
    uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[boc->port].pin);
    uint8_t curState = (portState & bit ? 1 : 0) ^ (boc->negate ? 1 : 0);

    /* current state hasn't changed since the last read... */
    if (boc->lastState != curState)
    {
      boc->lastState = curState;
      BUTTONDEBUG("BinaryOutputChannel % changed to %d\n", boc->channel,
        boc->lastState);
      rscp_txBinaryIOChannelChange(boc->channel, boc->lastState);
    }
  }
}
#endif /* RSCP_SUPPORT */

/**
 * -- Ethersex META --
 * header(protocols/rscp/rscp_io.h)
 * timer(1, rscp_IOChannels_periodic())
 * block(Miscelleanous)
 */
