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

volatile uint8_t rscp_heartbeatCounter;


void rscp_parseBIC(void *ptr, uint16_t items)
{
  rscp_binaryInputChannels =
    malloc(items * sizeof(rscp_binaryInputChannel));
  if (rscp_binaryInputChannels == NULL) {
    rscp_numBinaryInputChannels = 0;
    RSCP_DEBUG_CONF("Out of memory\n");
    return;
  }

  rscp_numBinaryInputChannels = items;
  memset(rscp_binaryInputChannels, 0,
    items * sizeof(rscp_binaryInputChannel));
  RSCP_DEBUG_CONF("Allocated %d bytes for %d binary input channels\n",
    items * sizeof(rscp_binaryInputChannel), items);

  for (uint16_t i = 0; i < items; ++i)
  {
    rscp_binaryInputChannels[i].channel =
      rscpEE_word(rscp_binaryInputChannel, channel, ptr);
    rscp_binaryInputChannels[i].port =
      rscpEE_word(rscp_binaryInputChannel, port, ptr);
    rscp_binaryInputChannels[i].flags =
      rscpEE_byte(rscp_binaryInputChannel, flags, ptr);

    RSCP_DEBUG_CONF("binary input: port: %d - flags: %02x -> %c%c%c\n",
      rscp_binaryInputChannels[i].port,
      rscp_binaryInputChannels[i].flags,
      rscp_binaryInputChannels[i].pullup ? 'P' : 'p',
      rscp_binaryInputChannels[i].negate ? 'N' : 'n',
      rscp_binaryInputChannels[i].report ? 'R' : 'r');

    rscp_setPortDDR(rscp_binaryInputChannels[i].port, 0);
    rscp_setPortPORT(rscp_binaryInputChannels[i].port,
      rscp_binaryInputChannels[i].pullup);

    ptr += offsetof(rscp_binaryInputChannel, status);
  }
}


void rscp_parseBOC(void *ptr, uint16_t items)
{
  rscp_binaryOutputChannels =
    malloc(items * sizeof(rscp_binaryOutputChannel));
  if (rscp_binaryOutputChannels == NULL) {
    rscp_numBinaryOutputChannels = 0;
    RSCP_DEBUG_CONF("out of memory\n");
    return;
  }

  rscp_numBinaryOutputChannels = items;
  memset(rscp_binaryOutputChannels, 0,
    items * sizeof(rscp_binaryOutputChannel));
  RSCP_DEBUG_CONF("allocated %d bytes for %d binary output channels\n",
    items * sizeof(rscp_binaryOutputChannel), items);

  for (uint16_t i = 0; i < items; ++i)
  {
    rscp_binaryOutputChannels[i].channel =
      rscpEE_word(rscp_binaryOutputChannel, channel, ptr);
    rscp_binaryOutputChannels[i].port =
        rscpEE_word(rscp_binaryOutputChannel, port, ptr);
    rscp_binaryOutputChannels[i].flags =
        rscpEE_byte(rscp_binaryOutputChannel, flags, ptr);

    RSCP_DEBUG_CONF("binary output: port:%d - flags: 0x%02x -> %c%c%c%c\n",
      rscp_binaryOutputChannels[i].port,
      rscp_binaryOutputChannels[i].flags,
      rscp_binaryOutputChannels[i].openDrain ? 'D' : 'd',
      rscp_binaryOutputChannels[i].openSource ? 'S' : 's',
      rscp_binaryOutputChannels[i].negate ? 'N' : 'n',
      rscp_binaryOutputChannels[i].report ? 'R' : 'r');

    rscp_setPortDDR(rscp_binaryOutputChannels[i].port, 1);
    rscp_setPortPORT(rscp_binaryOutputChannels[i].port,
      rscp_binaryOutputChannels[i].negate);

    ptr += sizeof(rscp_binaryOutputChannel);
  }
}


void rscp_parseOWC(void *ptr, uint16_t items)
{
  rscp_owChannels =
    malloc(items * sizeof(rscp_owChannel));
  if (rscp_owChannels == NULL) {
    rscp_numOwChannels = 0;
    RSCP_DEBUG_CONF("out of memory\n");
    return;
  }

  rspc_owList_p = ptr;
  rscp_numOwChannels = items;
  memset(rscp_owChannels, 0,
    items * sizeof(rscp_owChannel));
  RSCP_DEBUG_CONF("allocated %d bytes for %d onewire channels\n",
    items * sizeof(rscp_owChannels), items);

  for (uint16_t i = 0; i < rscp_numOwChannels; i++)
  {
#ifdef RSCP_DEBUG_CONF
    onewireTemperatureChannel owItem;

    eeprom_read_block(&owItem, ptr, sizeof(onewireTemperatureChannel));

    RSCP_DEBUG_CONF("1WID: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
        owItem.owROM.bytewise[0], owItem.owROM.bytewise[1],
        owItem.owROM.bytewise[2], owItem.owROM.bytewise[3],
        owItem.owROM.bytewise[4], owItem.owROM.bytewise[5],
        owItem.owROM.bytewise[6], owItem.owROM.bytewise[7]);
    RSCP_DEBUG_CONF("interval: %d\n", owItem.interval);
#endif

    // some initial delay for each channel;
    rscp_owChannels[i].interval = rscp_heartbeatCounter + 5 + i;

    ptr += sizeof(onewireTemperatureChannel);
  }
}


void
rscp_parseRuleDefinitions(void)
{
  void *ptr = (void *)(rscpEE_word(rscp_conf_header, rule_p,
    RSCP_EEPROM_START) + RSCP_EEPROM_START);

  RSCP_DEBUG_CONF("rule pointer: 0x%04X\n", ptr);

  /*
  uint16_t numRules = rscpEE_word(rscp_conf_header, numRules,
    RSCP_EEPROM_START);

  RSCP_DEBUG_CONF("number of rules:  %u\n", numRules);
  while(numRules-- > 0)
  {
  }
   */
}


void
rscp_parseChannelDefinitions(void)
{
  RSCP_DEBUG_CONF("PORT: %02x %02x %02x %02x \n", PORTA, PORTF, PORTC, PORTE);
  RSCP_DEBUG_CONF("DDR:  %02x %02x %02x %02x \n", DDRA, DDRF, DDRC, DDRE);

  rscp_chConfig *chConfig = (rscp_chConfig *)(rscpEE_word(rscp_conf_header, channel_p, RSCP_EEPROM_START) + RSCP_EEPROM_START);

  uint8_t numChannelTypes = rscpEE_byte(rscp_chConfig, numChannelTypes, chConfig);

  for(int i=0; i<numChannelTypes; i++)
  {
    rscp_chList *chListEntry = &(chConfig->channelTypes[i]);
    uint8_t channelType = rscpEE_byte(rscp_chList, channelType, chListEntry);
    uint16_t items = rscpEE_word(rscp_chList, channel_list_items, chListEntry);
    void* chConfigPtr = (void*) rscpEE_word(rscp_chList, channel_list_p, chListEntry) +
      RSCP_EEPROM_START;

    RSCP_DEBUG_CONF("parsing %u channels of type 0x%02x @ 0x%04x\n",
        items, channelType, chConfigPtr);

    switch (channelType)
    {
      case RSCP_CHANNEL_BINARY_INPUT:
      {
        rscp_parseBIC(chConfigPtr, items);
        break;
      }
      case RSCP_CHANNEL_BINARY_OUTPUT:
      {
        rscp_parseBOC(chConfigPtr, items);
        break;
      }
#if 0
      case RSCP_CHANNEL_COMPLEX_INPUT:
      {
        break;
      }
      case RSCP_CHANNEL_COMPLEX_OUTPUT:
      {
        rscp_parseCOC((void *)(chConfigPtr), items);
        break;
      }
#endif
#ifdef RSCP_USE_OW
      case RSCP_CHANNEL_OWTEMPERATURE:
      {
        rscp_parseOWC(chConfigPtr, items);
        break;
      }
#endif /* RSCP_USE_OW */
      default:
      {
        RSCP_DEBUG_CONF("could not parse channel type 0x%02x --- SKIPPING\n",
          channelType);
        break;
      }
    };
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
  RSCP_DEBUG_CONF("start of rscp config in eeprom: 0x%03X\n",
      RSCP_EEPROM_START);
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

  // set a different heartbeat offset for each device
  rscp_heartbeatCounter =
      (uip_ethaddr.addr[0] ^ uip_ethaddr.addr[1] ^ uip_ethaddr.addr[2] ^
       uip_ethaddr.addr[3] ^ uip_ethaddr.addr[4] ^ uip_ethaddr.addr[5]);

  RSCP_DEBUG_CONF("hearbeat offset: %d\n", rscp_heartbeatCounter);

  rscp_parseChannelDefinitions();
  rscp_parseRuleDefinitions();
}


void
rscp_main(void)
{
  // RSCP_DEBUG("bla\n");
}

void rscp_setBinaryOutputChannel(rscp_binaryOutputChannel *boc,
  uint8_t* payload) {
  RSCP_DEBUG("setBinaryOutputChannel(%d): ", boc->channel);

  switch (payload[0]) {
    case 0x10: // boolean false
      RSCP_DEBUG("off\n");
      rscp_setPortPORT(boc->port, 0);
      break;
    case 0x11: // boolean true
      RSCP_DEBUG("on\n");
      rscp_setPortPORT(boc->port, 1);
      break;
    default:
      RSCP_DEBUG("Invalid value for setBinaryOutputChannel of type %d",
          payload[0]);
      break;
  }

  rscp_pollBinaryOutputChannelState(boc); // report new state of output
}


void rscp_handleChannelStateCommand(uint8_t* payload)
{
  uint16_t channelID = ntohs(((uint16_t*)payload)[0]);

  RSCP_DEBUG("handleChannelStateCommand: channel=%d\n", channelID);

  /*
   * FIXME: we might want to build a list of channelIDs, types and pointers to
   * the actual channel definition struct, to speed up the channel search.
   */
  // search for matching channel...
  // ...in binary output channels
  for (uint16_t i = 0; i < rscp_numBinaryOutputChannels; i++)
    if (rscp_binaryOutputChannels[i].channel == channelID)
    {
      rscp_setBinaryOutputChannel(&(rscp_binaryOutputChannels[i]), &(payload[2]));
      return;
    }

  // ...more channel types
}


void
rscp_handleMessage(uint8_t * src_addr, uint16_t msg_type,
    uint16_t payload_len, uint8_t * payload)
{
  RSCP_DEBUG("Message from: %02X:%02X:%02X:%02X:%02X:%02X, type: 0x%04X, size: %d\n",
      src_addr[0], src_addr[1], src_addr[2], src_addr[3], src_addr[4], src_addr[5],
      msg_type,
      payload_len);
#ifdef DEBUG_RSCP_PAYLOAD
  for (int i = 0; i < payload_len; i++) {
    if((i % 32) == 0)
      RSCP_DEBUG("    ");
    printf_P(PSTR("%s%02X"), ((i > 0) ? " " : ""), payload[i]);
  }
  printf_P(PSTR("\n"));
#endif /* DEBUG_RSCP */

  // Is this a command? Check whether this is even for me.
  if ((msg_type & 0xf000) == 0x2000 && !RSCP_ISFORME(payload)) {
    RSCP_DEBUG("Command to %02X:%02X:%02X:%02X:%02X:%02X isn't for me\n",
      payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
    return;
  }

  switch (msg_type) {
    case RSCP_CHANNEL_EVENT:
#warning FIXME: testcode currently not working
#if 0
      if (RSCP_ISFORME(src_addr)) {
      }
      if (payload[2] == RSCP_UNIT_BOOLEAN &&
        payload[3] == 0x11)
      {
        uint16_t channelID = ntohs(*((uint16_t *)&payload[0]));
        if (channelID - rscp_binaryInputChannels[0].channel > 0 &&
            channelID - rscp_binaryInputChannels[0].channel <= 16) {
          RSCP_DEBUG("** MATCH ** channel %d\n", channelID);
//          rscp_togglePortPORT(channelID -
//            rscp_binaryInputChannels[0].channel +
//            rscp_binaryOutputChannels[0].channel);
        }
      }
#endif
      break;

    case RSCP_CHANNEL_STATE_CMD:
      rscp_handleChannelStateCommand(&(payload[6]));
      break;
  }
}

void
rscp_periodic(void)     // 1Hz interrupt
{
  if (--rscp_heartbeatCounter == 0)
  {
    /* send a heartbeat packet every 256 seconds */
    rscp_heartbeatCounter = 0;

    rscp_sendHeartBeat();
//    rscp_sendPeriodicOutputEvents();
//    rscp_sendPeriodicInputEvents();
  }
#ifdef RSCP_USE_OW
  rscp_sendPeriodicTemperature();
#endif /* RSCP_USE_OW */
}


void
rscp_sendHeartBeat(void)
{
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();
  rscp_encodeUInt8(RSCP_NODE_STATE_RUNNING, buffer);
  rscp_transmit(RSCP_NODE_HEARTBEAT);
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


#ifdef RSCP_USE_OW
void
rscp_sendPeriodicTemperature(void)
{
  for (uint16_t i = 0; i < rscp_numOwChannels; i++)
  {
    if (rscp_owChannels[i].interval-- == 0)
    {
      onewireTemperatureChannel owItem;

      eeprom_read_block(&owItem, &(rspc_owList_p[i]),
          sizeof(onewireTemperatureChannel));

      rscp_owChannels[i].interval = owItem.interval;

      ow_sensor_t *owSensor = ow_find_sensor(&owItem.owROM);

      if (owSensor != NULL)
      {
        rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

        rscp_encodeChannel(owItem.channel, buffer);

        // set unit and value
        rscp_encodeUInt8(RSCP_UNIT_TEMPERATURE, buffer);
        rscp_encodeDecimal16Field(owSensor->temp, -1, buffer);

        rscp_transmit(RSCP_CHANNEL_EVENT);
      }
      else
      {
        RSCP_DEBUG("warning oneWire sensor not present: "
            "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
            owItem.owROM.bytewise[0], owItem.owROM.bytewise[1],
            owItem.owROM.bytewise[2], owItem.owROM.bytewise[3],
            owItem.owROM.bytewise[4], owItem.owROM.bytewise[5],
            owItem.owROM.bytewise[6], owItem.owROM.bytewise[7]);
      }
    }
  }

}
#endif /* RSCP_USE_OW */

/*
 * General encoding methods
 */
// generate integer encode/decode methods by macro expansion
#define ENCODE_NUMBER(SIZE) int8_t rscp_encodeInt##SIZE (int##SIZE##_t value, rscp_payloadBuffer_t *buffer) { \
  for(int shift = SIZE - 8; shift >= 0; shift -= 8) \
    *(buffer->pos++) = (value >> shift) & 0xff; \
  return 0; \
} \
int8_t rscp_encodeUInt##SIZE (uint##SIZE##_t value, rscp_payloadBuffer_t *buffer) { \
  for(int shift = SIZE - 8; shift >= 0; shift -= 8) \
    *(buffer->pos++) = (value >> shift) & 0xff; \
  return 0; \
}
ENCODE_NUMBER(8)
ENCODE_NUMBER(16)
ENCODE_NUMBER(32)

int8_t rscp_encodeChannel(uint16_t channel, rscp_payloadBuffer_t *buffer) {
  return rscp_encodeUInt16(channel, buffer);
}

/*
 * Field encoding methods
 */
int8_t rscp_encodeBooleanField(int8_t value, rscp_payloadBuffer_t *buffer) {
  *(buffer->pos++) = value ? 0x11 : 0x10;
  return 0;
}

// generate integer encode/decode methods by macro expansion
#define ENCODE_NUMBER_FIELD(SIZE, CODE) int8_t rscp_encodeInt##SIZE##Field(int##SIZE##_t value, rscp_payloadBuffer_t *buffer) { \
  *(buffer->pos++) = CODE; \
  for(int shift = SIZE - 8; shift >= 0; shift -= 8) \
    *(buffer->pos++) = (value >> shift) & 0xff; \
  return 0; \
} \
int8_t rscp_encodeUInt##SIZE##Field(uint##SIZE##_t value, rscp_payloadBuffer_t *buffer) { \
  *(buffer->pos++) = CODE+1; \
  for(int shift = SIZE - 8; shift >= 0; shift -= 8) \
    *(buffer->pos++) = (value >> shift) & 0xff; \
  return 0; \
}
ENCODE_NUMBER_FIELD(8, 0x01)
ENCODE_NUMBER_FIELD(16, 0x03)
ENCODE_NUMBER_FIELD(32, 0x05)

// FIXME: support for float/double?

int8_t rscp_encodeDecimal16Field(int16_t significand, int8_t scale, rscp_payloadBuffer_t *buffer) {
  if(scale < -4 || scale > 3 || significand < -4096 || significand > 4095)
    return -1;

  *(buffer->pos++) = 0x0b;
  *(buffer->pos++) = (scale << 5) | ((significand >> 8) & 0x1f);
  *(buffer->pos++) = significand & 0xff;

  return 0;
}
int8_t rscp_encodeDecimal24Field(int32_t significand, int8_t scale, rscp_payloadBuffer_t *buffer) {
  if(scale < -8 || scale > 7 || significand < -524288 || significand > 524287)
    return -1;

  *(buffer->pos++) = 0x0c;
  *(buffer->pos++) = (scale << 4) | ((significand >> 16) & 0x0f);
  *(buffer->pos++) = (significand >> 8) & 0xff;
  *(buffer->pos++) = significand & 0xff;

  return 0;
}
int8_t rscp_encodeDecimal32Field(int32_t significand, int8_t scale, rscp_payloadBuffer_t *buffer) {
  if(scale < -16 || scale > 15 || significand < -67108864 || significand > 67108863)
    return -1;

  *(buffer->pos++) = 0x0d;
  *(buffer->pos++) = (scale << 3) | ((significand >> 24) & 0x07);
  *(buffer->pos++) = (significand >> 16) & 0xff;
  *(buffer->pos++) = (significand >> 8) & 0xff;
  *(buffer->pos++) = significand & 0xff;

  return 0;
}

#endif /* RSCP_SUPPORT */

/*
   -- Ethersex META --
   header(protocols/rscp/rscp.h)
   init(rscp_init)
   timer(50, rscp_periodic())
   block(Miscelleanous)
 */
