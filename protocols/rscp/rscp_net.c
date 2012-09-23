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

#include "rscp.h"
#include "core/bool.h"
#include "protocols/uip/uip_router.h"


#ifdef RSCP_SUPPORT

#ifdef RSCP_USE_UDP_ETHERNET
void
rscp_netUdp(void)
{
  if (!uip_newdata())
    return;

  struct rscp_message *rscp = (struct rscp_message *) uip_appdata;

  RSCP_DEBUG_NET("received %d bytes UDP data containing %d bytes RSCP data\n",
    uip_len, ntohs(rscp->payload_len));

  uip_len = 0;

  RSCP_DEBUG_NET("VERS : 0x%01X\n", rscp->version);
  RSCP_DEBUG_NET("HDLEN: 0x%01X\n", rscp->header_len);
  RSCP_DEBUG_NET("RSVD : 0x%02X\n", rscp->reserved);
  RSCP_DEBUG_NET("TIMES: 0x%08lX\n", ntohl(rscp->timestamp));

  rscp_get(rscp->mac, ntohs(rscp->msg_type), ntohs(rscp->payload_len),
    rscp->payload);
}


#endif /* RSCP_USE_UDP_ETHERNET */
void
rscp_net_init(void)
{
#ifdef RSCP_USE_UDP_ETHERNET
  uip_udp_conn_t *rscp_conn;
  uip_ipaddr_t ip;
  uip_ipaddr_copy(&ip, all_ones_addr);
  if (!(rscp_conn = uip_udp_new(&ip, 0, rscp_netUdp)))
    RSCP_DEBUG_NET("couldn't bind to UDP port %d\n", RSCP_ETHTYPE);

  uip_udp_bind(rscp_conn, HTONS(RSCP_ETHTYPE));
  RSCP_DEBUG_NET("listening on UDP port %d\n", RSCP_ETHTYPE);
#endif /* RSCP_USE_UDP_ETHERNET */
}


#ifdef RSCP_USE_RAW_ETHERNET
void
rscp_net_raw(void)
{
  struct rscp_message *rscp = (struct rscp_message *) &uip_buf[RSCP_RAWH_LEN];

  RSCP_DEBUG_NET("received %d bytes RAW data containing %d bytes RSCP data\n",
    uip_len, ntohs(rscp->payload_len));

  uip_len = 0;

  RSCP_DEBUG_NET("VERS : 0x%01X\n", rscp->version);
  RSCP_DEBUG_NET("HDLEN: 0x%01X\n", rscp->header_len);
  RSCP_DEBUG_NET("RSVD : 0x%02X\n", rscp->reserved);
  RSCP_DEBUG_NET("TIMES: 0x%08lX\n", ntohl(rscp->timestamp));

  rscp_get(rscp->mac, ntohs(rscp->msg_type), ntohs(rscp->payload_len),
    rscp->payload);
}
#endif /* RSCP_USE_RAW_ETHERNET */


rscp_networkMode_t rscp_networkMode = rscp_ModeUDP;
//rscp_networkMode_t rscp_networkMode = rscp_ModeRawEthernet;


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
  memcpy(rscp_message->mac, uip_ethaddr.addr, 6);
  rscp_message->timestamp = htonl(0xaabbccdd);
  rscp_message->msg_type = htons(msg_type);
  rscp_message->payload_len = htons(payload_len);

  switch (rscp_networkMode) {
    case rscp_ModeRawEthernet:
      packet->type = HTONS(RSCP_ETHTYPE);
      uip_len = RSCP_RAWH_LEN + RSCP_RAW_POS_DATA + payload_len;
      transmit_packet();
      RSCP_DEBUG_NET("Sent RAW RSCP packet %d (%d)\n", payload_len, uip_len);
      break;

    case rscp_ModeUDP:
      packet->type = HTONS(UIP_ETHTYPE_IP);

      // UDP broadcast
      uip_udp_conn_t rscp_conn;
      for(int i=0; i<4; i++)
        rscp_conn.ripaddr[i] = uip_hostaddr[i] | ~uip_netmask[i];

      rscp_conn.rport = HTONS(RSCP_ETHTYPE);
      rscp_conn.lport = HTONS(RSCP_ETHTYPE);
      uip_slen = RSCP_HEADER_LEN + payload_len;
      uip_udp_conn = &rscp_conn;
      uip_process(UIP_UDP_SEND_CONN);
      router_output();
      RSCP_DEBUG_NET("Sent UDP RSCP packet %d (%d)\n", payload_len, uip_slen);
      uip_slen = 0;
      break;

    default:
      uip_len = 0;
      break;
  }
}

#endif /* RSCP_SUPPORT */


/*
  -- Ethersex META --
  header(protocols/rscp/rscp_net.h)
  init(rscp_net_init)
  block(Miscelleanous)
 */
