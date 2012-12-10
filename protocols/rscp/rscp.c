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
#include "crc32.h"
#include "timer.h"
#include "protocols/syslog/syslog.h"

#include "rscp_eltako_ms.h"
#include "rscp_onewire.h"
#include "rscp_dmx.h"

// the interval at which we check whether there's a more recent configuration available
#define CONFIG_CHECK_INTERVAL 60000

/* local variables */
volatile uint8_t rscp_heartbeatCounter;


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
    uint16_t items = rscpEEReadWord(chListEntry->channel_list_items);
    uint16_t firstChannelID = rscpEEReadWord(chListEntry->firstChannelID);
    void* chConfigPtr = (((void*)conf) + rscpEEReadWord(chListEntry->channel_list_p));

    RSCP_DEBUG_CONF(
        "parsing %u channels of type 0x%02x @ 0x%04x\n", items, channelType, chConfigPtr);

    switch (channelType) {
    case RSCP_CHANNEL_BINARY_INPUT:
      rscp_parseBIC(chConfigPtr, items, firstChannelID);
      break;
    case RSCP_CHANNEL_BINARY_OUTPUT:
      rscp_parseBOC(chConfigPtr, items, firstChannelID);
      break;
#if 0
      case RSCP_CHANNEL_COMPLEX_INPUT:
      {
        break;
      }
      case RSCP_CHANNEL_COMPLEX_OUTPUT:
      {
        rscp_parseCOC((void *)(chConfigPtr), items, firstChannelID);
        break;
      }
#endif
#ifdef RSCP_ONEWIRE_SUPPORT
      case RSCP_CHANNEL_OWTEMPERATURE:
        rscp_parseOWC(chConfigPtr, items, firstChannelID);
        break;
#endif
#ifdef RSCP_DMX_SUPPORT
      case RSCP_CHANNEL_DMX:
        rscp_parseDMXChannels(chConfigPtr, items, firstChannelID);
        break;
#endif
#ifdef ELTAKOMS_SUPPORT
      case RSCP_CHANNEL_ELTAKO_MS:
        rscp_parseEltakoChannels(chConfigPtr, items, firstChannelID);
        break;
#endif
    default:
      RSCP_DEBUG_CONF(
          "could not parse channel type 0x%02x --- SKIPPING\n", channelType);
      break;
    };
  }
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
static timer delayedInitTimer;
static void parseConfig(void);
static void delayedInit(timer *t, void *user) {
  static uint8_t trys = 25;

  if(!syslog_is_available() && trys-- > 0)
    timer_schedule_after_msecs(&delayedInitTimer, 200);
  else
    parseConfig(); // give up for now
}

void rscp_init(void) {

  timer_init(&configCheckTimer, &periodicConfigCheck, 0);

  memset(segmentControllers, 0,
      sizeof(segmentController) * NUM_SEGMENT_CONTROLLERS);

  timer_init(&delayedInitTimer, &delayedInit, 0);
  timer_schedule_after_msecs(&delayedInitTimer, 50);
}

static uint8_t isInitialized;

static void parseConfig(void) {
  // cleanup phase. FIXME: find better place
  if(isInitialized == 0x42) {
#ifdef ELTAKOMS_SUPPORT
    rscp_cleanupEltakoMS();
#endif
  }
  isInitialized = 0;
  // end cleanup

  // init phase
#ifdef RSCP_DMX_SUPPORT
  rscp_initDMX();
#endif

  /*
   * Read configuration
   */
  rscpConfiguration = (rscp_configuration*) RSCP_EEPROM_START;
  uint16_t expectedConfigOffset = (uint16_t) (rscpConfiguration + sizeof(rscpConfiguration));
  RSCP_DEBUG_CONF(
      "Initializing. Start of rscp config in eeprom: 0x%03hx, config file @0x%03hx\n", rscpConfiguration, expectedConfigOffset);

  // we expect the configuration to immediately follow the rscpConfiguration structure
  rscp_conf_header *conf = (rscp_conf_header*) rscpEEReadWord(rscpConfiguration->p);
  if((uint16_t)conf != expectedConfigOffset) {
    RSCP_DEBUG_CONF("Updating config offset from %03hx to %03hx\n", conf, expectedConfigOffset);
    rscpEEWriteWord(rscpConfiguration->p, expectedConfigOffset);
  }

  // verify config status in eeprom
  uint8_t status = rscpEEReadByte(rscpConfiguration->status);
  if (status == rscp_configValid) {
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

          timer_schedule_after_msecs(&configCheckTimer, CONFIG_CHECK_INTERVAL);

          isInitialized = 0x42; // magic

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

static void handleChannelStateCommand(uint8_t* payload) {
  uint16_t channelID = ntohs(((uint16_t*)payload)[0]);

  RSCP_DEBUG("handleChannelStateCommand: channel=%d\n", channelID);

  /*
   * FIXME: we might want to build a list of channelIDs, types and pointers to
   * the actual channel definition struct, to speed up the channel search.
   */
  // search for matching channel...
#ifdef RSCP_DMX_SUPPORT
  if(rscp_maybeHandleDMX_CSC(channelID, payload + 2))
    return;
#endif

  // ...in binary output channels
  if(rscp_maybeHandleBOC_CSC(channelID, payload + 2))
    return;

  // ...more channel types
}

typedef struct {
  uint16_t channelID;
  float startValue;
  float finalValue;

} transition;

static void handleTransitionCommand(uint8_t* payload) {
  // FIXME
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
#ifdef RSCP_DEBUG_MSG
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
#ifdef DEBUG_RSCP_PAYLOAD
  for (int i = 0; i < payload_len; i++) {
    if((i % 32) == 0)
    RSCP_DEBUG("    ");
    printf_P(PSTR("%s%02X"), ((i > 0) ? " " : ""), payload[i]);
  }
  printf_P(PSTR("\n"));
#endif /* DEBUG_RSCP */
#endif

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
    handleChannelStateCommand(&(payload[6]));
    break;

  case RSCP_TRANSITION_CMD:
    handleTransitionCommand(&(payload[6]));
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

#endif /* RSCP_SUPPORT */

/*
 -- Ethersex META --
 header(protocols/rscp/rscp.h)
 init(rscp_init)
 timer(50, rscp_periodic())
 ifdef(`conf_IRMP',`timer(1, rscp_sendPeriodicIrmpEvents())')
 block(Miscelleanous)
 */
