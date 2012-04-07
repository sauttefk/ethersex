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
  guid[0]  = 0xff;
  guid[1]  = 0xff;
  guid[2]  = 0xff;
  guid[3]  = 0xff;
  guid[4]  = 0xff;
  guid[5]  = 0xff;
  guid[6]  = 0xff;
  guid[7]  = 0xfe;
  guid[8]  = uip_ethaddr.addr[0];
  guid[9]  = uip_ethaddr.addr[1];
  guid[10] = uip_ethaddr.addr[2];
  guid[11] = uip_ethaddr.addr[3];
  guid[12] = uip_ethaddr.addr[4];
  guid[13] = uip_ethaddr.addr[5];
  guid[14] = 0x00;
  guid[15] = 0x00;
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
vscp_get(struct vscp_raw_event *vscp)
{
  VSCP_DEBUG("VERS : 0x%02X\n", vscp->version);
  VSCP_DEBUG("HEAD : 0x%08lX\n", ntohl(vscp->head));
  VSCP_DEBUG("SUB  : 0x%04X\n", ntohs(vscp->subsource));
  VSCP_DEBUG("TIMES: 0x%08lX\n", ntohl(vscp->timestamp));
  VSCP_DEBUG("CLASS: 0x%04X\n", ntohs(vscp->class));
  VSCP_DEBUG("TYPE : 0x%04X\n", ntohs(vscp->type));
  VSCP_DEBUG("DSIZE: %d\n", ntohs(vscp->size));
  VSCP_DEBUG("DATA : ");
#ifdef DEBUG_VSCP
  for (int i = 0; i < ntohs(vscp->size); i++)
    printf_P(PSTR("%s%02X"), ((i > 0) ? ":" : ""), vscp->data[i]);
  printf_P(PSTR("\n"));
#endif /* !DEBUG_VSCP */

  uint8_t guidMismatch = memcmp (&vscp->data, &guid, 16);

  if (vscp->class == HTONS(VSCP_CLASS1_PROTOCOL) ||
      vscp->class == HTONS(VSCP_CLASS2_LEVEL1_PROTOCOL))
  {
    switch (vscp->type)
    {
      case HTONS(VSCP_TYPE_PROTOCOL_SEGCTRL_HEARTBEAT):
        VSCP_DEBUG("0x%02X SEGCTRL_HEARTBEAT\n",
                   VSCP_TYPE_PROTOCOL_SEGCTRL_HEARTBEAT);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE):
        VSCP_DEBUG("0x%02X NEW_NODE_ONLINE\n",
                   VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_PROBE_ACK):
        VSCP_DEBUG("0x%02X PROBE_ACK\n",
                   VSCP_TYPE_PROTOCOL_PROBE_ACK);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_SET_NICKNAME):
        VSCP_DEBUG("0x%02X SET_NICKNAME\n",
                   VSCP_TYPE_PROTOCOL_SET_NICKNAME);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_NICKNAME_ACCEPTED):
        VSCP_DEBUG("0x%02X NICKNAME_ACCEPTED\n",
                   VSCP_TYPE_PROTOCOL_NICKNAME_ACCEPTED);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_DROP_NICKNAME):
        VSCP_DEBUG("0x%02X DROP_NICKNAME\n",
                   VSCP_TYPE_PROTOCOL_DROP_NICKNAME);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_READ_REGISTER):
        VSCP_DEBUG("0x%02X READ_REGISTER 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_READ_REGISTER, vscp->data[17]);
        if (guidMismatch)
          return;
        vscp_readRegister(vscp);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_WRITE_REGISTER):
        VSCP_DEBUG("0x%02X WRITE_REGISTER 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_WRITE_REGISTER, vscp->data[17]);
        if (guidMismatch)
          return;
        vscp_writeRegister(vscp);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_RW_RESPONSE):
        VSCP_DEBUG("0x%02X RW_RESPONSE 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_RW_RESPONSE, vscp->data[17]);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_WHO_IS_THERE):
        VSCP_DEBUG("0x%02X WHO_IS_THERE\n",
                   VSCP_TYPE_PROTOCOL_WHO_IS_THERE);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE):
        VSCP_DEBUG("0x%02X WHO_IS_THERE_RESPONSE\n",
                   VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO):
        VSCP_DEBUG("0x%02X GET_MATRIX_INFO\n",
                   VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO);
        if (guidMismatch)
          return;
        vscp_getMatrixinfo(vscp);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE):
        VSCP_DEBUG("0x%02X GET_MATRIX_INFO_RESPONSE\n",
                   VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE);
        break;


      case HTONS(VSCP_TYPE_PROTOCOL_RESET_DEVICE):
        VSCP_DEBUG("0x%02X RESET_DEVICE\n",
                   VSCP_TYPE_PROTOCOL_RESET_DEVICE);
        break;


      default:
        VSCP_DEBUG("unsupported type 0x%04X\n", ntohs(vscp->type));
    }
  }
  else if (vscp->class == HTONS(VSCP_CLASS1_ALARM) ||
           vscp->class == HTONS(VSCP_CLASS2_LEVEL1_ALARM))
  {
  }
  else if (vscp->class == HTONS(VSCP_CLASS1_MEASUREMENT) ||
           vscp->class == HTONS(VSCP_CLASS2_LEVEL1_MEASUREMENT))
  {
  }
  else if (vscp->class == HTONS(VSCP_CLASS1_INFORMATION) ||
           vscp->class == HTONS(VSCP_CLASS2_LEVEL1_INFORMATION))
  {
  }
  else if (vscp->class == HTONS(VSCP_CLASS1_CONTROL) ||
           vscp->class == HTONS(VSCP_CLASS2_LEVEL1_CONTROL))
  {
  }
  else if (vscp->class == HTONS(VSCP_CLASS2_PROTOCOL))
  {
    switch (vscp->type)
    {
      case HTONS(VSCP2_TYPE_PROTOCOL_READ_REGISTER):
        VSCP_DEBUG("0x%02x read register 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_READ_REGISTER, vscp->data[17]);
        if (guidMismatch)
          return;
        vscp_readRegister(vscp);
        break;


      case HTONS(VSCP2_TYPE_PROTOCOL_WRITE_REGISTER):
        VSCP_DEBUG("0x%02x write register 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_WRITE_REGISTER, vscp->data[17],
                   vscp->data[18]);
        if (guidMismatch)
          return;
        vscp_writeRegister(vscp);
        break;


      case HTONS(VSCP2_TYPE_PROTOCOL_READ_WRITE_RESPONSE):
        VSCP_DEBUG("0x%02x read/write response\n",
                   VSCP2_TYPE_PROTOCOL_READ_WRITE_RESPONSE);
        break;


      default:
        VSCP_DEBUG("unsupported type 0x%04X\n", ntohs(vscp->type));
    }
  }
  else
  {
    VSCP_DEBUG("unsupported class 0x%04X type 0x%04X\n",
               ntohs(vscp->class), ntohs(vscp->type));
  }


  // Feed Event into decision matrix
//  vscp_feedDM(pEvent);

}


void
vscp_readRegister(struct vscp_raw_event *vscp)
{
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_PROTOCOL);
  vscp->type = HTONS(VSCP_TYPE_PROTOCOL_RW_RESPONSE);
  vscp->size = htons(2);
  vscp->data[0] = vscp->data[17];
  vscp->data[1] = 42;
  uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + 2;
}


void
vscp_writeRegister(struct vscp_raw_event *vscp)
{
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_PROTOCOL);
  vscp->type = HTONS(VSCP_TYPE_PROTOCOL_RW_RESPONSE);
  vscp->size = htons(2);
  vscp->data[0] = vscp->data[17];
  vscp->data[1] = 42;
  uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + 2;
}


void
vscp_getMatrixinfo(struct vscp_raw_event *vscp)
{
  vscp->head = HTONL(VSCP_LEVEL2_PRIORITY_MEDIUM);

  vscp->class = HTONS(VSCP_CLASS1_PROTOCOL);
  vscp->type = HTONS(VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE);
  vscp->size = htons(7);
  vscp->data[0] = 16;   // 16 rows in matrix
  vscp->data[1] = 16;   // matrix offset
  vscp->data[2] = 0;    // page start
  vscp->data[3] = 0;
  vscp->data[4] = 0;    // page end
  vscp->data[5] = 0;
  vscp->data[6] = 0;    // size of row for level II
  uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + 7;
}


void
vscp_createHead(struct vscp_raw_event *vscp)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;

  memset(packet->dest.addr, 0xff, 6);             // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);  // our mac
  packet->type = HTONS(VSCP_ETHTYPE);             // vscp raw packet

  vscp->version = 0;                          // version 0
  vscp->head = htonl(0xE0000000);
  vscp->subsource = htons(0x55AA);
  vscp->timestamp = htonl(0);

//  vscp->timestamp = htonl(clock_get_time() * 1000);
//  vscp->size = htons(2);
//  vscp->data[0] = event->data[17];
//  vscp->data[1] = 42;
//  uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + 2;

  return;
}

#endif /* !VSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/vscp/vscp.h)
   init(vscp_init)
   mainloop(vscp_main)
   block(Miscelleanous)
 */
