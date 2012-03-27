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
#include "protocols/vscp/vscp_class.h"
#include "protocols/vscp/vscp_type.h"
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
    printf_P(PSTR("%s%02x"), ((i > 0) ? ":" : ""), event->data[i]);

  printf_P(PSTR("\n"));
#endif /* !DEBUG_VSCP */

  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct vscp_raw_event *vscp =
    (struct vscp_raw_event *) &uip_buf[VSCP_RAWH_LEN];

  if (event->class == HTONS(VSCP_CLASS1_PROTOCOL) ||
      event->class == HTONS(VSCP_CLASS2_LEVEL1_PROTOCOL))
  {
    switch (event->type)
    {
      case HTONS(VSCP_TYPE_PROTOCOL_READ_REGISTER):
        VSCP_DEBUG("0x%02x read register 0x%02x\n",
                   VSCP_TYPE_PROTOCOL_READ_REGISTER, event->data[17]);

        if (memcmp (&vscp->event.guid, &vscp->event.guid, 16))
          return;
        
//       vscp_readRegister(pEvent, &outEvent);

        memset(packet->dest.addr, 0xff, 6);     // broadcast
        memcpy(packet->src.addr, uip_ethaddr.addr, 6);  // our mac
        packet->type = HTONS(VSCP_ETHTYPE);     // vscp raw packet
        vscp->version = 0;
        vscp->head = htonl(0xE0000000);
        vscp->subsource = htons(0);
        vscp->timestamp = htonl(clock_get_time() * 1000);
        vscp->event.class = htons(0);
        vscp->event.type = htons(10);
        memset(&vscp->event.guid[0], 0xff, 7);
        memset(&vscp->event.guid[7], 0xfe, 1);
        memcpy(&vscp->event.guid[8], uip_ethaddr.addr, 6);
        memset(&vscp->event.guid[14], 0x00, 2);
        vscp->event.size = htons(2);
        vscp->event.data[0] = event->data[17];
        vscp->event.data[1] = 42;
        uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + 2;
        break;

      case HTONS(VSCP_TYPE_PROTOCOL_WRITE_REGISTER):
        VSCP_DEBUG("0x%02x write register 0x%02x\n",
                   VSCP_TYPE_PROTOCOL_WRITE_REGISTER, event->data[17]);

        if (memcmp (&vscp->event.guid, &vscp->event.guid, 16))
          return;
//        vscp_writeRegister(pEvent, &outEvent);
        break;

      case HTONS(VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO):

//        outEvent.head = (VSCP_PRIORITY_MEDIUM << 5);
//        outEvent.sizeData = 23;
//        outEvent.vscp_class = VSCP_CLASS1_PROTOCOL;
//        outEvent.vscp_type = VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE;
//        // GUID is filled in by the send routine automatically

//        outEvent.data[16] = 16;   // 16 rows in matrix
//        outEvent.data[17] = 16;   // Matrix offset
//        outEvent.data[18] = 0;    // Page stat
//        outEvent.data[19] = 0;
//        outEvent.data[20] = 0;    // Page end
//        outEvent.data[21] = 0;
//        outEvent.data[22] = 0;    // Size of row for Level II

//        vscp_sendEvent(&outEvent);
        break;

      default:
        VSCP_DEBUG("unsupported type 0x%04x\n", ntohs(event->type));
    }
  }
  else if (event->class == HTONS(VSCP_CLASS1_ALARM) ||
           event->class == HTONS(VSCP_CLASS2_LEVEL1_ALARM))
  {
  }
  else if (event->class == HTONS(VSCP_CLASS1_MEASUREMENT) ||
           event->class == HTONS(VSCP_CLASS2_LEVEL1_MEASUREMENT))
  {
  }
  else if (event->class == HTONS(VSCP_CLASS1_INFORMATION) ||
           event->class == HTONS(VSCP_CLASS2_LEVEL1_INFORMATION))
  {
  }
  else if (event->class == HTONS(VSCP_CLASS1_CONTROL) ||
           event->class == HTONS(VSCP_CLASS2_LEVEL1_CONTROL))
  {
  }
  else if (event->class == HTONS(VSCP_CLASS2_PROTOCOL))
  {
    switch (event->type)
    {
      case HTONS(VSCP2_TYPE_PROTOCOL_READ_REGISTER):
        if (memcmp (&vscp->event.guid, &vscp->event.guid, 16))
          return;
        
//        vscp_readRegister(pEvent, &outEvent);
        break;

      case HTONS(VSCP2_TYPE_PROTOCOL_WRITE_REGISTER):
        if (memcmp (&vscp->event.guid, &vscp->event.guid, 16))
          return;
//        vscp_writeRegister( pEvent, &outEvent );

      default:
        VSCP_DEBUG("unsupported type 0x%04x\n", ntohs(event->type));
    }
  }
  else
  {
    VSCP_DEBUG("unsupported class 0x%04x type 0x%04x\n",
               ntohs(event->class), ntohs(event->type));
  }


  // Feed Event into decision matrix
//  vscp_feedDM(pEvent);

}
#endif /* !VSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/vscp/vscp.h)
   init(vscp_init)
   mainloop(vscp_main)
   block(Miscelleanous)
 */
