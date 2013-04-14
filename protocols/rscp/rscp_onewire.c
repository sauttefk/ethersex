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

#include <stdlib.h>
#include "rscp.h"
#include "rscp_onewire.h"

#ifdef RSCP_SUPPORT
#ifdef RSCP_ONEWIRE_SUPPORT

#include "timer.h"
#include "hardware/onewire/onewire.h"

typedef struct  __attribute__ ((packed))
{
  uint16_t pollTimer;           // interval
} rscp_owChannel;

typedef struct __attribute__ ((packed))
{                               // channel type 0x30 (ow temperature)
  ow_rom_code_t owROM;          // onewire ROM code
  uint16_t interval;            // report-interval (s)
} onewireTemperatureChannel;

static uint16_t numOwChannels;
static uint16_t firstOWChannel;
static onewireTemperatureChannel *owList_p;
static rscp_owChannel *owChannels = 0;

void
hook_ow_poll_handler(ow_sensor_t * ow_sensor, uint8_t state)
{
  RSCP_DEBUG_IO("Temperature %d state %d\n", ow_sensor->temp, state);

  for (uint16_t i = 0; i < numOwChannels; i++)
  {
    onewireTemperatureChannel owItem;
    uint16_t channelID = firstOWChannel + i;

    eeprom_read_block(&owItem, &(owList_p[i]),
        sizeof(onewireTemperatureChannel));
    if (owItem.owROM.raw == ow_sensor->ow_rom_code.raw)
    {
      if (state == OW_READY && owChannels[i].pollTimer == 0)
      {
        RSCP_DEBUG_IO("polling oneWire channel %d\n", channelID);
        rscp_payloadBuffer_t *buffer = rscp_getPayloadBuffer();

        rscp_encodeChannel(channelID, buffer);

        // set unit and value
        rscp_encodeUInt8(RSCP_UNIT_TEMPERATURE, buffer);
        rscp_encodeDecimal16Field(ow_sensor->temp, -1, buffer);

        rscp_transmit(RSCP_CHANNEL_EVENT, 0);

        // reset poll timer
        owChannels[i].pollTimer = owItem.interval;
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

void rscp_onewire_periodic() {
  // Decrement poll timers until they are zero.
  // The hook function will then proceed to poll the channel and reset the
  // timer.
  for (uint16_t i = 0; i < numOwChannels; i++)
    if(owChannels[i].pollTimer > 0)
      owChannels[i].pollTimer--;
}

void rscp_parseOWC(void *ptr, uint16_t items, uint16_t firstChannelID) {
  if(owChannels)
    free(owChannels);

  owChannels = malloc(items * sizeof(rscp_owChannel));
  if (!owChannels) {
    firstOWChannel = 0;
    RSCP_DEBUG_CONF("Out of memory\n");
    return;
  }
  RSCP_DEBUG_CONF("Allocated %d bytes for %d onewire channels\n", items * sizeof(rscp_owChannel), items);

  memset(owChannels, 0, items * sizeof(rscp_owChannel));

  owList_p = ptr;
  numOwChannels = items;
  firstOWChannel = firstChannelID;

#ifdef RSCP_DEBUG_CONF
  for (uint16_t i = 0; i < numOwChannels; i++) {
    onewireTemperatureChannel owItem;

    eeprom_read_block(&owItem, &(owList_p[i]),
        sizeof(onewireTemperatureChannel));

    RSCP_DEBUG_CONF(
        "onewire temperature: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X, interval: %d\n",
        owItem.owROM.bytewise[0], owItem.owROM.bytewise[1], owItem.owROM.bytewise[2], owItem.owROM.bytewise[3], owItem.owROM.bytewise[4], owItem.owROM.bytewise[5], owItem.owROM.bytewise[6], owItem.owROM.bytewise[7],
        owItem.interval
    );
  }
#endif

  if(numOwChannels > 0)
    hook_ow_poll_register(hook_ow_poll_handler);
}

/*
  -- Ethersex META --
  header(protocols/rscp/rscp_onewire.h)
  timer(50, rscp_onewire_periodic())
  block(Miscelleanous)
 */
#endif
#endif
