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
#include "crc32.h"
#include "timer.h"

#ifdef RSCP_SUPPORT

// the interval at which we check whether there's a more recent configuration available
#define CONFIG_CHECK_INTERVAL 60000

/* local prototypes */
#ifdef RSCP_USE_OW
void hook_ow_poll_handler(ow_sensor_t * ow_sensor, uint8_t state);
#endif

/* local variables */
volatile uint8_t rscp_heartbeatCounter;

static void parseBIC(void *ptr, uint16_t items) {
  rscp_binaryInputChannels = malloc(items * sizeof(rscp_binaryInputChannel));
  if (rscp_binaryInputChannels == NULL ) {
    rscp_numBinaryInputChannels = 0;
    RSCP_DEBUG_CONF("Out of memory\n");
    return;
  }

  rscp_numBinaryInputChannels = items;
  memset(rscp_binaryInputChannels, 0, items * sizeof(rscp_binaryInputChannel));
  RSCP_DEBUG_CONF(
      "Allocated %d bytes for %d binary input channels\n", items * sizeof(rscp_binaryInputChannel), items);

  for (uint16_t i = 0; i < items; ++i) {
    rscp_binaryInputChannels[i].channel =
        rscpEE_word(rscp_binaryInputChannel, channel, ptr);
    rscp_binaryInputChannels[i].port =
        rscpEE_word(rscp_binaryInputChannel, port, ptr);
    rscp_binaryInputChannels[i].flags =
        rscpEE_byte(rscp_binaryInputChannel, flags, ptr);

//    RSCP_DEBUG_CONF(
//        "binary input: port: %d - flags: %02x -> %c%c%c\n", rscp_binaryInputChannels[i].port, rscp_binaryInputChannels[i].flags, rscp_binaryInputChannels[i].pullup ? 'P' : 'p', rscp_binaryInputChannels[i].negate ? 'N' : 'n', rscp_binaryInputChannels[i].report ? 'R' : 'r');

    rscp_setPortDDR(rscp_binaryInputChannels[i].port, 0);
    rscp_setPortPORT(rscp_binaryInputChannels[i].port,
        rscp_binaryInputChannels[i].pullup);

    ptr += offsetof(rscp_binaryInputChannel, status);
  }
}

static void parseBOC(void *ptr, uint16_t items) {
  rscp_binaryOutputChannels = malloc(items * sizeof(rscp_binaryOutputChannel));
  if (rscp_binaryOutputChannels == NULL ) {
    rscp_numBinaryOutputChannels = 0;
    RSCP_DEBUG_CONF("out of memory\n");
    return;
  }

  rscp_numBinaryOutputChannels = items;
  memset(rscp_binaryOutputChannels, 0,
      items * sizeof(rscp_binaryOutputChannel));
  RSCP_DEBUG_CONF(
      "allocated %d bytes for %d binary output channels\n", items * sizeof(rscp_binaryOutputChannel), items);

  for (uint16_t i = 0; i < items; ++i) {
    rscp_binaryOutputChannels[i].channel =
        rscpEE_word(rscp_binaryOutputChannel, channel, ptr);
    rscp_binaryOutputChannels[i].port =
        rscpEE_word(rscp_binaryOutputChannel, port, ptr);
    rscp_binaryOutputChannels[i].flags =
        rscpEE_byte(rscp_binaryOutputChannel, flags, ptr);

    RSCP_DEBUG_CONF(
        "binary output: port:%d - flags: 0x%02x -> %c%c%c%c\n", rscp_binaryOutputChannels[i].port, rscp_binaryOutputChannels[i].flags, rscp_binaryOutputChannels[i].openDrain ? 'D' : 'd', rscp_binaryOutputChannels[i].openSource ? 'S' : 's', rscp_binaryOutputChannels[i].negate ? 'N' : 'n', rscp_binaryOutputChannels[i].report ? 'R' : 'r');

    rscp_setPortDDR(rscp_binaryOutputChannels[i].port, 1);
    rscp_setPortPORT(rscp_binaryOutputChannels[i].port,
        rscp_binaryOutputChannels[i].negate);

    ptr += sizeof(rscp_binaryOutputChannel);
  }
}

#ifdef RSCP_USE_OW
static void parseOWC(void *ptr, uint16_t items) {
  rspc_owList_p = ptr;
  rscp_numOwChannels = items;

#ifdef RSCP_DEBUG_CONF
  for (uint16_t i = 0; i < rscp_numOwChannels; i++) {
    onewireTemperatureChannel owItem;

    eeprom_read_block(&owItem, &(rspc_owList_p[i]),
        sizeof(onewireTemperatureChannel));

    RSCP_DEBUG_CONF(
        "1WID: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", owItem.owROM.bytewise[0], owItem.owROM.bytewise[1], owItem.owROM.bytewise[2], owItem.owROM.bytewise[3], owItem.owROM.bytewise[4], owItem.owROM.bytewise[5], owItem.owROM.bytewise[6], owItem.owROM.bytewise[7]);
    RSCP_DEBUG_CONF("interval: %d\n", owItem.interval);
  }
#endif
}
#endif

#if defined(DMX_SUPPORT) && defined(DMX_STORAGE_SUPPORT)
#define RSCP_DMX_SUPPORT
#include "protocols/dmx/dmx.h"
#include "services/dmx-storage/dmx_storage.h"
/*
 * just create an in-RAM copy of the config. It is just four bytes and used rather
 * frequently.
 */
static void parseDMXChannels(void *ptr, uint16_t items) {
  rscp_dmxChannelConfig *eeConfig = (rscp_dmxChannelConfig*) ptr;

  dmxChannelConfig.firstDMXRSCPChannel = rscpEEReadWord(eeConfig->firstDMXRSCPChannel);
  dmxChannelConfig.maxDMXSlot = rscpEEReadWord(eeConfig->maxDMXSlot);

  RSCP_DEBUG_CONF("DMX: first rscp channel: %d, max DMX slot: %d\n",
      dmxChannelConfig.firstDMXRSCPChannel, dmxChannelConfig.maxDMXSlot);
}
#endif

static void parseRuleDefinitions(void) {
  void *ptr = (void *) (rscpEE_word(rscp_conf_header, rule_p,
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

static void parseChannelDefinitions(void) {
  rscp_conf_header *conf = (rscp_conf_header*) rscpEEReadWord(rscpConfiguration->p);
  rscp_chConfig *chConfig = (rscp_chConfig*) (((void*)conf) + rscpEEReadWord(conf->channel_p));
  uint8_t numChannelTypes = rscpEEReadByte(chConfig->numChannelTypes);

  for (uint8_t i = 0; i < numChannelTypes; i++) {
    rscp_chList *chListEntry = &(chConfig->channelTypes[i]);
    uint8_t channelType = rscpEEReadByte(chListEntry->channelType);
    uint16_t items = rscpEEReadByte(chListEntry->channel_list_items);
    void* chConfigPtr = (((void*)conf) + rscpEEReadWord(chListEntry->channel_list_p));

    RSCP_DEBUG_CONF(
        "parsing %u channels of type 0x%02x @ 0x%04x\n", items, channelType, chConfigPtr);

    switch (channelType) {
    case RSCP_CHANNEL_BINARY_INPUT: {
      parseBIC(chConfigPtr, items);
      break;
    }
    case RSCP_CHANNEL_BINARY_OUTPUT: {
      parseBOC(chConfigPtr, items);
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
        parseOWC(chConfigPtr, items);
        break;
      }
#endif /* RSCP_USE_OW */
#ifdef RSCP_DMX_SUPPORT
      case RSCP_CHANNEL_DMX:
      {
        parseDMXChannels(chConfigPtr, items);
        break;
      }
#endif
    default: {
      RSCP_DEBUG_CONF(
          "could not parse channel type 0x%02x --- SKIPPING\n", channelType);
      break;
    }
    };
  }

  RSCP_DEBUG_CONF("PORT: %02x %02x %02x %02x \n", PORTA, PORTF, PORTC, PORTE);
  RSCP_DEBUG_CONF("DDR:  %02x %02x %02x %02x \n", DDRA, DDRF, DDRC, DDRE);
}

static uint32_t calcConfigCRC() {
  // verify configuration CRC
  uint16_t length = rscpEEReadWord(rscpConfiguration->length);
  uint8_t *p = (uint8_t*) rscpEEReadWord(rscpConfiguration->p);
  uint32_t crc = crc32init();
  while (length-- > 0) {
    uint8_t b = eeprom_read_byte(p);
    crc32update(crc, b);
    p++;
  }
  crc32finish(crc);
  return crc;
}

typedef struct {
  enum {
    DS_NONE = 0, DS_INITIATING, DS_IN_PROGRESS
  } state;
  uint8_t txID;
  uint8_t nextBlockNumber;
  uint16_t offset;
  uint16_t remaining;
  uint16_t blockSize;
  uint32_t crc32;
  segmentController segmentController;
  bool downloadDeferred;

  timer timer;
  uint8_t retrys;
} rscp_configDownload;

static rscp_configDownload configDownload;

static void sendConfigDownloadRequest() {
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

  rscp_encodeUInt32(rscpEEReadByte(rscpConfiguration->status) == rscp_configValid
      ?rscpEEReadDWord(rscpConfiguration->crc32) : 0, buffer); // CRC
  rscp_encodeUInt8(configDownload.txID, buffer); // transaction-ID
  rscp_encodeUInt16(configDownload.blockSize, buffer); // packet size
  rscp_encodeRaw("CONF", 4, buffer); // file name
  rscp_encodeUInt8(0, buffer);

  rscp_transmit(RSCP_FILE_TRANSFER_REQUEST, &(configDownload.segmentController.address));

  timer_schedule_after_msecs(&(configDownload.timer), 1000);
}

static void configDownloadTimedOut(timer *t, void *user) {
  switch (configDownload.state) {
  default:
    break; // should not happen
  case DS_INITIATING:
    if (configDownload.retrys-- > 0) {
      RSCP_DEBUG("No response to config download init message - retrying\n");
      sendConfigDownloadRequest();
    } else {
      configDownload.state = DS_NONE;
      RSCP_DEBUG("No response to config download init message - giving up\n");
    }
    break;
  case DS_IN_PROGRESS:
    RSCP_DEBUG("Config download timed out receiving data\n");
    configDownload.state = DS_NONE;
    rscpEEWriteByte(rscpConfiguration->status, rscp_configInvalid);

    rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();
    rscp_encodeUInt8(configDownload.txID, buffer); // transaction-ID
    rscp_encodeRaw("Timed out", 9, buffer); // message
    rscp_encodeUInt8(0, buffer);
    rscp_transmit(RSCP_FILE_TRANSFER_ERROR, &(configDownload.segmentController.address));
    break;
  }
}

static void initiateConfigDownload() {
  if (configDownload.state != DS_NONE) {
    RSCP_DEBUG("Config download already in progress\n");
    return;
  }

  // choose segment controller
  int i=0;
  // FIXME: check last seen as well
  while(i<NUM_SEGMENT_CONTROLLERS && segmentControllers[i].state != RUNNING)
    i++;
  if(i>= NUM_SEGMENT_CONTROLLERS) {
    RSCP_DEBUG("No segment controller available/up\n");
    configDownload.downloadDeferred = true;
    return;
  }

  configDownload.segmentController = segmentControllers[i];
  configDownload.state = DS_INITIATING;
  configDownload.retrys = 15;
  configDownload.blockSize = 512;
  configDownload.txID = txidCounter++;
  configDownload.downloadDeferred = false;

  timer_init(&(configDownload.timer), &configDownloadTimedOut, 0);
  sendConfigDownloadRequest();

  RSCP_DEBUG("Config download request sent: tx=%d, myCRC=%08lx\n", configDownload.txID,
      rscpEEReadByte(rscpConfiguration->status) == rscp_configValid ?
          rscpEEReadDWord(rscpConfiguration->crc32) : 0);
}

static timer configCheckTimer;
static void periodicConfigCheck(timer *t, void *user) {
  RSCP_DEBUG("Periodic config check\n");
  initiateConfigDownload();
  timer_schedule_after_msecs(&configCheckTimer, CONFIG_CHECK_INTERVAL);
}

/* ----------------------------------------------------------------------------
 * initialization of RSCP
 */
void rscp_init(void) {
  timer_init(&configCheckTimer, &periodicConfigCheck, 0);

  memset(segmentControllers, 0,
      sizeof(segmentController) * NUM_SEGMENT_CONTROLLERS);

#ifdef RSCP_DMX_SUPPORT
  dmxChannelConfig.firstDMXRSCPChannel = 0;
  dmxChannelConfig.maxDMXSlot = 0;
#endif

  /*
   * Read configuration
   */
  rscpConfiguration = (rscp_configuration*) RSCP_EEPROM_START;
  uint16_t expectedConfigOffset = (uint16_t) (rscpConfiguration + sizeof(rscpConfiguration));
  RSCP_DEBUG_CONF(
      "Initializing. Start of rscp config in eeprom: 0x%03hx, config file @0x%03hx\n", rscpConfiguration, expectedConfigOffset);

  // verify config status in eeprom
  uint8_t status = rscpEEReadByte(rscpConfiguration->status);
  if (status == rscp_configValid) {
    // we expect the configuration to immediately follow the rscpConfiguration structure
    rscp_conf_header *conf = (rscp_conf_header*) rscpEEReadWord(rscpConfiguration->p);
    if((uint16_t)conf != expectedConfigOffset) {
      RSCP_DEBUG_CONF("Updating config offset from %hd to %hd\n", conf, expectedConfigOffset);
      rscpEEWriteWord(rscpConfiguration->p, expectedConfigOffset);
    }

    // Verify config CRC
    uint32_t actualCRC = calcConfigCRC();
    uint32_t expectedCRC = rscpEEReadDWord(rscpConfiguration->crc32);
    if (expectedCRC == actualCRC) {
      // config integrity is Ok
      uint16_t version = rscpEEReadWord(conf->version);
      RSCP_DEBUG_CONF(
          "version: %hd, mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
          version,
          rscpEEReadByte(conf->mac[0]),
          rscpEEReadByte(conf->mac[1]),
          rscpEEReadByte(conf->mac[2]),
          rscpEEReadByte(conf->mac[3]),
          rscpEEReadByte(conf->mac[4]),
          rscpEEReadByte(conf->mac[5])
      );

      // can we use this version?
      if (version == 1) {
        uint8_t matching = 0;
        for(int i=0; i<6; i++)
          if(rscpEEReadByte(conf->mac[i]) == uip_ethaddr.addr[i])
            matching++;

        // is the configuration for this device?
        if (matching == 6) {
          // we're ready to go!
          // set a different heartbeat offset for each device
          rscp_heartbeatCounter = (uip_ethaddr.addr[0] ^ uip_ethaddr.addr[1]
              ^ uip_ethaddr.addr[2] ^ uip_ethaddr.addr[3] ^ uip_ethaddr.addr[4]
              ^ uip_ethaddr.addr[5]);

          RSCP_DEBUG_CONF("heartbeat offset: %d\n", rscp_heartbeatCounter);

          parseChannelDefinitions();
          parseRuleDefinitions();
#ifdef RSCP_USE_OW
          hook_ow_poll_register(hook_ow_poll_handler);
#endif /* RSCP_USE_OW */

          timer_schedule_after_msecs(&configCheckTimer, CONFIG_CHECK_INTERVAL);

          return;
        } else {
          RSCP_DEBUG_CONF("the config does not match this device's mac address\n");
        }
      } else {
        RSCP_DEBUG_CONF("this firmware only supports rscp config version 1\n");
      }
    } else {
      RSCP_DEBUG_CONF(
          "Configuration CRC32 %08lx doesn't match the expected %08lx", actualCRC, expectedCRC);
    }

    // mark as invalid, so we don't try to verify it again
    rscpEEWriteByte(rscpConfiguration->status, rscp_configInvalid);
  } else {
    RSCP_DEBUG_CONF("Configuration is missing or invalid.\n");
  }

  // if we ended up here, sonething's not quite right with the configuration.
  // try to get a new one ASAP.
  timer_schedule_after_msecs(&configCheckTimer, 500);
}

#ifdef RSCP_USE_OW
void
hook_ow_poll_handler(ow_sensor_t * ow_sensor, uint8_t state)
{
  RSCP_DEBUG("Temperature %d state %d\n", ow_sensor->temp, state);

  for (uint16_t i = 0; i < rscp_numOwChannels; i++)
  {
    onewireTemperatureChannel owItem;

    eeprom_read_block(&owItem, &(rspc_owList_p[i]),
        sizeof(onewireTemperatureChannel));
    if (owItem.owROM.raw == ow_sensor->ow_rom_code.raw)
    {
      if (state == OW_CONVERT)
      {
        RSCP_DEBUG("oneWire channel %d set interval %ds\n", owItem.channel,
            owItem.interval);
        ow_sensor->polling_delay = owItem.interval;
      }
      else if (state == OW_READY)
      {
        RSCP_DEBUG("oneWire channel %d ready\n", owItem.channel);
        rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

        rscp_encodeChannel(owItem.channel, buffer);

        // set unit and value
        rscp_encodeUInt8(RSCP_UNIT_TEMPERATURE, buffer);
        rscp_encodeDecimal16Field(ow_sensor->temp, -1, buffer);

        rscp_transmit(RSCP_CHANNEL_EVENT);
      }
      return;
    }
  }
  RSCP_DEBUG("no channel definition for oneWire: "
      "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
      ow_sensor->ow_rom_code.bytewise[0], ow_sensor->ow_rom_code.bytewise[1],
      ow_sensor->ow_rom_code.bytewise[2], ow_sensor->ow_rom_code.bytewise[3],
      ow_sensor->ow_rom_code.bytewise[4], ow_sensor->ow_rom_code.bytewise[5],
      ow_sensor->ow_rom_code.bytewise[6], ow_sensor->ow_rom_code.bytewise[7]);
}
#endif /* RSCP_USE_OW */

void rscp_main(void) {
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
    RSCP_DEBUG(
        "Invalid value for setBinaryOutputChannel of type %d", payload[0]);
    break;
  }

  rscp_pollBinaryOutputChannelState(boc); // report new state of output
}

void rscp_handleChannelStateCommand(uint8_t* payload) {
  uint16_t channelID = ntohs(((uint16_t*)payload)[0]);

  RSCP_DEBUG("handleChannelStateCommand: channel=%d\n", channelID);

  /*
   * FIXME: we might want to build a list of channelIDs, types and pointers to
   * the actual channel definition struct, to speed up the channel search.
   */
  // search for matching channel...
#ifdef RSCP_DMX_SUPPORT
  RSCP_DEBUG("First RSCP DMX channel: %d, highest DMX slot: %d\n", dmxChannelConfig.firstDMXRSCPChannel, dmxChannelConfig.maxDMXSlot);
  // ... in range associated with DMX
  if(dmxChannelConfig.maxDMXSlot > 0
      && channelID >= dmxChannelConfig.firstDMXRSCPChannel && channelID < dmxChannelConfig.firstDMXRSCPChannel + dmxChannelConfig.maxDMXSlot) {
    int32_t value;
    switch(payload[2]) {
    case rscp_field_Byte:
      value = payload[3];
      break;
    case rscp_field_Short:
      value = ntohl(*((int32_t*)(&payload[3])));
      break;
    case rscp_field_Integer:
      value = ntohl(*((int32_t*)(&payload[3])));
      break;
    default:
      RSCP_DEBUG("Invalid field type for DMX channel: %d", payload[2]);
      return;
    }

    if(value >= 0 && value <= 255)
      set_dmx_channel(0, channelID - dmxChannelConfig.firstDMXRSCPChannel, value);
    else
      RSCP_DEBUG("Invalid DMX channel value: %ld", value);
    return;
  }
#endif

  // ...in binary output channels
  for (uint16_t i = 0; i < rscp_numBinaryOutputChannels; i++)
    if (rscp_binaryOutputChannels[i].channel == channelID) {
      rscp_setBinaryOutputChannel(&(rscp_binaryOutputChannels[i]),
          &(payload[2]));
      return;
    }

  // ...more channel types
}

static void handleSegmentControllerHeartbeat(rscp_nodeAddress *srcAddr,
    uint8_t *payload) {
#ifdef RSCP_DEBUG
  switch(srcAddr->type) {
  case rscp_ModeRawEthernet: {
    uint8_t *a = srcAddr->u.ethNodeAddress.macAddress.addr;
    RSCP_DEBUG("Got segment controller heartbeat from %02X:%02X:%02X:%02X:%02X:%02X\n",
      a[0], a[1], a[2], a[3], a[4], a[5]);
    break;
  }
  case rscp_ModeUDP: {
    uip_ipaddr_t *i = &(srcAddr->u.ipNodeAddress.ipAddress);
    RSCP_DEBUG("Got segment controller heartbeat from %d.%d.%d.%d\n",
        uip_ipaddr1(i), uip_ipaddr2(i), uip_ipaddr3(i), uip_ipaddr4(i));
    break;
  }
  default:
    RSCP_DEBUG("Got segment controller heartbeat of type %d\n", srcAddr->type);
    break;
  }
#endif

  uint8_t state = payload[0];
  bool found = false;
  for (int i = 0; i < NUM_SEGMENT_CONTROLLERS; i++) {
    if (memcmp(srcAddr, &(segmentControllers[i].address), sizeof(rscp_nodeAddress))
        == 0) {
      if (state == RUNNING) {
        // found existing segment controller - move it to the head of the list
        if (i != 0)
          memmove(segmentControllers, &(segmentControllers[1]), sizeof(segmentController) * i);
        found = true;
        // fall out
      } else {
        // else don't bother moving it, just record the new state
        segmentControllers[i].state = state;
        mtime_get_current(&(segmentControllers[i].lastSeen));
        return;
      }
    }
  }

  if(!found) {
    // not found - make room at head of list
    memmove(segmentControllers, &(segmentControllers[1]),
        sizeof(segmentController) * (NUM_SEGMENT_CONTROLLERS - 1));
  }
  mtime_get_current(&(segmentControllers[0].lastSeen));
  segmentControllers[0].address = *srcAddr;
  segmentControllers[0].state = state;

  // if the SC is up and we don't have a config yet, go for a download.
  if(state == RUNNING &&
      (rscpEEReadByte(rscpConfiguration->status) != rscp_configValid || configDownload.downloadDeferred))
    initiateConfigDownload();
}

void rscp_handleMessage(rscp_nodeAddress *srcAddr, uint16_t msg_type,
    uint16_t payload_len, uint8_t * payload) {
#ifdef RSCP_DEBUG
  switch (srcAddr->type) {
  case rscp_ModeRawEthernet: {
    u8_t *a = srcAddr->u.ethNodeAddress.macAddress.addr;
    RSCP_DEBUG(
        "Message from: %02X:%02X:%02X:%02X:%02X:%02X, type: 0x%04X, size: %d\n", a[0], a[1], a[2], a[3], a[4], a[5], msg_type, payload_len);
    break;
  }
  case rscp_ModeUDP: {
    u8_t *a = srcAddr->u.ipNodeAddress.macAddress.addr;
    uip_ipaddr_t *i = &(srcAddr->u.ipNodeAddress.ipAddress);
    RSCP_DEBUG(
        "Message from: %d.%d.%d.%d (%02X:%02X:%02X:%02X:%02X:%02X), type: 0x%04X, size: %d\n",
        uip_ipaddr1(i), uip_ipaddr2(i), uip_ipaddr3(i), uip_ipaddr4(i),
        a[0], a[1], a[2], a[3], a[4], a[5], msg_type, payload_len);
    break;
  }
  default:
    RSCP_DEBUG("Unsupported address type %d\n", srcAddr->type);
    break;
  }
#endif
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
    RSCP_DEBUG(
        "Command to %02X:%02X:%02X:%02X:%02X:%02X isn't for me\n", payload[0], payload[1], payload[2], payload[3], payload[4], payload[5]);
    return;
  }

  switch (msg_type) {
  case RSCP_CHANNEL_EVENT:
#warning FIXME: testcode currently not working
#if 0
    if (RSCP_ISFORME(srcMAC)) {
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

  case RSCP_FILE_TRANSFER_RESPONSE: {
    uint8_t txID = payload[0];
    uint8_t status = payload[1];
    if (configDownload.state == DS_INITIATING && configDownload.txID == txID) {
      uint32_t length = ntohl(*((uint32_t*)&(payload[2])));
      timer_cancel(&(configDownload.timer));

      switch (status) {
      case RSCP_FT_STATUS_DATA_FOLLOWS:
        configDownload.state = DS_IN_PROGRESS;
        configDownload.remaining = length;
        configDownload.offset = 0;
        configDownload.nextBlockNumber = 0;
        configDownload.crc32 = ntohl(*((uint32_t*)&(payload[6])));
        rscpEEWriteWord(rscpConfiguration->length, length);
        rscpEEWriteByte(rscpConfiguration->status, rscp_configUpdating);
        timer_schedule_after_msecs(&(configDownload.timer), 10000);
        RSCP_DEBUG("Config download started: tx=%hhd, length=%ld, crc=%lx\n", configDownload.txID, length, configDownload.crc32);
        break;
      case RSCP_FT_STATUS_NOT_MODIFIED:
        RSCP_DEBUG("Config is up to date\n", status);
        configDownload.state = DS_NONE;
        break;
      case RSCP_FT_STATUS_NOT_FOUND: // huh?
        RSCP_DEBUG("Config not found\n", status);
        configDownload.state = DS_NONE;
        break;
      default:
        RSCP_DEBUG("Config download unexpected status: %d\n", status);
        configDownload.state = DS_NONE;
        break;
      }
    } else
      RSCP_DEBUG(
          "Unexpected file transfer response: tx=%d, status=%d\n", txID, status);
    break;
  }

  case RSCP_FILE_TRANSFER_DATA: {
    uint8_t txID = payload[0];
    if (configDownload.state == DS_IN_PROGRESS && configDownload.txID == txID) {
      uint8_t blockNumber = payload[1];
      uint16_t blockLength =
          configDownload.remaining < configDownload.blockSize ?
              configDownload.remaining : configDownload.blockSize;

      // check block number and allow for block number wraparound
      if (blockNumber == configDownload.nextBlockNumber
          || (blockNumber == 0 && configDownload.nextBlockNumber == 0xff)) {
        configDownload.nextBlockNumber = blockNumber + 1;

        timer_schedule_after_msecs(&(configDownload.timer), 10000);

        uint8_t *dst = (uint8_t*) rscpEEReadWord(rscpConfiguration->p) + configDownload.offset;
        uint8_t *src = payload+2;
        RSCP_DEBUG("Got file transfer block #%d of length %d @%d -> @%04x\n",
            blockNumber, blockLength, configDownload.offset, dst);
        for(int i=0; i<blockLength; i++)
          eeprom_write_byte(dst++, *(src++));
        configDownload.remaining -= blockLength;
        configDownload.offset += blockLength;

        // send ACK
        rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();
        rscp_encodeUInt8(configDownload.txID, buffer);
        rscp_encodeUInt8(blockNumber, buffer);
        rscp_transmit(RSCP_FILE_TRANSFER_ACK, srcAddr);

        // done yet?
        if (configDownload.remaining <= 0) {
          timer_cancel(&(configDownload.timer));
          configDownload.state = DS_NONE;
          uint32_t actual = calcConfigCRC();
          if (actual == configDownload.crc32) {
            RSCP_DEBUG("Downloaded configuration is valid\n");
            rscpEEWriteDWord(rscpConfiguration->crc32, actual);
            rscpEEWriteByte(rscpConfiguration->status, rscp_configValid);
            rscp_init(); // re-initialize
          } else {
            RSCP_DEBUG(
                "Downloaded configuration CRC mismatch: %lx != %lx\n", actual, configDownload.crc32);
            rscpEEWriteByte(rscpConfiguration->status, rscp_configInvalid);
          }
        }
      } else
        RSCP_DEBUG("Data block out of sequence: got=%d, expected=%d\n", blockNumber, configDownload.nextBlockNumber);
    } else
      RSCP_DEBUG("Unexpected file transfer data: state=%d, tx=%d\n", configDownload.state, txID, status);
    break;
  }
  case RSCP_SEGMENT_CTRL_HEARTBEAT: {
    handleSegmentControllerHeartbeat(srcAddr, payload);
    break;
  }
  }
}

#ifdef IRMP_SUPPORT
void
rscp_sendPeriodicIrmpEvents(void)
{
  irmp_data_t irmp_data;
  while (irmp_read(&irmp_data))
  {
    rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();
    rscp_encodeChannel(0xffff, buffer);
    rscp_encodeUInt8(irmp_data.protocol, buffer);
    rscp_encodeUInt16(irmp_data.address, buffer);
    rscp_encodeUInt16(irmp_data.command, buffer);
    rscp_encodeUInt8(irmp_data.flags, buffer);
    rscp_transmit(RSCP_CHANNEL_EVENT, 0);

    RSCP_DEBUG("%02" PRIu8 ":%04" PRIX16 ":%04" PRIX16 ":%02" PRIX8 "\n",
        irmp_data.protocol, irmp_data.address, irmp_data.command,
        irmp_data.flags);
  }
}
#endif

static void sendHeartBeat(void) {
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();
  rscp_encodeUInt8(rscpEEReadByte(rscpConfiguration->status), buffer);
  rscp_encodeInt32(rscpEEReadDWord(rscpConfiguration->crc32), buffer);
  rscp_transmit(RSCP_NODE_HEARTBEAT, 0);
  RSCP_DEBUG("node heartbeat sent\n");
}

void rscp_periodic(void)     // 1Hz interrupt
{
  if (--rscp_heartbeatCounter == 0) {
    /* send a heartbeat packet every 256 seconds */
    rscp_heartbeatCounter = 0;

    sendHeartBeat();
//    rscp_sendPeriodicOutputEvents();
//    rscp_sendPeriodicInputEvents();
  }
}

void rscp_sendPeriodicOutputEvents(void) {
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

#warning FIXME

  rscp_transmit(RSCP_CHANNEL_EVENT, 0);
  RSCP_DEBUG("node output data sent\n");
}

void rscp_sendPeriodicInputEvents(void) {
  rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

#warning FIXME

  rscp_transmit(RSCP_CHANNEL_EVENT, 0);
  RSCP_DEBUG("node input data sent\n");
}

#endif /* RSCP_SUPPORT */

/*
 -- Ethersex META --
 header(protocols/rscp/rscp.h)
 init(rscp_init)
 timer(50, rscp_periodic())
 ifdef(`conf_IRMP',`timer(1, rscp_sendPeriodicIrmpEvents())')
 block(Miscelleanous)
 */
