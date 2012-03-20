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
#include "protocols/vscp/vscp.h"

#ifdef VSCP_SUPPORT
void
vscp_net_init(void)
{
  uip_udp_conn_t *conn;
  uip_ipaddr_t ip;
  uip_ipaddr_copy(&ip, all_ones_addr);
  if (!(conn = uip_udp_new(&ip, 0, vscp_net_udp)))
    return;                                         /* couldn't bind socket */

  uip_udp_bind(conn, HTONS(CONF_VSCP_PORT));
  VSCP_DEBUG("listening on UDP port %d\n", CONF_VSCP_PORT);
}

void
vscp_net_udp(void)
{
  if (!uip_newdata())
    return;

  VSCP_DEBUG("received %d bytes data\n", uip_len);

  struct vscp_udp_event *vscp;
  vscp = (struct vscp_udp_event *) uip_appdata;
  
  if (uip_len < 24)
    return;                                                    /* too short */
  if (uip_len > 512)
    return;                                                     /* too long */
//  if (ntohs(vscp->size) > VSCP_MAX_DATA);
//    return;                                                     /* too long */


#ifdef DEBUG_VSCP
  VSCP_DEBUG("HEAD : 0x%02x\n", vscp->head);
  VSCP_DEBUG("CLASS: 0x%04x\n", ntohs(vscp->class));
  VSCP_DEBUG("TYPE : 0x%04x\n", ntohs(vscp->type));
  VSCP_DEBUG("GUID : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:" 
                    "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
             vscp->guid[0], vscp->guid[1], vscp->guid[2], vscp->guid[3],
             vscp->guid[4], vscp->guid[5], vscp->guid[6], vscp->guid[7],
             vscp->guid[8], vscp->guid[9], vscp->guid[10], vscp->guid[11],
             vscp->guid[12], vscp->guid[13], vscp->guid[14], vscp->guid[15]);
  VSCP_DEBUG("DSIZE: %d\n", ntohs(vscp->size));
  VSCP_DEBUG("DATA : ");
  for (int i = 0; i < ntohs(vscp->size); i++)
  {
    printf_P(PSTR("%s%02x"), ((i > 0) ? ":" : ""), vscp->data[i]);
  }
  printf_P(PSTR("\n"));
#endif /* !DEBUG_VSCP */

  if (ntohs(vscp->class) == 512)
  {
    switch (ntohs(vscp->type))
    {
      case 9:
        VSCP_DEBUG("0x09 read register 0x%02x\n", vscp->data[17]);
        break;
      default:
        VSCP_DEBUG("unsupported type 0x%04x\n", ntohs(vscp->type));
    }
  }
  else
  {
    VSCP_DEBUG("unsupported class 0x%04x type 0x%04x\n", ntohs(vscp->class),
               ntohs(vscp->type));
  }
}

void
vcsp_net_raw (void)
{
  VSCP_DEBUG("received %d bytes raw data\n", uip_len);
  
  struct vscp_raw_event *vscp;
  vscp = (struct vscp_raw_event *) &uip_buf[VSCP_RAWH_LEN];

  if (uip_len < 60 || uip_len > 512 
      || uip_len - VSCP_RAWH_LEN - 33 < ntohs(vscp->size) ) {
    VSCP_DEBUG("ethernet frame has wrong size\n");
    uip_len = 0;
    return;
  }
  uip_len = 0;

  #ifdef DEBUG_VSCP
  VSCP_DEBUG("VERS : 0x%02x\n", vscp->version);
  VSCP_DEBUG("HEAD : 0x%08lx\n", ntohl(vscp->head));
  VSCP_DEBUG("SUB  : 0x%04x\n", ntohs(vscp->subsource));
  VSCP_DEBUG("TIMES: 0x%08lx\n", ntohl(vscp->timestamp));
  VSCP_DEBUG("CLASS: 0x%04x\n", ntohs(vscp->class));
  VSCP_DEBUG("TYPE : 0x%04x\n", ntohs(vscp->type));
  VSCP_DEBUG("GUID : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:" 
                    "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n", 
             vscp->guid[0], vscp->guid[1], vscp->guid[2], vscp->guid[3],
             vscp->guid[4], vscp->guid[5], vscp->guid[6], vscp->guid[7],
             vscp->guid[8], vscp->guid[9], vscp->guid[10], vscp->guid[11],
             vscp->guid[12], vscp->guid[13], vscp->guid[14], vscp->guid[15]);
  VSCP_DEBUG("DSIZE: %d\n", ntohs(vscp->size));
  VSCP_DEBUG("DATA : ");
  for (int i = 0; i < ntohs(vscp->size); i++)
  {
    printf_P(PSTR("%s%02x"), ((i > 0) ? ":" : ""), vscp->data[i]);
  }
  printf_P(PSTR("\n"));
#endif /* !DEBUG_VSCP */
}
#endif /* !VSCP_SUPPORT */

/*
   -- Ethersex META --
   net_init(vscp_net_init)
 */
