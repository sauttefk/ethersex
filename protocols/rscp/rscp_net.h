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

#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#endif /* htonl */
#ifndef ntohl
#define ntohl htonl
#endif /* ntohl */

/* constants */

typedef enum rscp_networkMode {
  rscp_ModeRawEthernet,
  rscp_ModeUDP
} rscp_networkMode_t;

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

typedef struct rscp_udp_message
{
  uint8_t  mac[6];                    // mac address of sender
  rscp_message_t message;             // rscp message
} rscp_udp_message_t;

typedef struct rscp_payloadBuffer {
  uint8_t  *start;
  uint8_t  *pos;
} rscp_payloadBuffer_t;

#define RSCP_ETHTYPE                  0x4313
#define RSCP_RAWH_LEN                 sizeof(struct uip_eth_hdr)
#define RSCP_HEADER_LEN               offsetof(rscp_message_t, payload)
#define RSCP_UDP_HEADER_LEN           offsetof(rscp_udp_message_t, message.payload)


/* prototypes */
void rscp_net_init(void);
void rscp_net_raw(void);
rscp_payloadBuffer_t* rscp_getPayloadBuffer();
void rscp_transmit(uint16_t messageType);

// generate encode/decode methods by macro expansion
#define ENCODE_NUMBER(SIZE) int8_t rscp_encodeInt##SIZE (int##SIZE##_t value, rscp_payloadBuffer_t *buffer); \
int8_t rscp_encodeUInt##SIZE (uint##SIZE##_t value, rscp_payloadBuffer_t *buffer);
ENCODE_NUMBER(8)
ENCODE_NUMBER(16)
ENCODE_NUMBER(32)

int8_t rscp_encodeChannel(uint16_t channel, rscp_payloadBuffer_t *buffer);

int8_t rscp_encodeBooleanField(int8_t value, rscp_payloadBuffer_t *buffer);

// generate encode/decode methods by macro expansion
#define ENCODE_NUMBER_FIELD(SIZE, CODE) int8_t rscp_encodeInt##SIZE##Field(int##SIZE##_t value, rscp_payloadBuffer_t *buffer); \
int8_t rscp_encodeUInt##SIZE##Field(uint##SIZE##_t value, rscp_payloadBuffer_t *buffer);
ENCODE_NUMBER_FIELD(8, 0x01)
ENCODE_NUMBER_FIELD(16, 0x03)
ENCODE_NUMBER_FIELD(32, 0x05)

#warning FIXME: support for float/double is missing

int8_t rscp_encodeDecimal16Field(int16_t significand, int8_t scale, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeDecimal24Field(int32_t significand, int8_t scale, rscp_payloadBuffer_t *buffer);
int8_t rscp_encodeDecimal32Field(int32_t significand, int8_t scale, rscp_payloadBuffer_t *buffer);

#endif /* _RSCP_NET_H */

