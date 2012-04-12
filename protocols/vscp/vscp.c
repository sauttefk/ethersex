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

#include "vscp.h"
#include "core/bit-macros.h"
#include "core/periodic.h"
#include "core/eeprom.h"
#include "protocols/uip/uip.h"
#include "protocols/uip/uip_router.h"
#ifdef VSCP_SUPPORT


/* ----------------------------------------------------------------------------
 *global variables
 */
uint8_t vscp_alarmstatus;
uint16_t vscp_page_select;



/* ----------------------------------------------------------------------------
 * initialization of VSCP
 */
void
vscp_setup(void)
{
  VSCP_DEBUG("init\n");
  guid[0] = 0xff;
  guid[1] = 0xff;
  guid[2] = 0xff;
  guid[3] = 0xff;
  guid[4] = 0xff;
  guid[5] = 0xff;
  guid[6] = 0xff;
  guid[7] = 0xfe;
  guid[8] = uip_ethaddr.addr[0];
  guid[9] = uip_ethaddr.addr[1];
  guid[10] = uip_ethaddr.addr[2];
  guid[11] = uip_ethaddr.addr[3];
  guid[12] = uip_ethaddr.addr[4];
  guid[13] = uip_ethaddr.addr[5];
  eeprom_restore(vscp_subsource, &guid[14], 2);

  VSCP_DEBUG("GUID : %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:"
             "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
             guid[0], guid[1], guid[2], guid[3], guid[4], guid[5], guid[6],
             guid[7], guid[8], guid[9], guid[10], guid[11], guid[12],
             guid[13], guid[14], guid[15]);

  vscp_init();
}


void
vscp_main(void)
{
}


void
vscp_get(uint8_t mode, uint16_t class, uint16_t type, uint16_t size,
         uint8_t *oguid, uint8_t *data)
{
  VSCP_DEBUG("OGUID: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:"
             "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
             oguid[0], oguid[1], oguid[2], oguid[3], oguid[4], oguid[5],
             oguid[6], oguid[7], oguid[8], oguid[9], oguid[10], oguid[11],
             oguid[12], oguid[13], oguid[14], oguid[15]);
  VSCP_DEBUG("CLASS: 0x%04X\n", class);
  VSCP_DEBUG("TYPE : 0x%04X\n", type);
  VSCP_DEBUG("DSIZE: %d\n", size);
  VSCP_DEBUG("DATA : ");
#ifdef DEBUG_VSCP
  for (int i = 0; i < size; i++)
    printf_P(PSTR("%s%02X"), ((i > 0) ? ":" : ""), data[i]);
  printf_P(PSTR("\n"));
#endif /* !DEBUG_VSCP */

  uint8_t guidMismatch = memcmp(data, guid, 16);

  if (class == VSCP_CLASS1_PROTOCOL ||
      class == VSCP_CLASS2_LEVEL1_PROTOCOL)
  {
    switch (type)
    {
      case VSCP_TYPE_PROTOCOL_SEGCTRL_HEARTBEAT:
        VSCP_DEBUG("0x%02X SEGCTRL_HEARTBEAT\n",
                   VSCP_TYPE_PROTOCOL_SEGCTRL_HEARTBEAT);
        break;


      case VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE:
        VSCP_DEBUG("0x%02X NEW_NODE_ONLINE\n",
                   VSCP_TYPE_PROTOCOL_NEW_NODE_ONLINE);
        break;


      case VSCP_TYPE_PROTOCOL_PROBE_ACK:
        VSCP_DEBUG("0x%02X PROBE_ACK\n", VSCP_TYPE_PROTOCOL_PROBE_ACK);
        break;


      case VSCP_TYPE_PROTOCOL_SET_NICKNAME:
        VSCP_DEBUG("0x%02X SET_NICKNAME\n", VSCP_TYPE_PROTOCOL_SET_NICKNAME);
        break;


      case VSCP_TYPE_PROTOCOL_NICKNAME_ACCEPTED:
        VSCP_DEBUG("0x%02X NICKNAME_ACCEPTED\n",
                   VSCP_TYPE_PROTOCOL_NICKNAME_ACCEPTED);
        break;


      case VSCP_TYPE_PROTOCOL_DROP_NICKNAME:
        VSCP_DEBUG("0x%02X DROP_NICKNAME\n",
                   VSCP_TYPE_PROTOCOL_DROP_NICKNAME);
        break;


      case VSCP_TYPE_PROTOCOL_READ_REGISTER:
        VSCP_DEBUG("0x%02X READ_REGISTER 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_READ_REGISTER, data[17]);
        if (guidMismatch)
          return;
        vscp_readRegister(mode);
        break;


      case VSCP_TYPE_PROTOCOL_WRITE_REGISTER:
        VSCP_DEBUG("0x%02X WRITE_REGISTER 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_WRITE_REGISTER, data[17]);
        if (guidMismatch)
          return;
//        vscp_writeRegister(vscp);
        break;


      case VSCP_TYPE_PROTOCOL_RW_RESPONSE:
        VSCP_DEBUG("0x%02X RW_RESPONSE 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_RW_RESPONSE, data[17]);
        break;


      case VSCP_TYPE_PROTOCOL_WHO_IS_THERE:
        VSCP_DEBUG("0x%02X WHO_IS_THERE\n", VSCP_TYPE_PROTOCOL_WHO_IS_THERE);
        break;


      case VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE:
        VSCP_DEBUG("0x%02X WHO_IS_THERE_RESPONSE\n",
                   VSCP_TYPE_PROTOCOL_WHO_IS_THERE_RESPONSE);
        break;


      case VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO:
        VSCP_DEBUG("0x%02X GET_MATRIX_INFO\n",
                   VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO);
        if (guidMismatch)
          return;
//        vscp_getMatrixinfo(vscp);
        break;


      case VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE:
        VSCP_DEBUG("0x%02X GET_MATRIX_INFO_RESPONSE\n",
                   VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE);
        break;


      case VSCP_TYPE_PROTOCOL_RESET_DEVICE:
        VSCP_DEBUG("0x%02X RESET_DEVICE\n", VSCP_TYPE_PROTOCOL_RESET_DEVICE);
        break;


      default:
        VSCP_DEBUG("unsupported type 0x%04X\n", type);
    }
  }
  else if (class == VSCP_CLASS1_ALARM ||
           class == VSCP_CLASS2_LEVEL1_ALARM)
  {
  }
  else if (class == VSCP_CLASS1_MEASUREMENT ||
           class == VSCP_CLASS2_LEVEL1_MEASUREMENT)
  {
  }
  else if (class == VSCP_CLASS1_INFORMATION ||
           class == VSCP_CLASS2_LEVEL1_INFORMATION)
  {
  }
  else if (class == VSCP_CLASS1_CONTROL ||
           class == VSCP_CLASS2_LEVEL1_CONTROL)
  {
  }
  else if (class == VSCP_CLASS2_PROTOCOL)
  {
    switch (type)
    {
      case VSCP2_TYPE_PROTOCOL_READ_REGISTER:
        VSCP_DEBUG("0x%02x read register 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_READ_REGISTER, data[17]);
        if (guidMismatch)
          return;
        vscp_readRegister(mode);
        break;


      case VSCP2_TYPE_PROTOCOL_WRITE_REGISTER:
        VSCP_DEBUG("0x%02x write register 0x%02X\n",
                   VSCP_TYPE_PROTOCOL_WRITE_REGISTER, data[17], data[18]);
        if (guidMismatch)
          return;
//        vscp_writeRegister(vscp);
        break;


      case VSCP2_TYPE_PROTOCOL_READ_WRITE_RESPONSE:
        VSCP_DEBUG("0x%02x read/write response\n",
                   VSCP2_TYPE_PROTOCOL_READ_WRITE_RESPONSE);
        break;


      default:
        VSCP_DEBUG("unsupported type 0x%04X\n", type);
    }
  }
  else
  {
    VSCP_DEBUG("unsupported class 0x%04X type 0x%04X\n", class, type);
  }


  // Feed Event into decision matrix
//  vscp_feedDM(pEvent);

}

void
vscp_readRegister(uint8_t mode)
{
  uint8_t *data =
    vscp_createHead(mode, VSCP_CLASS1_PROTOCOL,
                    VSCP_TYPE_PROTOCOL_RW_RESPONSE, 2);
  data[0] = data[17];
  data[1] = 42;
}


/*
void
vscp_writeRegister(struct vscp_raw_event *vscp)
{
  vscp->size = HTONS(2);
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_PROTOCOL);
  vscp->type = HTONS(VSCP_TYPE_PROTOCOL_RW_RESPONSE);
  vscp->data[0] = vscp->data[17];
  vscp->data[1] = 42;
}


void
vscp_getMatrixinfo(struct vscp_raw_event *vscp)
{
  vscp->size = HTONS(7);
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_PROTOCOL);
  vscp->type = HTONS(VSCP_TYPE_PROTOCOL_GET_MATRIX_INFO_RESPONSE);
  vscp->data[0] = 16;           // 16 rows in matrix
  vscp->data[1] = 16;           // matrix offset
  vscp->data[2] = 0;            // page start
  vscp->data[3] = 0;
  vscp->data[4] = 0;            // page end
  vscp->data[5] = 0;
  vscp->data[6] = 0;            // size of row for level II
}
*/

void
vscp_periodic(void)
{
  static uint8_t vscp_heartbeatInterval = 3;
  if (--vscp_heartbeatInterval == 0)
  {
    /* send a heartbeat packet every 60 seconds */
    vscp_heartbeatInterval = 60;

    vscp_sendHeartBeat();
    sendPeriodicOutputEvents();
    sendPeriodicInputEvents();
  }
}



void
vscp_sendHeartBeat(void)
{
  uint8_t *data =
    vscp_createHead(VSCP_MODE_RAWETHERNET, VSCP_CLASS1_INFORMATION,
                    VSCP_TYPE_INFORMATION_NODE_HEARTBEAT, 3);
  data[0] = 0;            // no meaning
  data[1] = 0x47;         // FIXME: zone
  data[2] = 0x11;         // FIXME: subzone
//  vscp->data[1] = appcfgGetc( APPCFG_VSCP_EEPROM_REG_MODULE_ZONE );         // Zone
//  vscp->data[2] = ( appcfgGetc( APPCFG_VSCP_EEPROM_REG_MODULE_SUBZONE ) & 0xe0 );   // SubZone
  transmit_packet();
  VSCP_DEBUG("node heartbeat sent\n");
}



void
sendPeriodicOutputEvents(void)
{
  uint8_t *data = vscp_createHead(VSCP_MODE_RAWETHERNET, VSCP_CLASS1_DATA,
                                  VSCP_TYPE_DATA_IO, 3);
  data[0] = VSCP_DATACODING_BYTE | VSCP_DATACODING_INDEX0;
  data[1] = 0xA5;         // FIXME: output data
  data[2] = 0xC3;         // FIXME: output data
//  vscp->data[1] = PORTB;
  transmit_packet();
  VSCP_DEBUG("node output data sent\n");
}



void
sendPeriodicInputEvents(void)
{
  uint8_t *data = vscp_createHead(VSCP_MODE_RAWETHERNET, VSCP_CLASS1_DATA,
                                  VSCP_TYPE_DATA_IO, 3);
  data[0] = VSCP_DATACODING_BYTE | VSCP_DATACODING_INDEX1;
  data[1] = 0xA5;         // FIXME: input data
  data[2] = 0xC3;         // FIXME: input data
//  vscp->data[1] = getInputState();
  transmit_packet();
  VSCP_DEBUG("node input data sent\n");
}



uint8_t*
vscp_createHead(uint8_t mode, uint16_t class, uint16_t type, uint16_t size)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;
  struct vscp_raw_event *vscp_raw =
    (struct vscp_raw_event *) &uip_buf[VSCP_RAWH_LEN];
  struct vscp_udp_event *vscp_udp = (struct vscp_udp_event *) uip_appdata;

  memset(packet->dest.addr, 0xff, 6);                   // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);        // our mac
  switch (mode)
  {
    case VSCP_MODE_RAWETHERNET:
      packet->type = HTONS(VSCP_ETHTYPE);               // vscp raw eth packet
      vscp_raw->version = VSCP_RAWETHERNET_VERSION;         // version 0
      vscp_raw->head = HTONL(VSCP_LEVEL2_PRIORITY_MEDIUM);
      vscp_raw->subsource = htons(guid[14] << 8 | guid[15]);
      vscp_raw->timestamp = htonl(0);
      //  vscp->timestamp = htonl(clock_get_time() * 1000);
      vscp_raw->size = htons(size);
      vscp_raw->class = htons(class);
      vscp_raw->type = htons(type);
      uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + size;
      return (vscp_raw->data);
    case VSCP_MODE_UDP:   // FIXME
      packet->type = HTONS(UIP_ETHTYPE_IP);
      vscp_udp->size = htons(size);
      vscp_udp->class = htons(class);
      vscp_udp->type = htons(type);
      uip_len = VSCP_UDP_POS_DATA - VSCP_CRC_LEN + size;
      return (vscp_udp->data);
    default:
      VSCP_DEBUG("unsupported mode\n");
  }
  return (&uip_buf[VSCP_RAWH_LEN]);
}

#endif /* !VSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/vscp/vscp.h)
   init(vscp_setup)
   mainloop(vscp_main)
   timer(50, vscp_periodic())
   block(Miscelleanous)
 */
