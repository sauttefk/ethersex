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
#include "rscp_io.h"
#include "core/bit-macros.h"
#include "core/periodic.h"
#include "core/eeprom.h"
#include "protocols/uip/uip.h"
#include "protocols/uip/uip_router.h"
#include "hardware/onewire/onewire.h"

#ifdef RSCP_SUPPORT

/* ---------------------------------------------------------------------------
 * global variables
 */

uint8_t testid[6];


void rscp_parseChannelDefinitions() {
  // Phase 1: count channels in order to determine storage requirements
  void *p1 = (void *)(rscp_channel_p + RSCP_EEPROM_START);
  for (uint8_t i = 0; i < rscp_channel_items; i++)
  {
    if(p1 >= (void *)(rscp_button_p + RSCP_EEPROM_START)) {
      RSCP_DEBUG_CONF("channel list parser list overrun\n");
      return;
    }
    uint8_t channelType = rscpEE_word(rscp_conf_channel, channelType, p1);
    RSCP_DEBUG_CONF("pointer: %04x - chidx: %d - chantype: 0x%02x\n",
                p1, i, channelType);
    switch (channelType)
    {
      case RSCP_CHANNEL_BINARY_INPUT:
      {
        rscp_numBinaryInputChannels++;
        p1 += RSCP_CHT01_SIZE;
        break;
      }
      case RSCP_CHANNEL_BINARY_OUTPUT:
      {
        rscp_numBinaryOutputChannels++;
        p1 += RSCP_CHT02_SIZE;
        break;
      }
      case RSCP_CHANNEL_COMPLEX_INPUT:
      {
        uint8_t numports =
          rscpEE_byte(rscp_conf_channel, chType11.numports, p1);
        uint8_t numstates =
          rscpEE_byte(rscp_conf_channel, chType11.numstates, p1);
        p1 += RSCP_CHT11_HEADSIZE + numports * RSCP_CHT11_PORT_SIZE +
          numstates * RSCP_CHT11_STATE_SIZE;
        break;
      }
      case RSCP_CHANNEL_COMPLEX_OUTPUT:
      {
        uint8_t numports =
          rscpEE_byte(rscp_conf_channel, chType12.numports, p1);
        uint8_t numstates =
          rscpEE_byte(rscp_conf_channel, chType12.numstates, p1);
        p1 += RSCP_CHT12_HEADSIZE + numports * RSCP_CHT12_PORT_SIZE +
          numstates * RSCP_CHT12_STATE_SIZE;
        break;
      }
      case RSCP_CHANNEL_OWTEMPERATURE:
      {
        p1 += RSCP_CHT30_SIZE;
        break;
      }
      default:
      {
        RSCP_DEBUG_CONF("could not parse channel type 0x%02x --- ABORTING\n",
          channelType);
        return;
      }
    }
  }

  // Phase 2: allocate storage for channels
  RSCP_DEBUG_CONF("Allocating %d binary input channels\n",
    rscp_numBinaryInputChannels);
  rscp_binaryInputChannels =
    malloc(rscp_numBinaryInputChannels * sizeof(rscp_binaryInputChannel));
  memset(rscp_binaryInputChannels, 0,
    rscp_numBinaryInputChannels * sizeof(rscp_binaryInputChannel));

  RSCP_DEBUG_CONF("Allocating %d binary output channels\n",
    rscp_numBinaryOutputChannels);
  rscp_binaryOutputChannels =
    malloc(rscp_numBinaryOutputChannels * sizeof(rscp_binaryOutputChannel));
  memset(rscp_binaryOutputChannels, 0,
    rscp_numBinaryOutputChannels * sizeof(rscp_binaryOutputChannel));

  RSCP_DEBUG_CONF("PORT: %02x %02x %02x %02x \n", PORTA, PORTF, PORTC, PORTE);
  RSCP_DEBUG_CONF("DDR:  %02x %02x %02x %02x \n", DDRA, DDRF, DDRC, DDRE);

  // Phase 3: create channels
  uint16_t bicIndex = 0;
  uint16_t bocIndex = 0;

  p1 = (void *)(rscp_channel_p + RSCP_EEPROM_START);
  for (uint8_t i = 0; i < rscp_channel_items; i++)
  {
    uint16_t channelId = rscpEE_word(rscp_conf_channel, channelId, p1);
    uint8_t channelType = rscpEE_word(rscp_conf_channel, channelType, p1);
    RSCP_DEBUG_CONF("pointer: %04x - chidx: %d - chanid: %d - chantype: 0x%02x\n",
                p1, i, channelId, channelType);
    switch (channelType)
    {
      case RSCP_CHANNEL_BINARY_INPUT:
      {
        rscp_binaryInputChannels[bicIndex].port =
          rscpEE_word(rscp_conf_channel, chType01.port, p1);
        rscp_binaryInputChannels[bicIndex].flags =
          rscpEE_byte(rscp_conf_channel, chType01.flags, p1);

        RSCP_DEBUG_CONF("binary input: port:%d - flags: %02x -> %c%c%c\n",
          rscp_binaryInputChannels[bicIndex].port,
          rscp_binaryInputChannels[bicIndex].flags,
          rscp_binaryInputChannels[bicIndex].pullup ? 'P' : 'p',
          rscp_binaryInputChannels[bicIndex].negate ? 'N' : 'n',
          rscp_binaryInputChannels[bicIndex].report ? 'R' : 'r');

        rscp_setPortDDR(rscp_binaryInputChannels[bicIndex].port, 0);
        rscp_setPortPORT(rscp_binaryInputChannels[bicIndex].port,
          rscp_binaryInputChannels[bicIndex].pullup);

        bicIndex++;
        p1 += RSCP_CHT01_SIZE;
        break;
      }
      case RSCP_CHANNEL_BINARY_OUTPUT:
      {
        rscp_binaryOutputChannels[bocIndex].port =
          rscpEE_word(rscp_conf_channel, chType02.port, p1);
        rscp_binaryOutputChannels[bocIndex].flags =
          rscpEE_byte(rscp_conf_channel, chType02.flags, p1);

        RSCP_DEBUG_CONF("binary output: port:%d - flags: 0x%02x\n",
          rscp_binaryOutputChannels[bocIndex].port,
          rscp_binaryOutputChannels[bocIndex].flags);

        rscp_setPortDDR(rscp_binaryOutputChannels[bocIndex].port, 1);
        rscp_setPortPORT(rscp_binaryOutputChannels[bocIndex].port, 0);

        bocIndex++;
        p1 += RSCP_CHT02_SIZE;
        break;
      }
      case RSCP_CHANNEL_COMPLEX_INPUT:
      {
        uint8_t channelflags =
          rscpEE_byte(rscp_conf_channel, chType11.flags, p1);
        uint8_t numports =
          rscpEE_byte(rscp_conf_channel, chType11.numports, p1);
        uint8_t numstates =
          rscpEE_byte(rscp_conf_channel, chType11.numstates, p1);
        RSCP_DEBUG_CONF("complex input: flags:0x%02x - ports:%d - states:%d\n",
                    channelflags, numports, numstates);
        p1 += RSCP_CHT11_HEADSIZE;
        for (int j = 0; j < numports; j++) {
          uint16_t channelPort =
            eeprom_read_word((void *)(p1 + RSCP_CHT11_PORTID));
          uint8_t channelFlags =
            eeprom_read_byte((void *)(p1 + RSCP_CHT11_PORT_FLAGS));
          RSCP_DEBUG_CONF("pointer: 0x%04x\n", p1);
          RSCP_DEBUG_CONF("complex port: %d - port:%d - flags: 0x%02x\n", j,
            channelPort, channelFlags);
          p1 += RSCP_CHT11_PORT_SIZE;
        }
        for (int j = 0; j < numstates; j++) {
          uint8_t channelState =
            eeprom_read_byte((void *)(p1 + RSCP_CHT11_PORTSTATES));
          RSCP_DEBUG_CONF("complex state: %d - bits: 0x%02x\n", j,
            channelState);
          RSCP_DEBUG_CONF("pointer: 0x%04x\n", p1);
          p1 += RSCP_CHT11_STATE_SIZE;
        }
        break;
      }
      case RSCP_CHANNEL_COMPLEX_OUTPUT:
      {
        uint8_t channelflags =
          rscpEE_byte(rscp_conf_channel, chType12.flags, p1);
        uint8_t numports =
          rscpEE_byte(rscp_conf_channel, chType12.numports, p1);
        uint8_t numstates =
          rscpEE_byte(rscp_conf_channel, chType12.numstates, p1);
        RSCP_DEBUG_CONF("complex output: flags:0x%02x - ports:%d - states:%d\n",
                    channelflags, numports, numstates);
        p1 += RSCP_CHT12_HEADSIZE;
        for (uint16_t j = 0; j < numports; j++) {
          uint16_t channelPort =
            eeprom_read_word((void *)(p1 + RSCP_CHT12_PORTID));
          uint8_t channelFlags =
            eeprom_read_byte((void *)(p1 + RSCP_CHT12_PORT_FLAGS));
          RSCP_DEBUG_CONF("pointer: 0x%04x\n", p1);
          RSCP_DEBUG_CONF("complex port: %d - port:%d - flags: 0x%02x\n", j,
                     channelPort, channelFlags);
          p1 += RSCP_CHT12_PORT_SIZE;
        }
        for (int j = 0; j < numstates; j++) {
          uint8_t channelState =
            eeprom_read_byte((void *)(p1 + RSCP_CHT12_PORTSTATES));
          RSCP_DEBUG_CONF("complex state: %d - bits: 0x%02x\n", j,
            channelState);
          RSCP_DEBUG_CONF("pointer: 0x%04x\n", p1);
          p1 += RSCP_CHT12_STATE_SIZE;
        }
        break;
      }
      case RSCP_CHANNEL_OWTEMPERATURE:
      {
        RSCP_DEBUG_CONF("1WID: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[0], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[1], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[2], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[3], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[4], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[5], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[6], p1),
                    rscpEE_byte(rscp_conf_channel, chType30.owROM[7], p1));
        RSCP_DEBUG_CONF("interval: %d\n", rscpEE_word(rscp_conf_channel,
          chType30.interval, p1));
        RSCP_DEBUG_CONF("tempHi: %d\n", rscpEE_word(rscp_conf_channel,
          chType30.tempHi, p1));
        RSCP_DEBUG_CONF("tempLo: %d\n", rscpEE_word(rscp_conf_channel,
          chType30.tempLo, p1));
        p1 += RSCP_CHT30_SIZE;
        break;
      }
      default:
      {
        RSCP_DEBUG_CONF("could not parse channel type 0x%02x --- ABORTING\n",
          channelType);
      }
    }
  }

  RSCP_DEBUG_CONF("PORT: %02x %02x %02x %02x \n", PORTA, PORTF, PORTC, PORTE);
  RSCP_DEBUG_CONF("DDR:  %02x %02x %02x %02x \n", DDRA, DDRF, DDRC, DDRE);
}


/* ----------------------------------------------------------------------------
 * initialization of RSCP
 */
void
rscp_init(void)
{
  RSCP_DEBUG("init\n");
  RSCP_DEBUG_CONF("start of rscp config in eeprom: 0x%03X\n", RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("free eeprom: %d\n", RSCP_FREE_EEPROM);
  RSCP_DEBUG_CONF("version: %d\n",
              rscpEE_word(rscp_conf_header, version, RSCP_EEPROM_START));
  RSCP_DEBUG_CONF("mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
             rscpEE_byte(rscp_conf_header, mac[0], RSCP_EEPROM_START),
             rscpEE_byte(rscp_conf_header, mac[1], RSCP_EEPROM_START),
             rscpEE_byte(rscp_conf_header, mac[2], RSCP_EEPROM_START),
             rscpEE_byte(rscp_conf_header, mac[3], RSCP_EEPROM_START),
             rscpEE_byte(rscp_conf_header, mac[4], RSCP_EEPROM_START),
             rscpEE_byte(rscp_conf_header, mac[5], RSCP_EEPROM_START));

  if (rscpEE_word(rscp_conf_header, version, RSCP_EEPROM_START) != 1)
  {
    RSCP_DEBUG_CONF("this firmware only supports rscp config version 1\n");
    return;
  }
  if (rscpEE_byte(rscp_conf_header, mac[0], RSCP_EEPROM_START) != uip_ethaddr.addr[0] ||
      rscpEE_byte(rscp_conf_header, mac[1], RSCP_EEPROM_START) != uip_ethaddr.addr[1] ||
      rscpEE_byte(rscp_conf_header, mac[2], RSCP_EEPROM_START) != uip_ethaddr.addr[2] ||
      rscpEE_byte(rscp_conf_header, mac[3], RSCP_EEPROM_START) != uip_ethaddr.addr[3] ||
      rscpEE_byte(rscp_conf_header, mac[4], RSCP_EEPROM_START) != uip_ethaddr.addr[4] ||
      rscpEE_byte(rscp_conf_header, mac[5], RSCP_EEPROM_START) != uip_ethaddr.addr[5])
  {
    RSCP_DEBUG_CONF("the config does not match this device's mac address\n");
    return;
  }

  rscp_channel_items =
    rscpEE_word(rscp_conf_header, channel_items, RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("channel items: %d\n", rscp_channel_items);
  rscp_channel_p =
    rscpEE_word(rscp_conf_header, channel_p, RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("channel pointer: 0x%04X\n", rscp_channel_p);

  rscp_button_items =
    rscpEE_word(rscp_conf_header, button_items, RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("button items: %d\n", rscp_button_items);
  rscp_button_p =
    rscpEE_word(rscp_conf_header, button_p, RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("button pointer: 0x%04X\n", rscp_button_p);

  rscp_rule_items =
    rscpEE_word(rscp_conf_header, rule_items, RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("config items: %d\n", rscp_rule_items);
  rscp_rule_p = rscpEE_word(rscp_conf_header, rule_p, RSCP_EEPROM_START);
  RSCP_DEBUG_CONF("config pointer: 0x%04X\n", rscp_rule_p);

  rscp_parseChannelDefinitions();
}


void
rscp_main(void)
{
  // RSCP_DEBUG("bla\n");
}


void
rscp_get(uint8_t * src_addr, uint16_t msg_type, uint16_t payload_len,
         uint8_t * payload)
{
  RSCP_DEBUG("SRCAD: %02X:%02X:%02X:%02X:%02X:%02X\n", src_addr[0],
             src_addr[1], src_addr[2], src_addr[3], src_addr[4], src_addr[5]);
  RSCP_DEBUG("TYPE : 0x%04X\n", msg_type);
  RSCP_DEBUG("DSIZE: %d\n", payload_len);
  RSCP_DEBUG("DATA : ");
#ifdef DEBUG_RSCP
  for (int i = 0; i < payload_len; i++)
    printf_P(PSTR("%s%02X"), ((i > 0) ? ":" : ""), payload[i]);
  printf_P(PSTR("\n"));
#endif /* DEBUG_RSCP */

  switch (msg_type) {
    case RSCP_CHANNEL_EVENT:
//      uint8_t mismatch = memcmp(&src_addr, testid, 6);
//      if (!mismatch) {
//    }
      if(payload[2] == RSCP_UNIT_BOOLEAN &&
         payload[3] == (RSCP_FIELD_CAT_LEN_IMMEDIATE << 6 |
                        RSCP_FIELD_TYPE_TRUE))
      {
        uint16_t port = htons((*(uint16_t*)&(payload[0]))) + 16;
        if(port > 16) {
#warning FIXME: testcode
          RSCP_DEBUG("** MATCH ** port %d\n", port);
          rscp_togglePortPORT(port);
        }
      }
      break;
  }
}


void
rscp_periodic(void)
{
  static uint8_t rscp_heartbeatInterval = 3;
  if (--rscp_heartbeatInterval == 0)
  {
    /* send a heartbeat packet every 60 seconds */
    rscp_heartbeatInterval = 10;

//    rscp_sendHeartBeat();
//    rscp_sendPeriodicOutputEvents();
//    rscp_sendPeriodicInputEvents();
      rscp_sendPeriodicTemperature();
  }
}


void
rscp_sendHeartBeat(void)
{
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

#warning FIXME

  rscp_transmit(RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node heartbeat sent\n");
}


void
rscp_sendPeriodicOutputEvents(void)
{
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

#warning FIXME

  rscp_transmit(RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node output data sent\n");
}


void
rscp_sendPeriodicInputEvents(void)
{
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

#warning FIXME

  rscp_transmit(RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node input data sent\n");
}


void
rscp_sendPeriodicTemperature(void)
{
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

  // set channel
#warning FIXME
  rscp_encodeChannel(0, buffer);

  // set unit and value
  rscp_encodeUInt8(RSCP_UNIT_TEMPERATURE, buffer);
  rscp_encodeDecimal16Field(ow_sensors[0].temp, -1, buffer);

  RSCP_DEBUG("temp 0x%04x\n", ow_sensors[0].temp);

  rscp_transmit(RSCP_CHANNEL_EVENT);
}
#endif /* RSCP_SUPPORT */

/*
   -- Ethersex META --
   header(protocols/rscp/rscp.h)
   init(rscp_init)
   timer(50, rscp_periodic())
   block(Miscelleanous)
 */
