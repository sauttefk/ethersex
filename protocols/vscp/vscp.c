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
vscp_init(void)
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
//        vscp_readRegister(vscp);
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
//        vscp_readRegister(vscp);
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

/*
void
vscp_readRegister(struct vscp_raw_event *vscp)
{
  vscp->size = HTONS(2);
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_PROTOCOL);
  vscp->type = HTONS(VSCP_TYPE_PROTOCOL_RW_RESPONSE);
  vscp->data[0] = vscp->data[17];
  vscp->data[1] = 42;
}


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

    struct vscp_raw_event *vscp =
      (struct vscp_raw_event *) &uip_buf[VSCP_RAWH_LEN];

    vscp_sendHeartBeat(vscp);
    sendPeriodicOutputEvents(vscp);
    sendPeriodicInputEvents(vscp);
  }
}



void
vscp_sendHeartBeat(struct vscp_raw_event *vscp)
{
  vscp->size = HTONS(3);
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_INFORMATION);
  vscp->type = HTONS(VSCP_TYPE_INFORMATION_NODE_HEARTBEAT);
  vscp->data[0] = 0;            // no meaning
  vscp->data[1] = 0x47;         // FIXME: zone
  vscp->data[2] = 0x11;         // FIXME: subzone
//  vscp->data[1] = appcfgGetc( APPCFG_VSCP_EEPROM_REG_MODULE_ZONE );         // Zone
//  vscp->data[2] = ( appcfgGetc( APPCFG_VSCP_EEPROM_REG_MODULE_SUBZONE ) & 0xe0 );   // SubZone
  transmit_packet();
  VSCP_DEBUG("node heartbeat sent\n");
}



void
sendPeriodicOutputEvents(struct vscp_raw_event *vscp)
{
  vscp->size = HTONS(3);
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_DATA);
  vscp->type = HTONS(VSCP_TYPE_DATA_IO);
  vscp->data[0] = VSCP_DATACODING_BYTE | VSCP_DATACODING_INDEX0;
  vscp->data[1] = 0xA5;         // FIXME: output data
  vscp->data[2] = 0xC3;         // FIXME: output data
//  vscp->data[1] = PORTB;
  transmit_packet();
  VSCP_DEBUG("node output data sent\n");
}



void
sendPeriodicInputEvents(struct vscp_raw_event *vscp)
{
  vscp->size = HTONS(3);
  vscp_createHead(vscp);
  vscp->class = HTONS(VSCP_CLASS1_DATA);
  vscp->type = HTONS(VSCP_TYPE_DATA_IO);
  vscp->data[0] = VSCP_DATACODING_BYTE | VSCP_DATACODING_INDEX1;
  vscp->data[1] = 0xA5;         // FIXME: input data
  vscp->data[2] = 0xC3;         // FIXME: input data
//  vscp->data[1] = getInputState();
  transmit_packet();
  VSCP_DEBUG("node input data sent\n");
}



void
vscp_createHead(struct vscp_raw_event *vscp)
{
  struct uip_eth_hdr *packet = (struct uip_eth_hdr *) &uip_buf;

  memset(packet->dest.addr, 0xff, 6);   // broadcast
  memcpy(packet->src.addr, uip_ethaddr.addr, 6);        // our mac
  packet->type = HTONS(VSCP_ETHTYPE);   // vscp raw packet

  vscp->version = 0;            // version 0
  vscp->head = HTONL(VSCP_LEVEL2_PRIORITY_MEDIUM);
  vscp->subsource = htons(guid[14] << 8 | guid[15]);
  vscp->timestamp = htonl(0);
//  vscp->timestamp = htonl(clock_get_time() * 1000);
  uip_len = VSCP_RAWH_LEN + VSCP_RAW_POS_DATA + ntohs(vscp->size);
}



uint8_t
vscp_readAppReg(uint32_t reg)
{
  uint8_t rv;
  uint8_t wrk;
  uint8_t addr;

  if (reg < 128)
  {
/*
    // Standard register space

    if (0 == vscp_page_select)
    {

      // * * * Standard register space * * *

      switch (reg)
      {

        case VSCP_REG_MODULE_ZONE:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_MODULE_ZONE);
          break;

        case VSCP_REG_MODULE_SUBZONE:
          rv = (appcfgGetc(APPCFG_VSCP_EEPROM_REG_MODULE_SUBZONE) & 0xe0);
          break;

        case VSCP_REG_MODULE_CONTROL:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_MODULE_CONTROL);
          break;

        case VSCP_REG_EVENT_INTERVAL:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_EVENT_INTERVAL);
          break;

        case VSCP_REG_INPUT_STATUS:
          rv = getInputState();
          break;

        case VSCP_REG_OUTPUT_STATUS:
          rv = PORTB;
          break;

        case VSCP_REG_ADCTRL_EVENT_PERIODCTRL:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_ADCTRL_EVENT_PERIODCTRL);
          break;

        case VSCP_REG_ADCTRL_ALARM_LOW:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_ADCTRL_ALARM_LOW);
          break;

        case VSCP_REG_ADCTRL_ALARM_HIGH:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_ADCTRL_ALARM_HIGH);
          break;

        case VSCP_REG_OUTPUT_CONTROL0:
        case VSCP_REG_OUTPUT_CONTROL1:
        case VSCP_REG_OUTPUT_CONTROL2:
        case VSCP_REG_OUTPUT_CONTROL3:
        case VSCP_REG_OUTPUT_CONTROL4:
        case VSCP_REG_OUTPUT_CONTROL5:
        case VSCP_REG_OUTPUT_CONTROL6:
        case VSCP_REG_OUTPUT_CONTROL7:
          wrk = appcfgGetc(APPCFG_PORTB);
          rv =
            appcfgGetc(APPCFG_VSCP_EEPROM_REG_OUTPUT_CONTROL0 +
                       (reg - VSCP_REG_OUTPUT_CONTROL0));
          // Fix startup bit state
          rv &= ~(1 << (reg - VSCP_REG_OUTPUT_CONTROL0));
          rv |= (wrk & (1 << (reg - VSCP_REG_OUTPUT_CONTROL0)));
          break;

        case VSCP_REG_INPUT_CONTROL0:
        case VSCP_REG_INPUT_CONTROL1:
        case VSCP_REG_INPUT_CONTROL2:
        case VSCP_REG_INPUT_CONTROL3:
        case VSCP_REG_INPUT_CONTROL4:
        case VSCP_REG_INPUT_CONTROL5:
        case VSCP_REG_INPUT_CONTROL6:
        case VSCP_REG_INPUT_CONTROL7:
          rv =
            appcfgGetc(APPCFG_VSCP_EEPROM_REG_INPUT_CONTROL0 +
                       (reg - VSCP_REG_INPUT_CONTROL0));
          break;

        case VSCP_REG_OUTPUT_PROTECTION_TIME0_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME0_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME1_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME1_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME2_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME2_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME3_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME3_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME4_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME4_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME5_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME5_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME6_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME6_LSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME7_MSB:
        case VSCP_REG_OUTPUT_PROTECTION_TIME7_LSB:
          rv =
            appcfgGetc(APPCFG_VSCP_EEPROM_REG_OUTPUT_PROTECTION_TIME0_MSB +
                       (reg - VSCP_REG_OUTPUT_PROTECTION_TIME0_MSB));
          break;

        case VSCP_REG_ANALOG0_MSB:
          rv = (AdcValues[5] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG1_MSB:
          rv = (AdcValues[6] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG2_MSB:
          rv = (AdcValues[7] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG3_MSB:
          rv = (AdcValues[8] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG4_MSB:
          rv = (AdcValues[2] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG5_MSB:
          rv = (AdcValues[3] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG6_MSB:
          rv = (AdcValues[0] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG7_MSB:
          rv = (AdcValues[1] >> 8) & 0xff;
          break;

        case VSCP_REG_ANALOG0_LSB:
          rv = AdcValues[5] & 0xff;
          break;

        case VSCP_REG_ANALOG1_LSB:
          rv = AdcValues[6] & 0xff;
          break;

        case VSCP_REG_ANALOG2_LSB:
          rv = AdcValues[7] & 0xff;
          break;

        case VSCP_REG_ANALOG3_LSB:
          rv = AdcValues[8] & 0xff;
          break;

        case VSCP_REG_ANALOG4_LSB:
          rv = AdcValues[2] & 0xff;
          break;

        case VSCP_REG_ANALOG5_LSB:
          rv = AdcValues[3] & 0xff;
          break;

        case VSCP_REG_ANALOG6_LSB:
          rv = AdcValues[0] & 0xff;
          break;

        case VSCP_REG_ANALOG7_LSB:
          rv = AdcValues[1] & 0xff;
          break;

        case VSCP_REG_PWM_MSB:
          rv = (getPWMValue() >> 8) & 0xff;
          break;

        case VSCP_REG_PWM_LSB:
          rv = getPWMValue() & 0xff;
          break;

        case VSCP_REG_SERIAL_BAUDRATE:
          rv = appcfgGetc(APPCFG_USART1_BAUD);
          break;

        case VSCP_REG_SERIAL_CONTROL:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_SERIAL_CONTROL);
          break;

        case VSCP_REG_ANALOG_MIN0_MSB:
        case VSCP_REG_ANALOG_MIN0_LSB:
        case VSCP_REG_ANALOG_MIN1_MSB:
        case VSCP_REG_ANALOG_MIN1_LSB:
        case VSCP_REG_ANALOG_MIN2_MSB:
        case VSCP_REG_ANALOG_MIN2_LSB:
        case VSCP_REG_ANALOG_MIN3_MSB:
        case VSCP_REG_ANALOG_MIN3_LSB:
        case VSCP_REG_ANALOG_MIN4_MSB:
        case VSCP_REG_ANALOG_MIN4_LSB:
        case VSCP_REG_ANALOG_MIN5_MSB:
        case VSCP_REG_ANALOG_MIN5_LSB:
        case VSCP_REG_ANALOG_MIN6_MSB:
        case VSCP_REG_ANALOG_MIN6_LSB:
        case VSCP_REG_ANALOG_MIN7_MSB:
        case VSCP_REG_ANALOG_MIN7_LSB:

        case VSCP_REG_ANALOG_MAX0_MSB:
        case VSCP_REG_ANALOG_MAX0_LSB:
        case VSCP_REG_ANALOG_MAX1_MSB:
        case VSCP_REG_ANALOG_MAX1_LSB:
        case VSCP_REG_ANALOG_MAX2_MSB:
        case VSCP_REG_ANALOG_MAX2_LSB:
        case VSCP_REG_ANALOG_MAX3_MSB:
        case VSCP_REG_ANALOG_MAX3_LSB:
        case VSCP_REG_ANALOG_MAX4_MSB:
        case VSCP_REG_ANALOG_MAX4_LSB:
        case VSCP_REG_ANALOG_MAX5_MSB:
        case VSCP_REG_ANALOG_MAX5_LSB:
        case VSCP_REG_ANALOG_MAX6_MSB:
        case VSCP_REG_ANALOG_MAX6_LSB:
        case VSCP_REG_ANALOG_MAX7_MSB:
        case VSCP_REG_ANALOG_MAX7_LSB:
          rv =
            appcfgGetc(APPCFG_VSCP_EEPROM_REG_ANALOG_MIN0_MSB +
                       (reg - VSCP_REG_ANALOG_MIN0_MSB));
          break;

        case VSCP_REG_ANALOG_HYSTERESIS:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_ANALOG_HYSTERESIS);
          break;

        case VSCP_REG_CAN_CONTROL:
          rv = appcfgGetc(APPCFG_VSCP_EEPROM_REG_CAN_CONTROL);
          break;

        case VSCP_REG_CAN_CLASS_MASK_MSB:
        case VSCP_REG_CAN_CLASS_MASK_LSB:
        case VSCP_REG_CAN_TYPE_MASK_MSB:
        case VSCP_REG_CAN_TYPE_MASK_LSB:
        case VSCP_REG_CAN_CLASS_FILTER_MSB:
        case VSCP_REG_CAN_CLASS_FILTER_LSB:
        case VSCP_REG_CAN_TYPE_FILTER_MSB:
        case VSCP_REG_CAN_TYPE_FILTER_LSB:
          rv =
            appcfgGetc(APPCFG_VSCP_EEPROM_REG_CAN_CLASS_MASK_MSB +
                       (reg - VSCP_REG_CAN_CLASS_MASK_MSB));
          break;

        case VSCP_REG_CAN_GUID:
        case VSCP_REG_CAN_GUID + 1:
        case VSCP_REG_CAN_GUID + 2:
        case VSCP_REG_CAN_GUID + 3:
        case VSCP_REG_CAN_GUID + 4:
        case VSCP_REG_CAN_GUID + 5:
        case VSCP_REG_CAN_GUID + 6:
        case VSCP_REG_CAN_GUID + 7:
        case VSCP_REG_CAN_GUID + 8:
        case VSCP_REG_CAN_GUID + 9:
        case VSCP_REG_CAN_GUID + 10:
        case VSCP_REG_CAN_GUID + 11:
        case VSCP_REG_CAN_GUID + 12:
        case VSCP_REG_CAN_GUID + 13:
        case VSCP_REG_CAN_GUID + 14:
          rv =
            appcfgGetc(APPCFG_VSCP_EEPROM_REG_REG_CAN_GUID0 +
                       (reg - VSCP_REG_CAN_GUID));
          break;

        default:
          rv = 0;
      }
    }
    else if (1 == vscp_page_select)
    {

      // * * * DM Rows 0-3 * * *
      rv = appcfgGetc(VSCP_DM_MATRIX_BASE + reg);

    }
    else if (2 == vscp_page_select)
    {

      // * * * DM Rows 5-7 * * *
      rv = appcfgGetc(VSCP_DM_MATRIX_BASE + 0x100 + reg);
    }
    else if (3 == vscp_page_select)
    {

      // * * * DM Rows 8-11 * * *
      rv = appcfgGetc(VSCP_DM_MATRIX_BASE + 0x200 + reg);
    }
    else if (4 == vscp_page_select)
    {

      // * * * DM Rows 12-15 * * *
      rv = appcfgGetc(VSCP_DM_MATRIX_BASE + 0x300 + reg);
    }
  */
  }
  else if (VSCP_REG_ALARMSTATUS == reg)
  {
    rv = vscp_alarmstatus;
    vscp_alarmstatus = 0x00;    /* reset alarm status */
  }
  else if (VSCP_REG_VSCP_MAJOR_VERSION == reg)
  {
    rv = VSCP_MAJOR_VERSION;
  }
  else if (VSCP_REG_VSCP_MINOR_VERSION == reg)
  {
    rv = VSCP_MINOR_VERSION;
  }
  else if (VSCP_REG_NODE_CONTROL == reg)
  {
    // * * * Reserved * * *
    rv = 0;
  }
  else if (VSCP_REG_FIRMWARE_MAJOR_VERSION == reg)
  {
    // * * * Get firmware Major version * * *
    rv = vscp_getFirmwareMajorVersion();
  }

  else if (VSCP_REG_FIRMWARE_MINOR_VERSION == reg)
  {
    // * * * Get firmware Minor version * * *
    rv = vscp_getFirmwareMinorVersion();
  }

  else if (VSCP_REG_FIRMWARE_SUB_MINOR_VERSION == reg)
  {
    // * * * Get firmware Sub Minor version * * *
    rv = vscp_getFirmwareSubMinorVersion();
  }
  else if (reg < VSCP_REG_MANUFACTUR_ID0)
  {
    // * * * Read from persitant locations * * *
    rv = vscp_getUserID(reg - VSCP_REG_USERID0);
  }
  else if ((reg > VSCP_REG_USERID4) && (reg < VSCP_REG_NICKNAME_ID))
  {
    // * * * Manufacturer ID information * * *
    rv = vscp_getManufacturerId(reg - VSCP_REG_MANUFACTUR_ID0);
  }
  else if (VSCP_REG_NICKNAME_ID == reg)
  {
    rv = 0xFF;                  // always undefined for level II
  }
  else if (VSCP_REG_PAGE_SELECT_LSB == reg)
  {
    // * * * Page select LSB * * *
    rv = (vscp_page_select & 0xff);
  }

  else if (VSCP_REG_PAGE_SELECT_MSB == reg)
  {
    // * * * Page select MSB * * *
    rv = (vscp_page_select >> 8) & 0xff;
  }
  else if (VSCP_REG_BOOT_LOADER_ALGORITHM == reg)
  {
    // * * * Boot loader algorithm * * *
    rv = VSCP_BOOTLOADER_NONE;
  }
  else if (VSCP_REG_BUFFER_SIZE == reg)
  {
    // * * * Buffer size * * *
    rv = vscp_getBufferSize();
  }
  else if ((reg > (VSCP_REG_GUID - 1)) && (reg < VSCP_REG_DEVICE_URL))
  {
    // * * * GUID * * *
    rv = vscp_getGUID(reg - VSCP_REG_GUID);
  }
  else if ((reg > (VSCP_REG_DEVICE_URL - 1)) && (reg < VSCP_REG_DEVICE_URL))
  {
    // * * * The device URL * * *
    rv = vscp_getMDF_URL(reg - VSCP_REG_DEVICE_URL);
  }
  else if (reg < 1024)
  {
    // Read internal EEPROM
    rv = appcfgGetc(VSCP_DM_MATRIX_BASE + reg - 0x100);
  }
  else if (reg >= 0x10000)
  {
    // Read external EEPROM
/*    addr = (reg & 0xffff);
    XEEBeginRead(EEPROM_CONTROL, addr);
    rv = XEERead();
    XEEEndRead();
*/
  }
  else if (reg >= 0x20000)
  {
    // Read internal RAM
    rv = *((unsigned char *) (reg & 0x0fff));
  }
  else
  {
    // Return zero for non mapped area read
    //addr = ( reg & 0xffff );
    rv = 0;
  }

  return rv;
}



#endif /* !VSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/vscp/vscp.h)
   init(vscp_init)
   mainloop(vscp_main)
   timer(50, vscp_periodic())
   block(Miscelleanous)
 */
