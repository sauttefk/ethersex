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

#include "rscp.h"

#ifdef RSCP_SUPPORT
#ifdef ELTAKOMS_SUPPORT

#include "timer.h"
#include "protocols/eltakoms/eltakoms.h"
#include "rscp_eltako_ms.h"

typedef struct eltakoMSChannelConfig {
  uint8_t subchannelType;
  uint16_t reportInterval;
} eltakoMSChannelConfig;

typedef struct eltakoMSChannel {
  timer timer;
  uint16_t channelID;
  uint8_t subchannelType;
  union {
    uint16_t ui16;
    int16_t i16;
    uint8_t ui8;
  } previousValue;
} eltakoMSChannel;

static eltakoMSChannel *eltakoMSChannels;

static uint8_t numEltakoMSChannels = 0;

static void pollELTAKOMS(timer *t, void *usr) {
  eltakoMSChannel *c = (eltakoMSChannel*) usr;

  RSCP_DEBUG("Polling ELKTAKO MS subchannel type: %d, valid=%d\n", c->subchannelType, eltakoms_data.valid);

  if(!eltakoms_data.valid)
    return;

  switch(c->subchannelType) {
    // continuous channels
  case 0x01: // temperature (int16)
    if(eltakoms_data.temperature != c->previousValue.i16) {
      c->previousValue.i16 = eltakoms_data.temperature;
      rscp_txContinuousIOChannelChange(c->channelID, &(c->previousValue.i16), -1, RSCP_UNIT_TEMPERATURE, rscp_field_Decimal16);
    }
    break;
  case 0x05: // wind (uint16)
    if(eltakoms_data.wind != c->previousValue.i16) {
      c->previousValue.ui16 = eltakoms_data.wind;
      rscp_txContinuousIOChannelChange(c->channelID, &(c->previousValue.ui16), -1, RSCP_UNIT_SPEED, rscp_field_Decimal16);
    }
    break;
  case 0x06: // twilight (uint16)
    if(eltakoms_data.dawn != c->previousValue.i16) {
      c->previousValue.ui16 = eltakoms_data.dawn;
      rscp_txContinuousIOChannelChange(c->channelID, &(c->previousValue.ui16), 1, RSCP_UNIT_ILLUMINATION, rscp_field_UnsignedShort);
    }
    break;
  case 0x02: // sun east (uint8)
    if(eltakoms_data.sune != c->previousValue.i16) {
      c->previousValue.ui8 = eltakoms_data.sune;
      rscp_txContinuousIOChannelChange(c->channelID, &(c->previousValue.ui8), 3, RSCP_UNIT_ILLUMINATION, rscp_field_UnsignedByte);
    }
    break;
  case 0x03: // sun south (uint8)
    if(eltakoms_data.suns != c->previousValue.i16) {
      c->previousValue.ui8 = eltakoms_data.suns;
      rscp_txContinuousIOChannelChange(c->channelID, &(c->previousValue.ui8), 3, RSCP_UNIT_ILLUMINATION, rscp_field_UnsignedByte);
    }
    break;
  case 0x04: // sun west (uint8)
    if(eltakoms_data.sunw != c->previousValue.i16) {
      c->previousValue.ui8 = eltakoms_data.sunw;
      rscp_txContinuousIOChannelChange(c->channelID, &(c->previousValue.ui8), 3, RSCP_UNIT_ILLUMINATION, rscp_field_UnsignedByte);
    }
    break;

    // binary channels
  case 0x10: // rain
    if(eltakoms_data.rain != c->previousValue.ui8) {
      c->previousValue.ui8 = eltakoms_data.rain;
      rscp_txBinaryIOChannelChange(c->channelID, c->previousValue.ui8);
    }
    break;
  case 0x11: // obscurity
    if(eltakoms_data.obscure != c->previousValue.ui8) {
      c->previousValue.ui8 = eltakoms_data.obscure;
      rscp_txBinaryIOChannelChange(c->channelID, c->previousValue.ui8);
    }
    break;
  default:
    RSCP_DEBUG("Unsupported ELKTAKO MS subchannel type: %d\n", c->subchannelType);
    break;
  }
}

void rscp_parseEltakoChannels(void *ptr, uint16_t items, uint16_t firstChannelID) {
  eltakoMSChannelConfig *eeConfig = (eltakoMSChannelConfig*) ptr;

  eltakoMSChannels = (eltakoMSChannel*) malloc(items * sizeof(eltakoMSChannel));
  if(!eltakoMSChannels) {
    RSCP_DEBUG("Can't allocate memory for channels\n");
    return;
  }

  numEltakoMSChannels = items;
  for(int i=0; i < items; i++) {
    eltakoMSChannelConfig *cfg = eeConfig + i;
    eltakoMSChannel *c = eltakoMSChannels + i;

    uint16_t interval = rscpEEReadWord(cfg->reportInterval);
    c->subchannelType = rscpEEReadByte(cfg->subchannelType);
    c->channelID = firstChannelID + i;
    c->previousValue.ui16 = 0xffff;

    if(interval) {
      uint32_t millis = interval * 1000L;

      RSCP_DEBUG("Scheduling poll of ELTAKO MS subchannel: %d, interval=%ld\n", c->subchannelType, millis);
      timer_init(&(c->timer), &pollELTAKOMS, c);

      // binary channels are sampled every 10 seconds
      if(c->subchannelType >= 16)
        millis = 10000;

      timer_schedule_repeating_msecs(&(c->timer), millis / 2, millis);
    }
  }
}

void rscp_cleanupEltakoMS() {
  for(int i=0; i < numEltakoMSChannels; i++) {
    eltakoMSChannel *c = eltakoMSChannels + i;
    timer_cancel(&(c->timer));
  }
  if(eltakoMSChannels)
    free(eltakoMSChannels);
}
#endif

#endif /* RSCP_SUPPORT */
