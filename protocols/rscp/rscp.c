/*
 * (c) 2012 by Frank Sautter <ethersix@sautter.com>
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

/* ----------------------------------------------------------------------------
 * global variables
 */

const uint8_t rscp_deviceURL[RSCP_SIZE_DEVURL] = CONF_RSCP_DEVICEURL;
  // PROGMEM doesn't work?!?


/* ----------------------------------------------------------------------------
 * initialization of RSCP
 */
void
rscp_init(void)
{
  RSCP_DEBUG("init\n");
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
}

void
rscp_periodic(void)
{
  static uint8_t rscp_heartbeatInterval = 3;
  if (--rscp_heartbeatInterval == 0)
  {
    /* send a heartbeat packet every 60 seconds */
    rscp_heartbeatInterval = 60;

//    rscp_sendHeartBeat();
//    sendPeriodicOutputEvents();
//    sendPeriodicInputEvents();
      sendPeriodicTemperature();
  }
}



void
rscp_sendHeartBeat(void)
{
  uint8_t *payload = rscp_getPayloadPointer();
#warning FIXME
  payload[0] = 0;               // no meaning
  payload[1] = 0x47;            // FIXME: zone
  payload[2] = 0x11;            // FIXME: subzone
  rscp_transmit(3, RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node heartbeat sent\n");
}



void
sendPeriodicOutputEvents(void)
{
  uint8_t *payload = rscp_getPayloadPointer();
#warning FIXME
  payload[0] = 0x00;
  payload[1] = 0xA5;            // FIXME: output data
  payload[2] = 0xC3;            // FIXME: output data
  rscp_transmit(3, RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node output data sent\n");
}



void
sendPeriodicInputEvents(void)
{
  uint8_t *payload = rscp_getPayloadPointer();
#warning FIXME
  payload[0] = 0x00;
  payload[1] = 0xA5;            // FIXME: input data
  payload[2] = 0xC3;            // FIXME: input data
  rscp_transmit(3, RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node input data sent\n");
}



void
sendPeriodicTemperature(void)
{
  uint8_t *payload = rscp_getPayloadPointer();
#warning FIXME
  payload[0] = 0x00;
  payload[1] = 0x00;
  payload[2] = RSCP_UNIT_TEMPERATURE;
  payload[3] = RSCP_FIELD_CAT_LEN_TINY << 6 | 0x21;
  payload[4] = (ow_sensors[0].temp >> 8) & 0x1f | -1 << 5;  //
  payload[5] = ow_sensors[0].temp & 0xff;
  RSCP_DEBUG("temp 0x%04x\n", ow_sensors[0].temp);
  rscp_transmit(6, RSCP_CHANNEL_EVENT);
  RSCP_DEBUG("node temperature sent\n");
}



uint8_t
getRSCP_DeviceURL(uint8_t idx)
{
  if (idx < 16)
#warning FIXME: return pgm_read_byte(&rscp_deviceURL[idx]);
    return rscp_deviceURL[idx];

  return 0;
}


uint8_t
rscp_getMDF_URL(uint8_t idx)
{
  return rscp_deviceURL[idx];
}

#endif /* RSCP_SUPPORT */


/*
   -- Ethersex META --
   header(protocols/rscp/rscp.h)
   init(rscp_init)
   timer(50, rscp_periodic())
   block(Miscelleanous)
 */
