/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
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

#include "vscp_io.h"
#include "hardware/input/buttons/buttons.h"

#ifdef VSCP_SUPPORT


/* ---------------------------------------------------------------------------
 * change of button state
 */
void vscp_button_handler(btn_ButtonsType button, uint8_t state) {
  VSCP_DEBUG("Button %d Status: %d\n", button, state);

  if(button > 0)
  {
    uint8_t *payload = vscp_getPayloadPointer(VSCP_MODE_RAWETHERNET);
    switch(state)
    {
     case BUTTON_RELEASE:
       payload[0] = VSCP_BUTTON_RELEASE;
#warning FIXME: get zone from config
       payload[1] = 0x00;           // zone
       payload[2] = 0x00;           // subzone
       payload[3] = 0x00;           // MSB button
       payload[4] = button - 1;     // LSB button
       vscp_transmit(VSCP_MODE_RAWETHERNET, 5, VSCP_CLASS1_INFORMATION,
                     VSCP_TYPE_INFORMATION_BUTTON, VSCP_PRIORITY_LOW);
       break;
     case BUTTON_PRESS:
       payload[0] = VSCP_BUTTON_PRESS;
#warning FIXME: get zone from config
       payload[1] = 0x00;           // zone
       payload[2] = 0x00;           // subzone
       payload[3] = 0x00;           // MSB button
       payload[4] = button - 1;     // LSB button
       vscp_transmit(VSCP_MODE_RAWETHERNET, 5, VSCP_CLASS1_INFORMATION,
                     VSCP_TYPE_INFORMATION_BUTTON, VSCP_PRIORITY_LOW);
       break;
     case BUTTON_LONGPRESS:
     case BUTTON_REPEAT:
       break;
    }
  }
}


/* ---------------------------------------------------------------------------
 * get value of hardware inputs
 */
uint32_t
vscp_get_input(void)
{
  return ((((uint32_t) PINE << 24) |    // io4.0 - io4.7
           ((uint32_t) PINF << 16) |    // io3.0 - io3.7
           ((uint32_t) PINC << 8) |     // io2.0 - io2.7
            (uint32_t) PINA));          // io1.0 - io1.7
}


/* ---------------------------------------------------------------------------
 * set hardware outputs
 */
void
vscp_set_output(uint32_t value)
{
  PORTA = (uint8_t) (value);
  PORTC = (uint8_t) (value >> 8);
  PORTF = (uint8_t) (value >> 16);
  PORTE = (uint8_t) (value >> 24);
}


/* ---------------------------------------------------------------------------
 * init vscp io
 */
void
vscp_io_init(void)
{
  hook_btn_input_register(vscp_button_handler);
}
#endif /* VSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/vscp/vscp_io.h)
   init(vscp_io_init)
   block(Miscelleanous)
 */
