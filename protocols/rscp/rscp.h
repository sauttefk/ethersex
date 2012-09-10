/*
 * (c) 2012 Frank Sautter <ethersix@sautter.com>
 *
 * This program is free software; you can redistsribute it and/or modify it
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

#include "config.h"
#include "rscp_net.h"

#ifndef _RSCP_H
#define _RSCP_H

#ifdef RSCP_SUPPORT

#ifdef DEBUG_RSCP
#include "core/debug.h"
#define RSCP_DEBUG(str...) debug_printf ("rscp: " str)
#else
#define RSCP_DEBUG(...)    ((void) 0)
#endif

extern uint8_t rscp_mode;

void rscp_setup(void);
void rscp_main(void);
void rscp_get(uint8_t * src_addr, uint16_t msg_type, uint16_t payload_len,
              uint8_t * payload);

void rscp_periodic(void);
void rscp_sendHeartBeat(void);
void sendPeriodicOutputEvents(void);
void sendPeriodicInputEvents(void);

#define RSCP_SIZE_DEVURL              32
#define FIRMWARE_MAJOR_VERSION        0x00
#define FIRMWARE_MINOR_VERSION        0x00
#define FIRMWARE_SUB_MINOR_VERSION    0x01

#define RSCP_CHANNEL_EVENT            0x1001

#define RSCP_UNIT_COUNT               0x01  // Counter
#define RSCP_UNIT_VOLTAGE             0x02  // Voltage (V)
#define RSCP_UNIT_TEMPERATURE         0x03  // Temperatured (°C)
#define RSCP_UNIT_MASS                0x04  // Mass (kg)
#define RSCP_UNIT_TIME                0x05  // Time (s)
#define RSCP_UNIT_CURRENT             0x06  // Current (A)
#define RSCP_UNIT_VOLUME              0x07  // Volume (l)
#define RSCP_UNIT_PERCENT             0x08  // Percent
#define RSCP_UNIT_BOOLEAN             0x09  // Boolean (on/off)

#define RSCP_FIELD_CAT_LEN_TINY       0x0   // length encoded in the lll bits
#define RSCP_FIELD_CAT_LEN_BYTE       0x1   // length follows as one-byte length specifier
#define RSCP_FIELD_CAT_LEN_INT        0x2   // length follows as two-byte length specifier
#define RSCP_FIELD_CAT_LEN_IMMEDIATE  0x3   // immediate fields (type contains value)

#define RSCP_FIELD_TYPE_FALSE         0x00  // boolean false
#define RSCP_FIELD_TYPE_TRUE          0x01  // boolean true

#endif /* RSCP_SUPPORT */
#endif /* _RSCP_H */
