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

#include <stdbool.h>

#include "rscp.h"
#include "rscp_dmx.h"

#ifdef RSCP_SUPPORT
#ifdef RSCP_DMX_SUPPORT

typedef struct __attribute__ ((packed)) {
  /*
   * The highest DMX slot used (1-based!).
   */
  uint16_t maxDMXSlot;
} rscp_dmxChannelConfig;

/*
 * The first RSCP channel ID corresponding to DMX slot #1 (1-based!).
 */
static uint16_t firstDMXRSCPChannel;

/*
 * The number of DMX channels used. There may be gaps in the used channels,
 * but this node doesn't care about that at the moment.
 */
static uint16_t numDMXChannels;

/*
 * The highest DMX slot used (1-based!).
 */
static uint16_t maxDMXSlot;

/*
 * just create an in-RAM copy of the config. It is just four bytes and used rather
 * frequently.
 */
void rscp_parseDMXChannels(void *configPtr, uint16_t items, uint16_t firstChannelID) {
  rscp_dmxChannelConfig *eeConfig = (rscp_dmxChannelConfig*) configPtr;

  firstDMXRSCPChannel = firstChannelID;
  maxDMXSlot = rscpEEReadWord(eeConfig->maxDMXSlot);

  dmx_txlen = maxDMXSlot + 1;

  RSCP_DEBUG_CONF("DMX: first rscp channel: %d, max DMX slot: %d\n", firstDMXRSCPChannel, maxDMXSlot);
}

void rscp_dmx_pollState(void) {
  for(int i = 0; i < maxDMXSlot; i++) {
    uint8_t value = get_dmx_channel(0, i - firstDMXRSCPChannel);
    rscp_txContinuousIOChannelChange(firstDMXRSCPChannel + i, &value, 0, RSCP_UNIT_BOOLEAN, rscp_field_UnsignedByte);
  }
}

bool rscp_maybeHandleDMX_CSC(uint16_t channelID, uint8_t *payload) {
  // ... in range associated with DMX
  if(maxDMXSlot > 0
      && channelID >= firstDMXRSCPChannel && channelID < firstDMXRSCPChannel + maxDMXSlot) {
    int32_t value = 0;
    switch(payload[0]) {
    case rscp_field_Byte:
      value = payload[1];
      break;
    case rscp_field_Short:
      value = ntohs(*((int16_t*)(payload + 1)));
      break;
    case rscp_field_Integer:
      value = ntohl(*((int32_t*)(payload + 1)));
      break;
    default:
      RSCP_DEBUG("Invalid field type for DMX channel: %d", payload[2]);
    }

    if(value >= 0 && value <= 255) {
      uint16_t dmxChannel = channelID - firstDMXRSCPChannel;
      uint8_t value = get_dmx_channel(0, dmxChannel);
      set_dmx_channel(0, dmxChannel, value);
      rscp_txContinuousIOChannelChange(firstDMXRSCPChannel + dmxChannel, &value, 0, RSCP_UNIT_BOOLEAN, rscp_field_UnsignedByte);
    } else
      RSCP_DEBUG("Invalid DMX channel value: %ld", value);
    return true;
  }
  return false;
}

void rscp_initDMX() {

  static rscp_channelDriver driverConfig;

  firstDMXRSCPChannel = 0;
  maxDMXSlot = 0;

  driverConfig.channelName = "dmx";
  driverConfig.configureChannels = &rscp_parseDMXChannels;
  driverConfig.handleChannelStateCommand = &rscp_maybeHandleDMX_CSC;
  driverConfig.pollState =  &rscp_dmx_pollState;

  rscp_registerChannelDriver(&driverConfig);
}

#endif
#endif
