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

#ifndef _RSCP_NET_H
#define _RSCP_NET_H

#ifdef DEBUG_RSCP_NET
#include "core/debug.h"
#define RSCP_DEBUG_NET(str...) debug_printf ("RSCP-NET: " str)
#else
#define RSCP_DEBUG_NET(...)    ((void) 0)
#endif


/* constants */
#define RSCP_FIRMWARE_MAJOR_VERSION             0x00
#define RSCP_FIRMWARE_MINOR_VERSION             0x00
#define RSCP_FIRMWARE_SUB_MINOR_VERSION         0x01

typedef enum rscp_networkMode {
  rscp_ModeRawEthernet,
  rscp_ModeUDP
} rscp_networkMode_t;

#define RSCP_RAW_POS_VERSION                    0
#define RSCP_RAW_POS_HEAD                       1
#define RSCP_RAW_POS_SUBSOURCE                  5
#define RSCP_RAW_POS_TIMESTAMP                  7
#define RSCP_RAW_POS_OBID                       11
#define RSCP_RAW_POS_CLASS                      15
#define RSCP_RAW_POS_TYPE                       17
#define RSCP_RAW_POS_SIZE                       19
#define RSCP_RAW_POS_DATA                       21

#define RSCP_UDP_POS_DATA                       23

#define RSCP_RAWH_LEN                           14  // complete ethernet header
#define RSCP_ETHTYPE                            0x4313

#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#endif /* htonl */
#ifndef ntohl
#define ntohl htonl
#endif /* ntohl */


/* structs */
typedef struct rscp_message
{
  uint8_t  version;                   // The version, currently 0
  uint8_t  header_len;                // The length of the header
  uint32_t timestamp;                 // timestamp in miliseconds
  uint16_t msg_type;                  // RSCP message type
  uint16_t payload_len;               // number of valid data bytes
  uint8_t  payload[512];              // data; max 512 bytes
} rscp_message_t;

#define RSCP_HEADER_LEN                         offsetof(rscp_message_t, payload)

typedef struct rscp_udp_message
{
  uint8_t  mac[6];                    // mac address of sender
  rscp_message_t message;             // rscp message
} rscp_udp_message_t;

#define RSCP_UDP_HEADER_LEN                         offsetof(rscp_udp_message_t, message.payload)


/* prototypes */
void rscp_net_init(void);
void rscp_net_raw(void);
uint8_t* rscp_getPayloadPointer();
void rscp_transmit(uint16_t size, uint16_t type);

#endif /* _RSCP_NET_H */

