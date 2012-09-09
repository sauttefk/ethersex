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

#include "rscp_io.h"
#include "protocols/uip/uip.h"
#include "hardware/input/buttons/buttons.h"

#ifdef RSCP_SUPPORT


/* ---------------------------------------------------------------------------
 * change of button state
 */
void rscp_button_handler(btn_ButtonsType button, uint8_t state) {
  RSCP_DEBUG("button %d status: %d\n", button, state);

  if(button > 0)  // button 0 is config button
  {
    uint8_t *payload = rscp_getPayloadPointer();
    switch(state)
    {
     case BUTTON_NOPRESS:
       payload[0] = htons(button - 1);
#warning FIXME: use defines
       payload[2] = 0x09;
       payload[3] = 0xC1;
       rscp_transmit(5, 0x1001);
       break;
     case BUTTON_PRESS:
       payload[0] = htons(button - 1);
#warning FIXME: use defines
       payload[2] = 0x09;
       payload[3] = 0xC0;
       rscp_transmit(5, 0x1001);
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
rscp_io_init(void)
{
  hook_btn_input_register(rscp_button_handler);
}
#endif /* RSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/rscp/rscp_io.h)
   init(rscp_io_init)
   block(Miscelleanous)
 */
