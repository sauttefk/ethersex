/*
 * Copyright (c) 2012 by Frank Sautter <ethersix@sautter.com>
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
#include <string.h>
#include <avr/interrupt.h>

#include "config.h"
#include "core/bool.h"
#include "core/bit-macros.h"
#include "protocols/vscp/vscp.h"
#include "protocols/uip/uip.h"
#include "protocols/uip/uip_router.h"
#include "protocols/ecmd/ecmd-base.h"
#ifdef VSCP_SUPPORT


/* ----------------------------------------------------------------------------
 *global variables
 */


/* ----------------------------------------------------------------------------
 * initialization of VSCP
 */
void
vscp_init(void)
{
  VSCP_DEBUG("init\n");
}


void
vscp_main(void)
{
}


static void
vscp_send(uint16_t len)
{
}


void
vscp_get(struct vscp_event *event)
{
  VSCP_DEBUG("CLASS: 0x%04x\n", ntohs(event->class));
  VSCP_DEBUG("TYPE : 0x%04x\n", ntohs(event->type));
  VSCP_DEBUG("GUID : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:" 
                    "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
             event->guid[0], event->guid[1], event->guid[2],
             event->guid[3], event->guid[4], event->guid[5],
             event->guid[6], event->guid[7], event->guid[8],
             event->guid[9], event->guid[10], event->guid[11],
             event->guid[12], event->guid[13], event->guid[14],
             event->guid[15]);
  VSCP_DEBUG("DSIZE: %d\n", ntohs(event->size));
  VSCP_DEBUG("DATA : ");
#ifdef DEBUG_VSCP
  for (int i = 0; i < ntohs(event->size); i++)
    printf_P(PSTR("%s%02x"), ((i > 0) ? ":" : ""),
                  event->data[i]);

  printf_P(PSTR("\n"));
#endif /* !DEBUG_VSCP */

  if (ntohs(event->class) == 512)
  {
    switch (ntohs(event->type))
    {
      case 9:
        VSCP_DEBUG("0x09 read register 0x%02x\n", event->data[17]);
        break;
      default:
        VSCP_DEBUG("unsupported type 0x%04x\n", ntohs(event->type));
    }
  }
  else
  {
    VSCP_DEBUG("unsupported class 0x%04x type 0x%04x\n",
               ntohs(event->class), ntohs(event->type));
  }

}
#endif /* !VSCP_SUPPORT */

/*
   -- Ethersex META --
   header(protocols/vscp/vscp.h)
   init(vscp_init)
   mainloop(vscp_main)
   block(Miscelleanous)
 */
