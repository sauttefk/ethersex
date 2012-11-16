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

#ifdef RSCP_SUPPORT

static rscp_nodeAddress srcAddress;

#ifdef RSCP_USE_RAW_ETHERNET
void rscp_net_raw(void) {
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct rscp_message *rscp = (struct rscp_message *) &uip_buf[RSCP_RAWH_LEN];

  RSCP_DEBUG_NET(
      "received %d bytes RAW data containing %d bytes RSCP data\n", uip_len, ntohs(rscp->payload_len));

  uip_len = 0;

  RSCP_DEBUG_NET("VERS : 0x%01X\n", rscp->version);
  RSCP_DEBUG_NET("HDLEN: 0x%01X\n", rscp->header_len);
  RSCP_DEBUG_NET("TIMES: 0x%08lX\n", ntohl(rscp->timestamp));

  srcAddress.type = rscp_ModeRawEthernet;
  memcpy(srcAddress.u.ethNodeAddress.macAddress.addr, packet->src.addr,
      sizeof(srcAddress.u.ethNodeAddress.macAddress));

  rscp_handleMessage(&srcAddress, ntohs(rscp->msg_type),
      ntohs(rscp->payload_len), rscp->payload);
}
#endif /* RSCP_USE_RAW_ETHERNET */

#ifdef RSCP_USE_UDP
#define BUF ((struct uip_udpip_hdr *) (uip_appdata - UIP_IPUDPH_LEN))

void rscp_netUdp(void) {
  if (!uip_newdata())
    return;

  struct rscp_udp_message *rscp = (struct rscp_udp_message *) uip_appdata;

  RSCP_DEBUG_NET(
      "received %d bytes UDP data containing %d bytes RSCP data\n", uip_len, ntohs(rscp->message.payload_len));

  uip_len = 0;

  RSCP_DEBUG_NET("VERS : 0x%02X\n", rscp->message.version);
  RSCP_DEBUG_NET("HDLEN: 0x%02X\n", rscp->message.header_len);
  RSCP_DEBUG_NET("TIMES: 0x%08lX\n", ntohl(rscp->message.timestamp));

  srcAddress.type = rscp_ModeUDP;
  memcpy(srcAddress.u.ipNodeAddress.macAddress.addr, rscp->mac.addr,
      sizeof(srcAddress.u.ipNodeAddress.macAddress));
  memcpy(srcAddress.u.ipNodeAddress.ipAddress, BUF ->srcipaddr,
      sizeof(srcAddress.u.ipNodeAddress.ipAddress));

  rscp_handleMessage(&srcAddress, ntohs(rscp->message.msg_type),
      ntohs(rscp->message.payload_len), rscp->message.payload);
}
#endif /* RSCP_USE_UDP */

void rscp_net_init(void) {
#ifdef RSCP_USE_UDP
  uip_udp_conn_t *rscp_conn;
  uip_ipaddr_t ip;
  uip_ipaddr_copy(&ip, all_ones_addr);
  if (!(rscp_conn = uip_udp_new(&ip, 0, rscp_netUdp)))
    RSCP_DEBUG_NET("couldn't bind to UDP port %d\n", RSCP_ETHTYPE);

  uip_udp_bind(rscp_conn, HTONS(RSCP_ETHTYPE));
  RSCP_DEBUG_NET("listening on UDP port %d\n", RSCP_ETHTYPE);
#endif /* RSCP_USE_UDP */
}

rscp_networkMode_t rscp_networkMode = rscp_ModeUDP;
//rscp_networkMode_t rscp_networkMode = rscp_ModeRawEthernet;

rscp_payloadBuffer_t rscp_payloadBuffer;

rscp_payloadBuffer_t*
rscp_getPayloadBuffer() {
  switch (rscp_networkMode) {
  case rscp_ModeRawEthernet:
    rscp_payloadBuffer.pos = rscp_payloadBuffer.start =
        (((rscp_message_t *) &uip_buf[RSCP_RAWH_LEN])->payload);
        break;

        case rscp_ModeUDP:
        default:
        rscp_payloadBuffer.pos = rscp_payloadBuffer.start =
        ((struct rscp_udp_message *) &uip_buf[UIP_LLH_LEN +
            UIP_IPUDPH_LEN])->message.payload;
        break;
      }

      return &rscp_payloadBuffer;
    }

void rscp_transmit(uint16_t msg_type) {
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  rscp_message_t *rscp_message;
  rscp_udp_message_t *rscp_udp_message;
  uint16_t payload_len = rscp_payloadBuffer.pos - rscp_payloadBuffer.start;

  switch (rscp_networkMode) {
  case rscp_ModeRawEthernet:
    rscp_message = (struct rscp_message *) &uip_buf[RSCP_RAWH_LEN];
    break;

    case rscp_ModeUDP:
    default:
    rscp_udp_message =
    (struct rscp_udp_message *) &uip_buf[UIP_LLH_LEN + UIP_IPUDPH_LEN];
    memcpy(rscp_udp_message->mac.addr, uip_ethaddr.addr, 6);

    rscp_message = &(rscp_udp_message->message);
    break;
  }

  memset(packet->dest.addr, 0xFF, 6);                   // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);        // our mac

  rscp_message->version = 0x0;
  rscp_message->header_len = RSCP_HEADER_LEN;
  rscp_message->timestamp = htonl(clock_get_time() * 1000 +
      clock_get_ticks() * 20);
  rscp_message->msg_type = htons(msg_type);
  rscp_message->payload_len = htons(payload_len);

  switch (rscp_networkMode) {
  case rscp_ModeRawEthernet:
    packet->type = HTONS(RSCP_ETHTYPE);
    uip_len = RSCP_RAWH_LEN+ RSCP_HEADER_LEN + payload_len;
    transmit_packet();
    RSCP_DEBUG_NET("Sent RAW RSCP packet %d (%d)\n", payload_len, uip_len);
    break;

    case rscp_ModeUDP:
    packet->type = HTONS(UIP_ETHTYPE_IP);

    // UDP broadcast
    uip_udp_conn_t rscp_conn;
    for(int i=0; i<4; i++)
    rscp_conn.ripaddr[i] = uip_hostaddr[i] | ~uip_netmask[i];

    rscp_conn.rport = HTONS(RSCP_UDP_PORT);
    rscp_conn.lport = HTONS(RSCP_UDP_PORT);
    uip_slen = RSCP_UDP_HEADER_LEN + payload_len;
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
