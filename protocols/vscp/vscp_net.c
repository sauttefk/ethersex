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

#include <avr/io.h>
#include <string.h>

#include "protocols/uip/uip.h"
#include "protocols/uip/uip_router.h"

#include "protocols/vscp/vscp.h"


#ifdef VSCP_SUPPORT
#ifdef VSCP_UDP_SUPPORT
void
vscp_net_init(void)
{
  uip_udp_conn_t *conn;
  uip_ipaddr_t ip;
  uip_ipaddr_copy(&ip, all_ones_addr);
  if (!(conn = uip_udp_new(&ip, 0, vscp_net_udp)))
    return;                     /* couldn't bind socket */

  uip_udp_bind(conn, HTONS(CONF_VSCP_PORT));
  VSCP_DEBUG("listening on UDP port %d\n", CONF_VSCP_PORT);
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

  uint16_t class = ntohs(vscp->class);
  uint16_t type = ntohs(vscp->type);
  uint16_t size = ntohs(vscp->size);

  vscp_get(VSCP_MODE_UDP, class, type, size, vscp->guid, vscp->data);
}
#endif /* !VSCP_UDP_SUPPORT */



#ifdef VSCP_RAWETHERNET_SUPPORT
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
  memset(&oguid[0], 0xff, 7);               // raw ethernet first 8 bytes
  oguid[7] = 0xFE;                          // = 0xFFFFFFFFFFFFFFFE
  memcpy(&oguid[8], &packet->src.addr, 6);  // sending mac addres
  memcpy(&oguid[14], &vscp->subsource, 2);  // subsource = lower 2 bytes GUID

  uint16_t class = ntohs(vscp->class);
  uint16_t type = ntohs(vscp->type);
  uint16_t size = ntohs(vscp->size);

  vscp_get(VSCP_MODE_RAWETHERNET, class, type, size, &oguid, vscp->data);
}
#endif /* !VSCP_RAWETHERNET_SUPPORT */
#endif /* !VSCP_SUPPORT */


/*
  -- Ethersex META --
  ifdef(`conf_VSCP', `net_init(vscp_net_init)')
 */
