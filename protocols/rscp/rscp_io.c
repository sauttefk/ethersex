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

const button_configType buttonConfig[CONF_NUM_BUTTONS] PROGMEM = { BTN_CONFIG(C) };
btn_statusType buttonStatus[CONF_NUM_BUTTONS];


uint8_t
get_button_state(uint16_t portID)
{
  if (buttonStatus[portID].polarity == 0)
  {
    /* active low */
    return (((*((portPtrType) pgm_read_word(&buttonConfig[portID].portIn)) &
            _BV(pgm_read_byte(&buttonConfig[portID].pin))) ==
            _BV(pgm_read_byte(&buttonConfig[portID].pin))) ? 0 : 1);
  }
  else
  {
    /* active high */
    return (((*((portPtrType) pgm_read_word(&buttonConfig[portID].portIn)) &
            _BV(pgm_read_byte(&buttonConfig[portID].pin))) ==
            _BV(pgm_read_byte(&buttonConfig[portID].pin))) ? 1 : 0);
  }
}


void
buttons_periodic(void)
{
  uint8_t curState;

  /* Check all configured buttons */
  for (uint8_t i = 0; i < CONF_NUM_BUTTONS; i++)
  {
    /* Get current value from portpin... */
    curState = get_button_state(i);

    /* Actual state hasn't change since the last read... */
    if (buttonStatus[i].curStatus == curState)
    {
      /* If the current button state is different from the last stable state,
       * run the debounce timer. Also keep the debounce timer running if the
       * button is pressed, because we need it for long press/repeat
       * recognition */
      if ((buttonStatus[i].curStatus != buttonStatus[i].status) ||
          (BUTTON_RELEASE != buttonStatus[i].status))
      {
        buttonStatus[i].debounce++;
      }
    }
    else
    {
      /* Actual state has changed since the last read.
       * Restart the debounce timer */
      buttonStatus[i].debounce = 0;
      buttonStatus[i].curStatus = curState;
    }

    /* Button was stable for DEBOUNCE_TIME*20 ms */
    if (CONF_BTN_DEBOUNCE_TIME <= buttonStatus[i].debounce)
    {
      /* Button is pressed.. */
      if (1 == buttonStatus[i].curStatus)
      {
        switch (buttonStatus[i].status)
        {
          /* ..and was not pressed before. Send the PRESS event */
          case BUTTON_RELEASE:
            buttonStatus[i].status = BUTTON_PRESS;
            BUTTONDEBUG("Pressed %S\n", GET_BUTTON_NAME(i));
            rscp_button_handler(i, buttonStatus[i].status);
            break;

          /* ..and was pressed before. Wait for long press. */
          case BUTTON_PRESS:
            if (CONF_BTN_LONGPRESS_TIME <= buttonStatus[i].debounce)
            {
              /* Long press time reached. Send LONGPRESS event. */
              buttonStatus[i].status = BUTTON_LONGPRESS;
              BUTTONDEBUG("Long press %S\n", GET_BUTTON_NAME(i));
              rscp_button_handler(i, buttonStatus[i].status);
            }
            break;

          /* ..and was long pressed before. Wait for repeat start. */
          case BUTTON_LONGPRESS:
            if (CONF_BTN_REPEAT_TIME <= buttonStatus[i].debounce)
            {
              /* Repeat time reached. Send REPEAT event. */
              buttonStatus[i].status = BUTTON_REPEAT;
              BUTTONDEBUG("Repeat %S\n", GET_BUTTON_NAME(i));
              rscp_button_handler(i, buttonStatus[i].status);
            }
            break;

          /* ..and is in repeat. Send cyclic events. */
          case BUTTON_REPEAT:
            if (CONF_BTN_REPEAT_TIME + CONF_BTN_REPEAT_RATE <=
                buttonStatus[i].debounce)
            {
              buttonStatus[i].status = BUTTON_REPEAT;
              buttonStatus[i].debounce = CONF_BTN_REPEAT_TIME;
              BUTTONDEBUG("Repeat %S\n", GET_BUTTON_NAME(i));
              rscp_button_handler(i, buttonStatus[i].status);
            }
            break;

          default:
            BUTTONDEBUG("Oops! Invalid state.\n");
            break;
        }
      }
      else
      {
        /* Button is not pressed anymore. Send RELEASE. */
        buttonStatus[i].status = BUTTON_RELEASE;
        BUTTONDEBUG("Released %S\n", GET_BUTTON_NAME(i));
        buttonStatus[i].debounce = 0;
        rscp_button_handler(i, buttonStatus[i].status);
      }
    }
  }
}

/* ---------------------------------------------------------------------------
 * change of button state
 */
void rscp_button_handler (btn_ButtonsType button, uint8_t state) {
  RSCP_DEBUG("button %d status: %d\n", button, state);

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


/**
 * -- Ethersex META --
 * header(protocols/rscp/rscp_io.h)
 * timer(1, buttons_periodic())
 * init(rscp_io_init)
 * block(Miscelleanous)
 */
