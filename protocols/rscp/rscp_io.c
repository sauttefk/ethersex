/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
 * (c) 2012 by Daniel Walter <fordprfkt@googlemail.com>
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

/* ---------------------------------------------------------------------------
 * change of button state
 */
void
rscp_io_handler (rscp_io_t button, uint8_t state, uint16_t repeatCnt)
{
  RSCP_DEBUG("button: %d status: %d repeat: %d\n", button, state, repeatCnt);

  if (button > 0)  // button 0 is config button
  {
    uint8_t *payload = rscp_getPayloadPointer();
    payload[0] = 0xff;
    payload[1] = 0xff;
    switch (state)
    {
     case BUTTON_RELEASE:
       payload[0] = (button - 1) >> 8;
       payload[1] = (button - 1) & 0xFF;
       payload[2] = RSCP_UNIT_BOOLEAN;
       payload[3] = RSCP_FIELD_CAT_LEN_IMMEDIATE << 6 | RSCP_FIELD_TYPE_TRUE;
       rscp_transmit(5, RSCP_CHANNEL_EVENT);
       break;
     case BUTTON_PRESS:
       payload[0] = (button - 1) >> 8;
       payload[1] = (button - 1) & 0xFF;
       payload[2] = RSCP_UNIT_BOOLEAN;
       payload[3] = RSCP_FIELD_CAT_LEN_IMMEDIATE << 6 | RSCP_FIELD_TYPE_FALSE;
       rscp_transmit(5, RSCP_CHANNEL_EVENT);
       break;
     case BUTTON_LONGPRESS:
     case BUTTON_REPEAT:
       break;
    }
  }
}

uint8_t rscp_setPortDDR(uint16_t portID, uint8_t value) {
  portPtrType portDDR = (portPtrType) pgm_read_word(&rscp_portConfig[portID].ddr);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG("set DDR port %d (%d, bit %d): %d\n", portID, pgm_read_word(&rscp_portConfig[portID].ddr), bit, value);

  // set direction
  if(value)
    return *portDDR |= bit;
  else
    return *portDDR &= ~bit;
}

uint8_t rscp_setPortPORT(uint16_t portID, uint8_t value) {
  portPtrType portOut = (portPtrType) pgm_read_word(&rscp_portConfig[portID].portOut);
  uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[portID].pin);

  RSCP_DEBUG("set PORT port %d (%d, bit %d): %d\n", portID, pgm_read_word(&rscp_portConfig[portID].ddr), bit, value);

  // set pullup
  if(value)
    return *portOut |= bit;
  else
    return *portOut &= ~bit;
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
rscp_txBinaryInputChannelChange (uint16_t channel, uint8_t state)
{
  RSCP_DEBUG("BinaryInputChannel: %d status: %d\n", channel, state, state);

  uint8_t *payload = rscp_getPayloadPointer();

  // set channel
  ((uint16_t*)payload)[0] = htons(channel);

  // set unit and value
  payload[2] = RSCP_UNIT_BOOLEAN;
  payload[3] = RSCP_FIELD_CAT_LEN_IMMEDIATE << 6 | (state ? RSCP_FIELD_TYPE_TRUE : RSCP_FIELD_TYPE_FALSE);

  rscp_transmit(5, RSCP_CHANNEL_EVENT);
}


/* ---------------------------------------------------------------------------
 * init rscp io
 */
void
rscp_io_init (void)
{
  RSCP_DEBUG("init-io\n");
  BTN_CONFIG(PULLUP);
}
#endif /* RSCP_SUPPORT */

void
rscp_inputChannels_periodic(void)
{
  /* Check all configured buttons */
  for (uint8_t i = 0; i < rscp_numBinaryInputChannels; i++)
  {

    rscp_binaryInputChannel *bic = &rscp_binaryInputChannels[i];

    /* Get current value from portpin... */
    volatile uint8_t portState =
        *((portPtrType) pgm_read_word(&rscp_portConfig[bic->port].portIn));
    uint8_t bit = 1 << pgm_read_byte(&rscp_portConfig[bic->port].pin);
    uint8_t curState = (portState & bit ? 1 : 0) ^ (bic->negate ? 1 : 0);

    /* Actual state hasn't change since the last read... */
    if (bic->lastRawState == curState)
    {
      /* If the current button state is different from the last stable state,
       * run the debounce timer. Also keep the debounce timer running if the
       * button is pressed, because we need it for long press/repeat
       * recognition */
      if (bic->lastRawState != bic->lastDebouncedState) {
        RSCP_DEBUG("c %d debounce for: %d - %d (%d)\n", i, curState, bic->debounceCounter, bic->debounceDelay);
        bic->debounceCounter++;
      }
    }
    else
    {
      /* Actual state has changed since the last read.
       * Restart the debounce timer */
      RSCP_DEBUG("c %d raw change to: %d\n", i, curState);
      bic->debounceCounter = 0;
      bic->lastRawState = curState;
    }

    /* Button was stable for debounceDelay*20 ms */
    if (bic->lastRawState != bic->lastDebouncedState && bic->debounceDelay <= bic->debounceCounter)
    {
      bic->lastDebouncedState = bic->lastRawState;
      BUTTONDEBUG("Debounced BinaryInputChannel % changed to %d\n", i, bic->lastDebouncedState);
      rscp_txBinaryInputChannelChange(i, bic->lastDebouncedState);
    }
  }
}

/**
 * -- Ethersex META --
 * header(protocols/rscp/rscp_io.h)
 * timer(1, rscp_inputChannels_periodic())
 * init(rscp_io_init)
 * block(Miscelleanous)
 */
