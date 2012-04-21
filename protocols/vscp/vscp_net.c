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

#include "vscp.h"
#include "core/bool.h"
#include "protocols/uip/uip_router.h"


#ifdef VSCP_SUPPORT
extern uint8_t guid[VSCP_SIZE_GUID];

#ifdef VSCP_USE_UDP
int8_t
vscp_udpinit(void)
{
  uip_udp_conn_t *conn;
  uip_ipaddr_t ip;
  uip_ipaddr_copy(&ip, all_ones_addr);
  if (!(conn = uip_udp_new(&ip, 0, vscp_net_udp)))
  {
    VSCP_DEBUG("couldn't bind to UDP port %d\n", CONF_VSCP_PORT);
    return(FALSE);                     /* couldn't bind socket */
  }

  uip_udp_bind(conn, HTONS(CONF_VSCP_PORT));
  VSCP_DEBUG("listening on UDP port %d\n", CONF_VSCP_PORT);
  return(TRUE);
}


void
vscp_net_udp(void)
{
  if (!uip_newdata())
    return;

  struct vscp_udp_event *vscp = (struct vscp_udp_event *) uip_appdata;

  VSCP_DEBUG("received %d bytes UDP data containing %d bytes VSCP data\n",
             uip_len, ntohs(vscp->size));

  if (uip_len > 512 ||
      uip_len - VSCP_UDP_POS_DATA - VSCP_CRC_LEN < ntohs(vscp->size))
  {
    VSCP_DEBUG("ethernet frame has wrong size\n");
    uip_len = 0;
    return;
  }
  uip_len = 0;

  VSCP_DEBUG("HEAD : 0x%02X\n", vscp->head);

  vscp_get(VSCP_MODE_UDP, ntohs(vscp->class), ntohs(vscp->type),
           ntohs(vscp->size), vscp->guid, vscp->data);
}
#endif /* VSCP_USE_UDP */



#ifdef VSCP_USE_RAW_ETHERNET
void
vscp_net_raw(void)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct vscp_raw_event *vscp =
    (struct vscp_raw_event *) &uip_buf[VSCP_RAWH_LEN];

  VSCP_DEBUG("received %d bytes RAW data containing %d bytes VSCP data\n",
             uip_len, ntohs(vscp->size));

  if (uip_len < 60 ||
      uip_len > 512 ||
      uip_len - VSCP_RAWH_LEN - VSCP_RAW_POS_DATA < ntohs(vscp->size))
  {
    VSCP_DEBUG("ethernet frame has wrong size\n");
    uip_len = 0;
    return;
  }
  uip_len = 0;

  VSCP_DEBUG("VERS : 0x%02X\n", vscp->version);
  VSCP_DEBUG("HEAD : 0x%08lX\n", ntohl(vscp->head));
  VSCP_DEBUG("TIMES: 0x%08lX\n", ntohl(vscp->timestamp));

  uint8_t oguid[16];                        // fumble originator guid together
  memset(&oguid[0], 0xFF, 7);               // raw ethernet first 8 bytes
  oguid[7] = 0xFE;                          // = 0xFFFFFFFFFFFFFFFE
  memcpy(&oguid[8], &packet->src.addr, 6);  // sending mac addres
  memcpy(&oguid[14], &vscp->subsource, 2);  // subsource = lower 2 bytes GUID

  vscp_get(VSCP_MODE_RAWETHERNET, ntohs(vscp->class), ntohs(vscp->type),
           ntohs(vscp->size), (uint8_t *) &oguid, vscp->data);
}
#endif /* VSCP_USE_RAW_ETHERNET */


void
vscp_transmit(uint8_t mode, uint16_t size, uint16_t class, uint16_t type,
              uint8_t priority)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct vscp_raw_event *vscp_raw =
    (struct vscp_raw_event *) &uip_buf[VSCP_RAWH_LEN];
  struct vscp_udp_event *vscp_udp = (struct vscp_udp_event *) uip_appdata;

  memset(packet->dest.addr, 0xFF, 6);                   // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);        // our mac

  switch (mode)
  {
    case VSCP_MODE_RAWETHERNET:
      packet->type = HTONS(VSCP_ETHTYPE);
      vscp_raw->version = VSCP_RAWETHERNET_VERSION;
      vscp_raw->head = htonl((uint32_t) priority << 24);
      vscp_raw->subsource = htons(guid[14] << 8 | guid[15]);
      vscp_raw->timestamp = htonl(0);
      //  vscp->timestamp = htonl(clock_get_time() * 1000);
      vscp_raw->class = htons(class);
      vscp_raw->type = htons(type);
      vscp_raw->size = htons(size);
      uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + size;
      transmit_packet();
      break;

    case VSCP_MODE_UDP:
      packet->type = HTONS(UIP_ETHTYPE_IP);
      vscp_udp->head = priority;
      vscp_udp->class = htons(class);
      vscp_udp->type = htons(type);
      vscp_udp->size = htons(size);
      uip_udp_conn->rport = HTONS(CONF_VSCP_PORT);
      uip_slen = VSCP_UDP_POS_DATA - VSCP_CRC_LEN + size;
      uip_process(UIP_UDP_SEND_CONN);
      break;

    default:
      uip_len = 0;
  }
}
#endif /* VSCP_SUPPORT */
