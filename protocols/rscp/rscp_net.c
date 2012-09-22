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

//rscp_udpinit(void)
//{
//  uip_udp_conn_t *conn;
//  uip_ipaddr_t ip;
//  uip_ipaddr_copy(&ip, all_ones_addr);
//  if (!(conn = uip_udp_new(&ip, 0, vscp_net_udp)))
//  {
//    RSCP_DEBUG("couldn't bind to UDP port %d\n", RSCP_ETHTYPE);
//    return(FALSE);                     /* couldn't bind socket */
//  }
//
//  // uip_udp_bind(conn, HTONS(RSCP_ETHTYPE));
//  RSCP_DEBUG("listening on UDP port %d\n", RSCP_ETHTYPE);
//  return(TRUE);
//}

#ifdef RSCP_USE_RAW_ETHERNET
void
rscp_net_raw(void)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct rscp_message *rscp = (struct rscp_message *) &uip_buf[RSCP_RAWH_LEN];

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

rscp_networkMode_t rscp_networkMode = rscp_ModeUDP;

//rscp_net_udp(void)
//{
//  if (!uip_newdata())
//    return;
//
//  struct rscp_udp_event *rscp = (struct rscp_udp_event *) uip_appdata;
//
//  RSCP_DEBUG("received %d bytes UDP data containing %d bytes RSCP data\n",
//             uip_len, ntohs(rscp->size));
//
//  if (uip_len > 512 ||
//      uip_len - rscp_UDP_POS_DATA - rscp_CRC_LEN < ntohs(rscp->size))
//  {
//    rscp_DEBUG("ethernet frame has wrong size\n");
//    uip_len = 0;
//    return;
//  }
//  uip_len = 0;
//
//  rscp_DEBUG("HEAD : 0x%02X\n", rscp->head);
//
//  rscp_get(rscp_MODE_UDP, ntohs(rscp->class), ntohs(rscp->type),
//           ntohs(rscp->size), rscp->guid, rscp->data);
//}


uint8_t*
rscp_getPayloadPointer()
{
  switch(rscp_networkMode) {
    case rscp_ModeRawEthernet:
      return (((rscp_message_t *) &uip_buf[RSCP_RAWH_LEN])->payload);

    case rscp_ModeUDP:
    default:
      return (((rscp_message_t *) &uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN])->payload);
  }
}



void
rscp_transmit(uint16_t payload_len, uint16_t msg_type)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  rscp_message_t *rscp_message;

  switch(rscp_networkMode) {
    case rscp_ModeRawEthernet:
      rscp_message = (struct rscp_message *) &uip_buf[RSCP_RAWH_LEN];
      break;

    case rscp_ModeUDP:
    default:
      rscp_message = (struct rscp_message *) &uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN];
      break;
  }

  memset(packet->dest.addr, 0xFF, 6);                   // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);        // our mac

  rscp_message->version = 0x0;
  rscp_message->header_len = RSCP_HEADER_LEN;
  rscp_message->timestamp = htonl(0xaabbccdd);
  rscp_message->msg_type = htons(msg_type);
  rscp_message->payload_len = htons(payload_len);

  switch (rscp_networkMode) {
    case rscp_ModeRawEthernet:
      packet->type = HTONS(RSCP_ETHTYPE);
      uip_len = RSCP_RAWH_LEN + RSCP_RAW_POS_DATA + payload_len;
      transmit_packet();
      break;

    case rscp_ModeUDP:
      packet->type = HTONS(UIP_ETHTYPE_IP);

      // UDP broadcast
      for(int i=0; i<4; i++)
        uip_udp_conn->ripaddr[i] = uip_hostaddr[i] | ~uip_netmask[i];

      uip_udp_conn->rport = HTONS(RSCP_ETHTYPE);
      uip_slen = RSCP_HEADER_LEN + payload_len;
      uip_process(UIP_UDP_SEND_CONN);
      router_output();

      RSCP_DEBUG("Sent UDP packet %d (%d)\n", payload_len, uip_slen);
      break;

    default:
      uip_len = 0;
      break;
  }
}

#endif /* RSCP_SUPPORT */
