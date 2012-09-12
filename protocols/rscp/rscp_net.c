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

#include "rscp.h"
#include "core/bool.h"
#include "protocols/uip/uip_router.h"


#ifdef RSCP_SUPPORT

#ifdef RSCP_USE_RAW_ETHERNET
void
rscp_net_raw(void)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct rscp_raw_event *rscp =
    (struct rscp_raw_event *) &uip_buf[RSCP_RAWH_LEN];

  RSCP_DEBUG("received %d bytes RAW data containing %d bytes RSCP data\n",
             uip_len, ntohs(rscp->payload_len));

/*  if (uip_len < 60 ||
      uip_len > 512 ||
      uip_len - RSCP_RAWH_LEN - RSCP_RAW_POS_DATA < ntohs(rscp->size))
  {
    RSCP_DEBUG("ethernet frame has wrong size\n");
    uip_len = 0;
    return;
  } */
  uip_len = 0;

  RSCP_DEBUG("VERS : 0x%01X\n", rscp->version);
  RSCP_DEBUG("HDLEN: 0x%01X\n", rscp->header_len);
  RSCP_DEBUG("RSVD : 0x%02X\n", rscp->reserved);
  RSCP_DEBUG("TIMES: 0x%08lX\n", ntohl(rscp->timestamp));

  rscp_get(&packet->src.addr, ntohs(rscp->msg_type), ntohs(rscp->payload_len),
           rscp->payload);
}
#endif /* RSCP_USE_RAW_ETHERNET */



uint8_t*
rscp_getPayloadPointer()
{
  return (((struct rscp_raw_event *) &uip_buf[RSCP_RAWH_LEN])->payload);
}



void
rscp_transmit(uint16_t payload_len, uint16_t msg_type)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct rscp_raw_event *rscp_raw =
    (struct rscp_raw_event *) &uip_buf[RSCP_RAWH_LEN];

  memset(packet->dest.addr, 0xFF, 6);                   // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);        // our mac

  packet->type = HTONS(RSCP_ETHTYPE);
  rscp_raw->version = 0x0;
  rscp_raw->header_len = RSCP_HEADER_LEN;
  rscp_raw->timestamp = htonl(0);
  rscp_raw->msg_type = htons(msg_type);
  rscp_raw->payload_len = htons(payload_len);
  uip_len = RSCP_RAWH_LEN + RSCP_RAW_POS_DATA + payload_len;
  transmit_packet();
}

#endif /* RSCP_SUPPORT */
